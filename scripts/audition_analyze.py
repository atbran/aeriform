#!/usr/bin/env python3
"""Analyse the WAVs written by `AeriformTests --render=<dir>`.

For the first phrase note (C4, held 0..1.4 s) it reports, per file:
  level    steady RMS in dBFS (0.5..1.2 s)
  attack   time for the short-term RMS to first reach 70 % of the steady level
  hnr      harmonic-to-noise ratio in dB from the normalised autocorrelation peak
  f0/cents autocorrelation pitch and its deviation from C4 (261.63 Hz)
  cent     spectral centroid in Hz
  h1..h6   harmonic levels relative to the strongest of the first six, in dB
  jitter   standard deviation of the per-50 ms pitch in cents (0.5..1.3 s)

Usage: audition_analyze.py DIR [BASELINE_DIR]
With a baseline directory, each row is followed by the baseline's row.
"""
import sys, os, wave, struct
import numpy as np

C4 = 261.6256


def read_wav(path):
    with wave.open(path, 'rb') as w:
        n, ch, sw, sr = w.getnframes(), w.getnchannels(), w.getsampwidth(), w.getframerate()
        raw = w.readframes(n)
    if sw == 3:
        b = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3)
        x = (b[:, 0].astype(np.int32) | (b[:, 1].astype(np.int32) << 8) | (b[:, 2].astype(np.int32) << 16))
        x = np.where(x >= 1 << 23, x - (1 << 24), x) / float(1 << 23)
    elif sw == 2:
        x = np.frombuffer(raw, dtype=np.int16) / 32768.0
    else:
        raise ValueError('unsupported width')
    x = x.reshape(-1, ch).mean(axis=1)
    return x, sr


def autocorr_pitch(seg, sr, lo=60.0, hi=1200.0):
    seg = seg - seg.mean()
    if np.max(np.abs(seg)) < 1e-6:
        return 0.0, -99.0
    n = len(seg)
    spec = np.fft.rfft(seg * np.hanning(n), 2 * n)
    ac = np.fft.irfft(np.abs(spec) ** 2)[:n]
    ac /= ac[0] + 1e-20
    a, b = int(sr / hi), int(sr / lo)
    k = a + int(np.argmax(ac[a:b]))
    # parabolic refinement
    if 1 <= k < n - 1:
        y0, y1, y2 = ac[k - 1], ac[k], ac[k + 1]
        d = 0.5 * (y0 - y2) / (y0 - 2 * y1 + y2 + 1e-20)
    else:
        d = 0.0
    r = min(max(ac[k], 1e-6), 0.999999)
    return sr / (k + d), 10 * np.log10(r / (1 - r))


def analyse(path):
    x, sr = read_wav(path)
    steady = x[int(0.5 * sr):int(1.2 * sr)]
    rms = np.sqrt(np.mean(steady ** 2)) + 1e-12
    level = 20 * np.log10(rms)
    # attack
    hop = int(0.005 * sr)
    env = np.array([np.sqrt(np.mean(x[i:i + hop] ** 2)) for i in range(0, int(1.2 * sr), hop)])
    idx = np.argmax(env >= 0.7 * rms) if np.any(env >= 0.7 * rms) else len(env)
    attack = idx * hop / sr * 1000
    f0, hnr = autocorr_pitch(steady[: int(0.2 * sr)], sr)
    cents = 1200 * np.log2(f0 / C4) if f0 > 0 else 0
    # spectrum
    n = len(steady)
    spec = np.abs(np.fft.rfft(steady * np.hanning(n)))
    freqs = np.fft.rfftfreq(n, 1 / sr)
    cent = float(np.sum(freqs * spec) / (np.sum(spec) + 1e-20))
    base = f0 if f0 > 0 else C4
    harms = []
    for h in range(1, 7):
        band = (freqs > base * h * 0.97) & (freqs < base * h * 1.03)
        harms.append(np.max(spec[band]) if np.any(band) else 1e-12)
    harms = 20 * np.log10(np.array(harms) / (max(harms) + 1e-20) + 1e-12)
    # jitter
    pitches = []
    for t in np.arange(0.5, 1.3, 0.05):
        p, _ = autocorr_pitch(x[int(t * sr):int((t + 0.05) * sr)], sr)
        if p > 0:
            pitches.append(1200 * np.log2(p / base))
    jitter = float(np.std(pitches)) if pitches else 0.0
    return dict(level=level, attack=attack, hnr=hnr, f0=f0, cents=cents, cent=cent, harms=harms, jitter=jitter)


def row(name, a):
    h = ' '.join('%5.1f' % v for v in a['harms'])
    return '%-26s %6.1f %6.0f %6.1f %7.1f %+6.1f %6.0f %5.1f | %s' % (
        name, a['level'], a['attack'], a['hnr'], a['f0'], a['cents'], a['cent'], a['jitter'], h)


def main():
    d = sys.argv[1]
    base = sys.argv[2] if len(sys.argv) > 2 else None
    print('%-26s %6s %6s %6s %7s %6s %6s %5s | %s' % ('file', 'dBFS', 'atkms', 'hnr', 'f0', 'cents', 'centr', 'jit', 'h1..h6 dB'))
    for f in sorted(os.listdir(d)):
        if not f.endswith('.wav'):
            continue
        print(row(f[:-4], analyse(os.path.join(d, f))))
        if base and os.path.exists(os.path.join(base, f)):
            print(row('  (before)', analyse(os.path.join(base, f))))


if __name__ == '__main__':
    main()
