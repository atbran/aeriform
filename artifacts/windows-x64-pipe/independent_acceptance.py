#!/usr/bin/env python3
"""Independent UI and DSP acceptance testing and measurement tool for PIPE model.
Owner: UI and acceptance agent (Gemini)
Performs independent re-derivations, element-level DSP contracts, and T8/T19 auditions.
Includes T5 tuning correction re-test and before-vs-after comparison.
"""

import os
import sys
import math
import cmath
import csv
import json
import wave
import hashlib
from pathlib import Path
import numpy as np
import scipy.signal as signal

REPO_ROOT = Path("d:/dev/Aeriform-ORCH/Repo")
PROTOTYPE_DIR = REPO_ROOT / "prototypes/pipe"
PROTOTYPE_BASELINE_OUTPUT = PROTOTYPE_DIR / "output"
PROTOTYPE_RETUNED_OUTPUT = PROTOTYPE_DIR / "output-retuned"
PROTOTYPE_REPEAT_OUTPUT = PROTOTYPE_DIR / "output-retuned-repeat"
ARTIFACTS_DIR = REPO_ROOT / "artifacts/windows-x64-pipe"
ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)

# -----------------------------------------------------------------------------
# Gammatone Filterbank (24 bands, ERB scale)
# -----------------------------------------------------------------------------
def erb_space(low_freq=100.0, high_freq=16000.0, num_bands=24):
    low_erb = 21.4 * np.log10(4.37 * low_freq / 1000.0 + 1.0)
    high_erb = 21.4 * np.log10(4.37 * high_freq / 1000.0 + 1.0)
    erbs = np.linspace(low_erb, high_erb, num_bands)
    center_freqs = (10.0 ** (erbs / 21.4) - 1.0) * 1000.0 / 4.37
    return center_freqs

def make_gammatone_filter(fc, fs, order=4):
    b = 1.019 * 24.7 * (4.37 * fc / 1000.0 + 1.0)
    dt = 1.0 / fs
    phi = 2.0 * np.pi * fc * dt
    a = np.exp(-2.0 * np.pi * b * dt)
    b_sos = [1.0, 0.0, 0.0]
    a_sos = [1.0, -2.0 * a * np.cos(phi), a * a]
    sos = np.tile(np.concatenate([b_sos, a_sos]), (order // 2, 1))
    return sos

def gammatone_energies(audio, fs, center_freqs):
    energies = []
    for fc in center_freqs:
        sos = make_gammatone_filter(fc, fs)
        filtered = signal.sosfilt(sos, audio)
        rms = np.sqrt(np.mean(filtered**2))
        db = 20.0 * np.log10(max(rms, 1e-12))
        energies.append(db)
    return np.array(energies)

# -----------------------------------------------------------------------------
# Loudness Matching (BS.1770 K-weighting approximation / integrated LUFS)
# -----------------------------------------------------------------------------
def k_weighting_filter(fs):
    f0 = 1500.0
    k = np.tan(np.pi * f0 / fs)
    vh = 10.0 ** (4.0 / 20.0)
    vb = 10.0 ** (4.0 / 40.0)
    a0 = 1.0 + np.sqrt(2.0) * k + k * k
    b0 = (vh + vb * np.sqrt(2.0) * k + k * k) / a0
    b1 = 2.0 * (k * k - vh) / a0
    b2 = (vh - vb * np.sqrt(2.0) * k + k * k) / a0
    a1 = 2.0 * (k * k - 1.0) / a0
    a2 = (1.0 - np.sqrt(2.0) * k + k * k) / a0
    shelf_sos = np.array([b0, b1, b2, 1.0, a1, a2])

    fc_hp = 38.0
    k_hp = np.tan(np.pi * fc_hp / fs)
    q = 0.5
    den = 1.0 + k_hp / q + k_hp * k_hp
    b0_hp = 1.0 / den
    b1_hp = -2.0 / den
    b2_hp = 1.0 / den
    a1_hp = 2.0 * (k_hp * k_hp - 1.0) / den
    a2_hp = (1.0 - k_hp / q + k_hp * k_hp) / den
    hp_sos = np.array([b0_hp, b1_hp, b2_hp, 1.0, a1_hp, a2_hp])

    return np.vstack([shelf_sos, hp_sos])

def integrated_loudness_lufs(audio, fs):
    sos = k_weighting_filter(fs)
    k_filtered = signal.sosfilt(sos, audio)
    block_size = int(0.4 * fs)
    step_size = int(0.1 * fs)
    if len(k_filtered) < block_size:
        mean_sq = np.mean(k_filtered**2)
        return -0.691 + 10.0 * np.log10(max(mean_sq, 1e-12))
    
    blocks = []
    for start in range(0, len(k_filtered) - block_size + 1, step_size):
        block = k_filtered[start : start + block_size]
        blocks.append(np.mean(block**2))
    blocks = np.array(blocks)
    gamma_a = -70.0
    l_blocks = -0.691 + 10.0 * np.log10(np.maximum(blocks, 1e-12))
    valid = l_blocks > gamma_a
    if not np.any(valid):
        return -70.0
    gamma_r = -0.691 + 10.0 * np.log10(np.mean(blocks[valid])) - 10.0
    valid_rel = l_blocks > gamma_r
    if not np.any(valid_rel):
        return gamma_r
    return -0.691 + 10.0 * np.log10(np.mean(blocks[valid_rel]))

def match_loudness(audio_list, fs, target_lufs=-20.0):
    matched = []
    gains = []
    for x in audio_list:
        lufs = integrated_loudness_lufs(x, fs)
        gain_db = target_lufs - lufs
        gain = 10.0 ** (gain_db / 20.0)
        y = x * gain
        if np.max(np.abs(y)) > 0.98:
            gain *= 0.98 / np.max(np.abs(y))
            y = x * gain
        matched.append(y)
        gains.append(gain)
    return matched, gains

# -----------------------------------------------------------------------------
# Harmonic-to-Noise Ratio (HNR)
# -----------------------------------------------------------------------------
def compute_hnr(audio, fs, f0_nominal=440.0):
    n = len(audio)
    start = int(n * 0.25)
    end = int(n * 0.75)
    x = audio[start:end]
    x = x - np.mean(x)
    corr = np.correlate(x, x, mode='full')
    corr = corr[len(x)-1:]
    corr0 = corr[0]
    if corr0 <= 0:
        return 0.0
    p_nom = int(fs / f0_nominal)
    p_min = max(2, int(p_nom * 0.7))
    p_max = min(len(corr)-1, int(p_nom * 1.4))
    if p_min >= p_max:
        return 0.0
    peak_idx = p_min + np.argmax(corr[p_min:p_max+1])
    r_peak = corr[peak_idx] / corr0
    if r_peak <= 0.001:
        return -30.0
    if r_peak >= 0.9999:
        return 40.0
    hnr = 10.0 * np.log10(r_peak / (1.0 - r_peak))
    return float(hnr)

def save_wav(path, data, fs):
    data = np.clip(data, -1.0, 1.0)
    int_data = (data * 32767.0).astype(np.int16)
    with wave.open(str(path), 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(fs)
        w.writeframes(int_data.tobytes())

# -----------------------------------------------------------------------------
# Independent DSP Primitive Re-derivation & Element-level Suites
# -----------------------------------------------------------------------------
def run_independent_dsp_derivations():
    results = {}
    
    max_deriv_err = 0.0
    max_contract_excess = 0.0
    
    def knee_val(u, h, s):
        sign = 1.0 if u >= 0 else -1.0
        return np.clip(0.9 * h * (1.0 + sign * s), 0.0, 0.95)
    
    def curve_val(u, h, s):
        k = knee_val(u, h, s)
        a = abs(u)
        if a <= k:
            return u
        t = (a - k) / (1.0 - k)
        f = (t - t**3 / 3.0) if t <= 1.0 else 2.0 / 3.0
        return math.copysign(k + (1.0 - k) * f, u)
    
    def prim_val(u, h, s):
        k = knee_val(u, h, s)
        a = abs(u)
        if a <= k:
            return 0.5 * u * u
        t = (a - k) / (1.0 - k)
        f = (0.5 * t * t - t**4 / 12.0) if t <= 1.0 else (2.0 / 3.0) * t - 0.25
        return 0.5 * k * k + (1.0 - k) * (k * t + (1.0 - k) * f)

    for h in [0.0, 0.4, 1.0]:
        for s in [-1.0, -0.3, 0.0, 0.7, 1.0]:
            for i in range(-2000, 2001):
                x = i / 1000.0
                eps = 1e-6
                num_deriv = (prim_val(x + eps, h, s) - prim_val(x - eps, h, s)) / (2.0 * eps)
                ana_deriv = curve_val(x, h, s)
                err = abs(num_deriv - ana_deriv)
                if err > max_deriv_err:
                    max_deriv_err = err
                excess = abs(ana_deriv) - abs(x)
                if excess > max_contract_excess:
                    max_contract_excess = excess

    results['primitive_derivative_max_error'] = max_deriv_err
    results['curve_magnitude_bound_excess'] = max_contract_excess

    hp_max_mag = 0.0
    for fs in [44100.0, 48000.0, 96000.0]:
        for fc in [20.0, 200.0, 2000.0]:
            a = math.exp(-2.0 * math.pi * fc / fs)
            w = np.linspace(0.0, math.pi, 2000)
            z = np.exp(-1j * w)
            h = 0.5 * (1.0 + a) * (1.0 - z) / (1.0 - a * z)
            mag = np.abs(h)
            peak = float(np.max(mag))
            if peak > hp_max_mag:
                hp_max_mag = peak

    results['hp_peak_magnitude'] = hp_max_mag

    dense_contract_violation = 0.0
    u_dense = np.linspace(-4.0, 4.0, 8001)
    for d in [1.0, 2.0, 8.0, 32.0]:
        for h in [0.0, 0.2, 0.5, 0.8, 1.0]:
            for s in [-1.0, -0.5, 0.0, 0.5, 1.0]:
                for u in u_dense:
                    cu = curve_val(u, h, s)
                    viol = abs(cu) - abs(u)
                    if viol > dense_contract_violation:
                        dense_contract_violation = viol
    results['t10_contractivity_max_violation'] = dense_contract_violation

    asym_steps = np.linspace(-1.0, 1.0, 11)
    asym_h2_h3 = {}
    even_odd_diff_at_zero = 0.0
    fs = 48000.0
    t = np.arange(int(fs * 2.0)) / fs
    sine_in = 0.5 * np.sin(2.0 * np.pi * 440.0 * t)
    
    for s in asym_steps:
        s_val = round(float(s), 2)
        out = np.array([curve_val(x * 2.0, 0.4, s_val) / 2.0 for x in sine_in])
        window = np.hanning(len(out))
        spec = np.abs(np.fft.rfft(out * window))
        
        idx1 = int(round(440.0 * len(out) / fs))
        idx2 = int(round(880.0 * len(out) / fs))
        idx3 = int(round(1320.0 * len(out) / fs))
        
        h1 = spec[idx1]
        h2 = spec[idx2]
        h3 = spec[idx3]
        
        h2_h3_db = 20.0 * np.log10(max(h2, 1e-12) / max(h3, 1e-12))
        asym_h2_h3[s_val] = float(h2_h3_db)
        if s_val == 0.0:
            even_odd_diff_at_zero = 20.0 * np.log10(max(h2, 1e-12) / max(h1, 1e-12))

    results['t10_asym_h2_h3'] = asym_h2_h3
    results['t10_even_attenuation_at_zero_db'] = float(even_odd_diff_at_zero)

    return results

# -----------------------------------------------------------------------------
# Suite T5: Retuned Tuning Verification
# -----------------------------------------------------------------------------
def run_t5_tuning_retest():
    """Evaluate before vs after tuning results across 72 impulse probes."""
    baseline_csv = PROTOTYPE_BASELINE_OUTPUT / "tuning-decay.csv"
    retuned_csv = PROTOTYPE_RETUNED_OUTPUT / "tuning-decay.csv"
    extreme_csv = PROTOTYPE_RETUNED_OUTPUT / "extreme-grid.csv"
    
    with baseline_csv.open(newline="") as f:
        baseline_rows = {r["name"]: r for r in csv.DictReader(f)}
    with retuned_csv.open(newline="") as f:
        retuned_rows = {r["name"]: r for r in csv.DictReader(f)}
        
    comparisons = []
    before_max = 0.0
    after_max = 0.0
    
    for name, r_after in retuned_rows.items():
        r_before = baseline_rows[name]
        c_before = abs(float(r_before["cents"])) if r_before["cents"] else None
        c_after = abs(float(r_after["cents"])) if r_after["cents"] else None
        if c_before is not None and c_before > before_max:
            before_max = c_before
        if c_after is not None and c_after > after_max:
            after_max = c_after
        comparisons.append({
            'name': name,
            'expected_hz': float(r_after['expected_hz']),
            'before_cents': float(r_before['cents']),
            'after_cents': float(r_after['cents']),
            'spectral_vs_analytic_cents': float(r_after['spectral_vs_analytic_cents']),
            'local_peak_detected': r_after['local_peak_detected'] == 'True'
        })
        
    # Analyze extreme grid
    extreme_found = 0
    extreme_unavail = 0
    if extreme_csv.exists():
        with extreme_csv.open(newline="") as f:
            for row in csv.DictReader(f):
                if row['peak_found'] == '1':
                    extreme_found += 1
                else:
                    extreme_unavail += 1

    # Check reproducibility between retuned and repeat
    repeat_matches = 0
    repeat_total = 0
    if PROTOTYPE_REPEAT_OUTPUT.exists():
        for p in PROTOTYPE_RETUNED_OUTPUT.glob("*.f32"):
            p_rep = PROTOTYPE_REPEAT_OUTPUT / p.name
            if p_rep.exists():
                repeat_total += 1
                if hashlib.sha256(p.read_bytes()).digest() == hashlib.sha256(p_rep.read_bytes()).digest():
                    repeat_matches += 1

    return {
        'total_probes': len(comparisons),
        'before_max_abs_cents': before_max,
        'after_max_abs_cents': after_max,
        'threshold_cents': 3.0,
        'pass_contract_c4': bool(after_max <= 3.0),
        'missing_peaks': sum(not c['local_peak_detected'] for c in comparisons),
        'extreme_grid_found': extreme_found,
        'extreme_grid_unavail': extreme_unavail,
        'repeat_reproducibility': f"{repeat_matches}/{repeat_total}",
        'probe_comparisons': comparisons
    }

def main():
    print("Running independent DSP re-derivations...")
    dsp_results = run_independent_dsp_derivations()
    print("DSP results:", json.dumps(dsp_results, indent=2))
    
    print("Running T5 tuning retest...")
    t5_results = run_t5_tuning_retest()
    print(f"T5 Retest: Before max = {t5_results['before_max_abs_cents']:.4f} ct, After max = {t5_results['after_max_abs_cents']:.6f} ct, Pass = {t5_results['pass_contract_c4']}")
    
    # Save combined measurement artifact
    output = {
        'dsp_rederivations': dsp_results,
        't5_tuning_retest': t5_results
    }
    
    with open(ARTIFACTS_DIR / "tuning_retest.json", "w") as f:
        json.dump(output, f, indent=2)
    print("Tuning retest measurements saved to", ARTIFACTS_DIR / "tuning_retest.json")

if __name__ == "__main__":
    main()
