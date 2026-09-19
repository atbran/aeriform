"""Report P1 engineering measurements; do not set acceptance thresholds."""
from pathlib import Path
import argparse
import csv
import hashlib
import json
import wave
import numpy as np


def write_wav(path, samples, fs):
    if not np.isfinite(samples).all() or np.max(np.abs(samples)) > 1:
        raise ValueError(f"Refusing to silently clip/repair {path}")
    with wave.open(str(path), "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(fs)
        f.writeframes(np.round(samples * 32767).astype("<i2").tobytes())


def rms(x):
    return float(np.sqrt(np.mean(x.astype(np.float64) ** 2)))


def predicted_response(row, frequencies):
    """Independent closed-loop transfer for the low-amplitude impulse fixture.

    push-before-read contributes D-1; the stored feedback contributes one more.
    The impulse fixture remains inside the saturator's identity region.
    """
    fs = float(row["fs"])
    note = float(row["note"])
    closed = row["name"].endswith("-closed")
    track = 2 ** ((note-60)/12)
    lp = np.clip((800 if closed else 4000)*track, 20, .45*fs)
    hp = np.clip((300 if closed else 40)*track, 20, .45*fs)
    a, b = np.exp(-2*np.pi*lp/fs), np.exp(-2*np.pi*hp/fs)
    z = np.exp(-2j*np.pi*frequencies/fs)  # z^-1
    d = float(row["delay"])
    integer, f = int(d), d-int(d)
    c0 = -f*(f-1)*(f-2)/6
    c1 = (f+1)*(f-1)*(f-2)/2
    c2 = -(f+1)*f*(f-2)/2
    c3 = (f+1)*f*(f-1)/6
    fractional = c0/z+c1+c2*z+c3*z*z
    forward = z**(integer-1)*fractional*.5*(1+z)*(1-a)/(1-a*z)*(.5*(1+b)*(1-z)/(1-b*z))
    sign = -1 if row["cylinder"] == "1" else 1
    return .001*forward/(1-sign*float(row["gain"])*z*forward)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("--baseline", type=Path, help="Prior output directory for before/after peak comparison")
    args = parser.parse_args()
    directory = args.output
    with (directory / "renders.csv").open(newline="") as f:
        rows = list(csv.DictReader(f))
    peaks, pressures, hashes, failures = [], [], {}, []
    audio = {}
    for row in rows:
        name = row["name"]
        path = directory / (name + ".f32")
        data = np.fromfile(path, dtype="<f4")
        fs = int(float(row["fs"]))
        hashes[path.name] = hashlib.sha256(path.read_bytes()).hexdigest()
        if not np.isfinite(data).all():
            failures.append(name)
            continue
        if row["kind"] == "impulse":
            expected = 440 * 2 ** ((float(row["note"]) - 69) / 12)
            size = 1 << 20
            spectrum = np.abs(np.fft.rfft(data, n=size))
            lo, hi = int(expected * .8 * size / fs), int(expected * 1.2 * size / fs)
            index = lo + int(np.argmax(spectrum[lo:hi + 1]))
            # Interpolate a local log-magnitude peak. A boundary maximum is not a
            # detected fundamental and must not be mislabeled a pitch measurement.
            interior = lo < index < hi
            shift = 0.
            if interior:
                a, b, c = np.log(np.maximum(spectrum[index - 1:index + 2], 1e-30))
                denominator = a - 2*b + c
                if denominator != 0:
                    shift = float(.5 * (a-c) / denominator)
            hz = (index + shift) * fs / size
            grid = np.linspace(expected*.8, expected*1.2, 20001)
            predicted = np.abs(predicted_response(row, grid))
            predicted_hz = float(grid[np.argmax(predicted)])
            measured_band = spectrum[lo:hi+1]
            expected_band = np.abs(predicted_response(row, np.arange(lo,hi+1)*fs/size))
            response_error = float(np.max(np.abs(measured_band-expected_band))/max(np.max(expected_band),1e-30))
            energy = np.cumsum(data.astype(np.float64)[::-1] ** 2)[::-1]
            db = 10 * np.log10(np.maximum(energy / max(energy[0], 1e-30), 1e-30))
            region = (db < -5) & (db > -25)
            decay = None
            if np.count_nonzero(region) > 10:
                slope = np.polyfit(np.flatnonzero(region)/fs, db[region], 1)[0]
                if slope < 0:
                    decay = float(-60 / slope)
            peaks.append(dict(name=name, expected_hz=expected, peak_hz=hz,
                              local_peak_detected=interior,
                              cents=1200*np.log2(hz/expected) if interior else None,
                              analytic_peak_hz=predicted_hz,
                              spectral_vs_analytic_cents=1200*np.log2(hz/predicted_hz),
                              relative_transfer_magnitude_error=response_error,
                              estimated_t60_s=decay,
                              phase_residual_rad=float(row["phase_residual"])))
        else:
            audio[name] = data
            write_wav(directory/(name+".wav"), data, fs)
            if row["kind"] == "pressure":
                steady = data[int(.5*fs):int(2.5*fs)]
                window = np.hanning(len(steady))
                power = np.abs(np.fft.rfft(steady*window))**2
                frequencies = np.fft.rfftfreq(len(steady), 1/fs)
                centroid = float(np.sum(power*frequencies)/max(np.sum(power),1e-30))
                pressures.append(dict(name=name, steady_rms=rms(steady), centroid_hz=centroid))
    # Fixed-pressure comparisons use one common RMS target and no hidden limiter.
    pressure_names = [r["name"] for r in rows if r["kind"] == "pressure"]
    levels = {n: rms(audio[n][24000:120000]) for n in pressure_names}
    target = min([.06] + [.95*levels[n]/max(float(np.max(np.abs(audio[n]))),1e-30) for n in pressure_names])
    joined = []
    for name in pressure_names:
        matched = audio[name] * (target / max(levels[name],1e-30))
        write_wav(directory/(name+"-matched.wav"), matched, 48000)
        joined.extend([matched,np.zeros(24000)])
    write_wav(directory/"pressure-audition-matched.wav",np.concatenate(joined),48000)
    # Dynamic sweeps are raw, so their performance dynamics remain audible.
    sweep_order = ["pressure-sweep-short","macro-sweep-short","pressure-sweep-long","macro-sweep-long"]
    joined = []
    for name in sweep_order:
        joined.extend([audio[name],np.zeros(24000)])
    write_wav(directory/"sweep-audition-raw.wav",np.concatenate(joined),48000)
    for name, records in [("tuning-decay",peaks),("pressure-metrics",pressures)]:
        with (directory/(name+".csv")).open("w",newline="") as f:
            writer=csv.DictWriter(f,fieldnames=list(records[0]))
            writer.writeheader();writer.writerows(records)
    (directory/"render-sha256.json").write_text(json.dumps(hashes,indent=2)+"\n")
    source_dir = Path(__file__).resolve().parent
    sources = [source_dir/name for name in ("PipePrototype.h","PeakTuning.h","render.cpp","verify.cpp","analyze.py","CMakeLists.txt")]
    sources.append(source_dir.parent.parent/"Source"/"DSP"/"FractionalDelay.h")
    (directory/"source-sha256.json").write_text(json.dumps({str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},indent=2)+"\n")
    valid = [abs(p["cents"]) for p in peaks if p["cents"] is not None]
    summary = dict(renders=len(rows),tuning_probes=len(peaks),nonfinite=failures,
                   guard_activations=sum(int(r["guards"]) for r in rows),
                   clamped=sum(int(r["clamped"]) for r in rows),
                   tuning_probes_without_peak=sum(r.get("peak_tuned", "1") != "1" for r in rows if r["kind"] == "impulse"),
                   max_abs_spectral_peak_cents=max(valid),
                   max_spectral_vs_analytic_cents=max(abs(p["spectral_vs_analytic_cents"]) for p in peaks),
                   max_relative_transfer_magnitude_error=max(p["relative_transfer_magnitude_error"] for p in peaks),
                   no_local_peak=sum(not p["local_peak_detected"] for p in peaks),
                   matching_rms=target,pressure_metrics=pressures)
    (directory/"summary.json").write_text(json.dumps(summary,indent=2)+"\n")
    if args.baseline:
        with (args.baseline/"tuning-decay.csv").open(newline="") as f:
            baseline = {r["name"]: r for r in csv.DictReader(f)}
        comparison = [dict(name=p["name"], before_cents=baseline[p["name"]]["cents"],
                           after_cents=p["cents"]) for p in peaks]
        with (directory/"tuning-before-after.csv").open("w",newline="") as f:
            writer=csv.DictWriter(f,fieldnames=list(comparison[0]))
            writer.writeheader();writer.writerows(comparison)
    print(json.dumps(summary,indent=2))
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
