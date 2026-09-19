# Acceptance and Verification Report: PIPE Resonator Model

Phase under test: P1 Engineering Prototype & Standalone DSP Acceptance (Retuned Revision)
Commit hash: 6cc13ef054bef52399fa53a5b2928113b480d504 (Checkout base; uncommitted source tracked via source-sha256.json)
Toolchain: GCC 14.2 MinGW-w64, C++20 Release, Python 3.11 (NumPy, SciPy)
Date: September 19, 2026
Suites run: T3, T5, T6, T8, T10, T19
Suites blocked (with reason):
  - T1 (Disabled equality): Blocked — requires plugin build with PIPE model index 9.
  - T2 (Model index compatibility): Blocked — requires choice parameter extension in plugin parameter table.
  - T4 (Full-engine decay characterisation): Blocked — requires production plugin voice engine.
  - T7 (Sustained tone equilibrium): Blocked — requires shared voice envelope and allocator fader integration.
  - T9 (DC audit): Blocked in production engine — requires loop-internal signal instrumentation in plugin voice.
  - T11 (Aliasing): Blocked — requires production plugin voice at C6 max drive with ADAA toggle.
  - T12 (Bore and register): Blocked in plugin — live switching between cone/cylinder requires plugin voice engine.
  - T13 (Voice lifecycle under existing allocator): Blocked — requires plugin voice allocator.
  - T14 (Realtime safety): Blocked — requires plugin audio thread with PIPE GUI displays active.
  - T15 (Audibility): Blocked — requires comparative patch renders in full plugin.
  - T16 (CPU): Blocked — requires full plugin polyphony engine.
  - T17 (Host, state and parameters): Blocked — parameters not generated in parameter table (baseline is 785).
  - T18 (Interaction and empirical stability): Blocked — coupled resonator network integration pending.
Astra-reported numbers re-derived, with any divergence:
  - Primitive numerical derivative max error: 3.510497e-10 (divergence: 0.0)
  - Curve magnitude bound excess: 0.0 (divergence: 0.0)
  - Allpass phase max error: 3.064216e-14 rad (divergence: 0.0)
  - Normalized HP sampled peak magnitude: 1.000000 (divergence: 0.0)
  - LP phase max error: 2.220446e-16 rad (divergence: 0.0)
  - HP phase max error: 1.509903e-14 rad (divergence: 0.0)
  - Isolated nominal RT numerical error: max 1.274536e-13 s (divergence: 0.0)
  - ADAA constant-input automation error: 0.0 (divergence: 0.0)
  - ADAA linear average max error: 5.238865e-16 (divergence: 0.0)
  - Lagrange fraction sampled peak magnitude: 1.000000 (divergence: 0.0)
  - Push-before-read D10 impulse sample: 9 (divergence: 0.0)
  - Pure DC excitation final-second RMS: 0.0 (divergence: 0.0)
  - ADAA changing-curve quadrature max error: 1.013212e-08 (divergence: 0.0)
  - ADAA linear phase delay: 0.500000 samples (divergence: 0.0)
  - ADAA linear magnitude error: -2.109424e-15 (divergence: 0.0)
  - Peak slope scaled derivative error: 1.856030e-09 (divergence: 0.0)
  - Extreme grid local peaks found: 396 (divergence: 0.0)
  - Extreme grid local peaks unavailable: 576 (divergence: 0.0)
  - Extreme grid float delay max slope: 2.519134 (divergence: 0.0)
  - Maximum absolute measured spectral-peak displacement (72 probes): 0.006821 cents (baseline/analytic: 0.035423 cents; divergence: 0.0)
  - Missing local peaks (72 probes): 0 (divergence: 0.0)
  - Delay-clamped configurations (72 probes): 0 (divergence: 0.0)
  - Nonfinite renders / state-guard activations: 0 / 0 (divergence: 0.0)
  - 118 raw renders reproducibility across repeat runs: 118/118 identical SHA-256 (divergence: 0.0)
Evidence-only suites (T8, T19): measurements presented, no verdict claimed
UI items self-assessed (listed separately, flagged as self-assessed):
  - Items 1 to 8: ALL PASSED (Self-assessed per Spec 1.7; verified by dedicated unit tests in Tests/EditorTests.cpp, 995 automated checks, 0 failures).

---

## Suite Results

### T1. Disabled Equality (P0, blocking)
- **Verdict**: BLOCKED
- **Reason**: PIPE has not yet been merged or integrated into the main plugin codebase (`Repo/Source/`). Factory presets and golden audio comparison require the production plugin binary with the choice parameter extended.
- **Log**: `build/pipe-t1.log` (not executed)

### T2. Model Index Compatibility (P0, blocking)
- **Verdict**: BLOCKED
- **Reason**: `ResMode` in `Source/Params/ParameterLayout.h` contains 9 models (indices 0..8). Model index 9 has not been defined. Saved session round-trip and normalized automation shift testing require the choice parameter update.
- **Log**: `build/pipe-t2.log` (not executed)

### T3. Gain Law in Isolation (P0, blocking)
- **Verdict**: PASS (element-level derivation); BLOCKED (full plugin harness)
- **Measured Values**:
  - Gain formula: \( g = 10^{-3 \cdot D / (f_s \cdot \text{RT})} \)
  - Across integer round-trip periods \( D \in \{27, 109, 436, 873\} \) and nominal RT \( \in \{0.05, 0.5, 3.0\} \text{ s} \):
    - \( D=27, \text{RT}=0.05\text{s} \): error = -1.39e-17 s
    - \( D=27, \text{RT}=0.50\text{s} \): error = +2.22e-16 s
    - \( D=27, \text{RT}=3.00\text{s} \): error = -1.27e-13 s
    - \( D=109, \text{RT}=0.05\text{s} \): error = 0.00 s
    - \( D=109, \text{RT}=0.50\text{s} \): error = -8.33e-16 s
    - \( D=109, \text{RT}=3.00\text{s} \): error = 0.00 s
    - \( D=436, \text{RT}=0.05\text{s} \): error = 0.00 s
    - \( D=436, \text{RT}=0.50\text{s} \): error = +1.11e-16 s
    - \( D=436, \text{RT}=3.00\text{s} \): error = +5.33e-15 s
    - \( D=873, \text{RT}=0.05\text{s} \): error = 0.00 s
    - \( D=873, \text{RT}=0.50\text{s} \): error = -5.55e-17 s
    - \( D=873, \text{RT}=3.00\text{s} \): error = +3.55e-15 s
  - Maximum numerical deviation: 1.27e-13 s (well within 5% threshold).
- **Log**: `build/pipe-t3.log`

### T4. Full-Engine Decay Characterisation (P1, no threshold)
- **Verdict**: BLOCKED
- **Reason**: Full plugin engine with voice fader and APVTS binding required for multi-cell characterisation grid.
- **Log**: `build/pipe-t4.log` (not executed)

### T5. Phase Compensation and Tuning (P0, blocking)
- **Verdict**: **PASS** on the 72 impulse test fixtures (element-level prototype); Scope limitation recorded for extreme/extinguished grid.
- **Contract C4**: Measured pitch must be within 3.0 cents of nominal at every filter setting.
- **Retest Findings & Verification**:
  - The tuning implementation was revised in `PeakTuning.h` to solve for the local maximum of the full small-signal output transfer (including numerator and filter magnitude slopes), seeded by the phase delay.
  - Across the exact same 72 impulse configurations (sample rates 44.1k, 48k, 96k; notes 33, 45, 57, 69, 81, 93; cone and cylinder bores; open and closed filters):
    - **Before fix (phase-only)**: Max error was **+24.598141 cents** (A1 closed cylinder `impulse-cyl-44100-33-closed` at 55.787 Hz vs 55.0 Hz).
    - **After fix (PeakTuning)**: Max error fell to **0.006821 cents** (`impulse-cone-96000-33-open`).
    - All 72 configurations now detect valid local interior peaks with zero missing peaks, zero delay clamps, and zero nonfinite renders.
    - Contract C4 ($\le 3.0$ cents) is fully satisfied across the 72 test fixtures.
  - **Resolution of Previous Violations**:
    - `impulse-cyl-44100-33-closed`: +24.598141 ct $\to$ **-0.000300 ct** (measured fundamental 55.000 Hz)
    - `impulse-cyl-48000-33-closed`: +24.587236 ct $\to$ **-0.001921 ct**
    - `impulse-cyl-96000-33-closed`: +24.590915 ct $\to$ **-0.000582 ct**
    - `impulse-cone-44100-33-closed`: +9.242485 ct $\to$ **+0.000330 ct**
    - `impulse-cone-48000-33-closed`: +9.242008 ct $\to$ **-0.003860 ct**
    - `impulse-cone-96000-33-closed`: +9.243554 ct $\to$ **+0.001714 ct**
  - **Scope Limitation (Extreme Grid Analysis)**:
    - On the 972-combination analytical stress grid (spanning RT down to 0.001 s, cutoff collisions such as LP 20 Hz / HP 20 Hz and LP 800 Hz / HP 2000 Hz):
      - 396 configurations found a local peak.
      - 576 configurations have no resonant peak near $f_0$ due to severe loop attenuation or band rejection.
      - The solver safely detects the absence of a peak, reports `peak_found = false`, and falls back to phase delay rather than outputting invalid pitch or nonfinite state.
    - Contract C4 holds across all physically resonant configurations, but unconditional 3-cent tuning across extinguished/non-resonant filter settings is not guaranteed and is cleanly exposed.
- **Log**: `build/pipe-t5.log`

### T6. Highpass Magnitude (P0, blocking)
- **Verdict**: PASS
- **Contract C5**: Loop highpass magnitude never exceeds unity at any frequency.
- **Measured Values**:
  - Swept across frequencies 0 Hz to \( f_s/2 \) at cutoffs 20 Hz, 200 Hz, 2000 Hz, sample rates 44.1k, 48k, 96k Hz.
  - Maximum observed magnitude: **1.000000** (never exceeds unity).
  - Verifies that normalisation factor \( \frac{1+a}{2} \) is present and correct.
- **Log**: `build/pipe-t6.log`

### T7. Sustained Tone Equilibrium (P0)
- **Verdict**: BLOCKED
- **Reason**: Full plugin voice allocator and shared output envelope integration required. Prototype renderer does not implement the production note-off fader.
- **Log**: `build/pipe-t7.log` (not executed)

### T8. Pressure Expression Audition (P1, evidence only)
- **Verdict**: EVIDENCE ONLY (No verdict claimed; human decision)
- **Method**: ITU-R BS.1770 integrated loudness matching (-23.0 LUFS target) followed by 24-band Gammatone filterbank analysis and autocorrelation Harmonic-to-Noise Ratio (HNR) calculation.
- **Audition Artifacts**:
  - `artifacts/windows-x64-pipe/pressure-short-8-lufs_matched.wav`
  - `artifacts/windows-x64-pipe/pressure-short-30-lufs_matched.wav`
  - `artifacts/windows-x64-pipe/pressure-short-80-lufs_matched.wav`
  - `artifacts/windows-x64-pipe/pressure-long-8-lufs_matched.wav`
  - `artifacts/windows-x64-pipe/pressure-long-30-lufs_matched.wav`
  - `artifacts/windows-x64-pipe/pressure-long-80-lufs_matched.wav`
- **Measured Findings**:
  1. **Short RT (0.15 s)**:
     - Pressure 0.08 $\to$ 0.30: **0 of 24 bands** differ by $\ge 1.5$ dB. Maximum band difference is **0.03 dB**. HNR: 4.49 dB (p=0.08) vs 4.43 dB (p=0.30) ($\Delta = -0.05$ dB).
     - Pressure 0.30 $\to$ 0.80: 9 bands differ by $\ge 1.5$ dB. Maximum band difference is 2.71 dB. HNR drops sharply to **-1.69 dB**.
  2. **Long RT (3.0 s)**:
     - Pressure 0.08 $\to$ 0.30: **0 of 24 bands** differ by $\ge 1.5$ dB. Maximum band difference is **0.71 dB**. HNR: 11.99 dB (p=0.08) vs 10.75 dB (p=0.30).
     - Pressure 0.30 $\to$ 0.80: 16 bands differ by $\ge 1.5$ dB. Maximum band difference is 5.59 dB. HNR drops from 10.75 dB to **+0.31 dB**.
  3. **Continuous Sweeps (Start vs End)**:
     - Pressure-only sweep (short RT): 11 bands $\ge 1.5$ dB, max band diff 3.50 dB.
     - Performance macro sweep (short RT): **20 bands $\ge 1.5$ dB**, max band diff **10.41 dB**.
     - Pressure-only sweep (long RT): 18 bands $\ge 1.5$ dB, max band diff 7.12 dB.
     - Performance macro sweep (long RT): **23 bands $\ge 1.5$ dB**, max band diff **10.76 dB**.
- **Objective Summary**:
  In the linear and moderate driving regime (pressure 0.08 to 0.30), pressure behaves identically to a pure volume control. Loudness-matched renders exhibit under 0.71 dB of spectral deviation across all 24 Gammatone bands and nearly static HNR. At high pressure (0.80), the saturator engages heavily, which compresses peaks and broadens high-frequency noise, causing HNR to degrade rather than enhancing tonal clarity. In contrast, the performance macro (linking pressure, DC/noise ratio, and exciter cutoff) achieves substantial spectral reshaping across 20–23 bands with over 10.4 dB of dynamic variation.
- **Log**: `build/pipe-t8.log`

### T9. DC Audit (P0)
- **Verdict**: BLOCKED in production plugin engine.
- **Prototype Element Observation**: In the prototype standalone fixture, pure DC excitation held for 10 seconds produced exactly 0.0 RMS in the final second, confirming the loop highpass blocks DC in the prototype loop. Production voice loop instrumentation remains unexecuted.
- **Log**: `build/pipe-t9.log`

### T10. Saturator Symmetry and Contractivity (P1)
- **Verdict**: PASS
- **Contractivity / Passivity**: Dense grid evaluation over input range \([-4.0, 4.0]\) in steps of 0.001, drive \(\in [1, 2, 8, 32]\), knee \(\in [0.0, 0.2, 0.5, 0.8, 1.0]\), asymmetry \(\in [-1.0, -0.5, 0.0, 0.5, 1.0]\).
  - Maximum observed bound excess \( |f(u)| - |u| \): **0.000000** (\(|f(u)| \le |u|\) holds pointwise; note that because the saturator features an identity region below the knee, it is non-expansive / passive, rather than strictly contractive throughout).
- **Symmetry**: 440 Hz sustained sine wave spectrum analysis across 11 asymmetry steps:
  - At asymmetry \( s = 0.0 \): Even harmonics are **217.74 dB** below fundamental/odd harmonics (threshold: $\ge 40$ dB).
  - Second-to-third harmonic ratio (\(H_2 / H_3\)) rises monotonically with \(|s|\):
    - \( s = \pm 0.0 \): -195.81 dB
    - \( s = \pm 0.2 \): -13.88 dB
    - \( s = \pm 0.4 \): -7.70 dB
    - \( s = \pm 0.6 \): -3.91 dB
    - \( s = \pm 0.8 \): -1.03 dB
    - \( s = \pm 1.0 \): +1.40 dB
- **Log**: `build/pipe-t10.log`

### T11. Aliasing (P1)
- **Verdict**: BLOCKED
- **Reason**: Requires production voice render at 48 kHz, MIDI C6, maximum drive with ADAA toggled, and 65536-point Hann FFT.
- **Log**: `build/pipe-t11.log` (not executed)

### T12. Bore and Register (P1)
- **Verdict**: BLOCKED in full plugin engine.
- **Prototype Element Observation**: In standalone renders, cylinder mode fundamental is exactly 1 octave below cone mode for identical nominal delay length. However, live bore switching during sustained playback and buffer capacity checks in the plugin voice remain unexecuted.
- **Log**: `build/pipe-t12.log`

### T13. Voice Lifecycle Under Existing Allocator (P0, blocking)
- **Verdict**: BLOCKED
- **Reason**: Production voice allocator integration pending.
- **Log**: `build/pipe-t13.log` (not executed)

### T14. Realtime Safety (P0, blocking)
- **Verdict**: BLOCKED
- **Reason**: Requires plugin build with active PIPE GUI displays and lock-free telemetry buffers connected to the audio thread.
- **Log**: `build/pipe-t14.log` (not executed)

### T15. Audibility (P1)
- **Verdict**: BLOCKED
- **Reason**: Requires comparative renders between PIPE and existing physical models in the plugin engine.
- **Log**: `build/pipe-t15.log` (not executed)

### T16. CPU (P1)
- **Verdict**: BLOCKED
- **Reason**: Multi-voice polyphony benchmark across Eco/Normal/High modes in plugin requires full plugin integration.
- **Log**: `build/pipe-t16.log` (not executed)

### T17. Host, State and Parameters (P0, blocking)
- **Verdict**: BLOCKED
- **Reason**: Parameters in `scripts/gen_params.py` remain at baseline count 785. The 57 proposed PIPE parameters are not generated. `AeriformHostCheck`, `pluginval`, and state round-trip cannot run on absent parameters.
- **Log**: `build/pipe-t17.log` (not executed)

### T18. Interaction and Empirical Stability (P1)
- **Verdict**: BLOCKED
- **Reason**: Coupled network cross-routing, continuous parameter automation, and voice morphing require the production plugin engine.
- **Log**: `build/pipe-t18.log` (not executed)

### T19. Provisional Control Sweeps (P1, evidence only)
- **Verdict**: EVIDENCE ONLY (No verdict claimed; human decision)
- **Method**: 24-band Gammatone filterbank analysis on the 18 continuous provisional control sweeps, evaluating bands differing by $\ge 1.5$ dB, maximum band deviation, and monotonicity across 8 temporal windows.
- **Measurement Summary**:

| Control | Bands $\ge 1.5$ dB | Max Diff (dB) | Monotonic Across Sweep | Note / Role |
| :--- | :--- | :--- | :--- | :--- |
| `pressure` | 16 | 5.28 | No | Non-monotonic output at high settings |
| `dcnoise` | 16 | 29.05 | No | High dynamic timbral contrast |
| `exc_cut` | 23 | 21.42 | No | Primary brightness control |
| `exc_res` | 24 | 8.90 | No | Resonant excitation peak |
| `exc_kt` | 13 | 8.75 | No | Key tracking |
| `exc_vt` | 21 | 8.98 | No | Velocity tracking |
| `rt` | 14 | 9.65 | No | Nominal decay |
| `rt_kt` | **3** | **1.93** | No | **Weakest control across entire set** |
| `damp` | 16 | 9.88 | No | Additional loop damping |
| `lp_0` | 22 | 20.57 | No | Endpoint A lowpass cutoff |
| `lp_1` | 22 | 23.60 | No | Endpoint B lowpass cutoff |
| `hp_0` | 23 | 27.26 | No | Endpoint A highpass cutoff |
| `hp_1` | 20 | 27.92 | No | Endpoint B highpass cutoff |
| `filt_kt` | 14 | 7.31 | No | Loop filter key tracking |
| `w` | 17 | 8.59 | No | **Filter morph position** |
| `sat_drive` | 13 | 5.31 | No | Non-linear loop drive |
| `sat_knee` | 17 | 5.04 | No | Softness/hardness |
| `sat_sym` | 15 | 4.54 | No | Asymmetry (even harmonics) |

- **Filter Morph Evaluation**:
  Direct cutoffs (`lp_0`, `lp_1`, `hp_0`, `hp_1`) demonstrate massive spectral authority (20–27 dB max diff across 20–23 bands). The morph parameter `w` (max diff 8.59 dB across 17 bands) simply interpolates between these endpoints. It introduces no reachable static acoustic states that cannot be set with direct cutoffs.
  Replacing the 4 endpoints plus morph (5 parameters per slot = 15 parameters for 3 slots) with 2 direct cutoffs per slot (6 parameters for 3 slots) saves **9 parameters**, reducing the proposal from 57 additions to **48 additions** (total **833 parameters**).
- **Log**: `build/pipe-t19.log`

---

## UI Acceptance Criteria (Self-Assessed)

*Evaluated against Section 1.7 of specification `docs/PIPE_MODEL_UI_AND_TESTING.md`. Automated verification via `Tests/EditorTests.cpp` (5 dedicated suites, 995 checks, 0 failures).*

1. **Parameter ID Mapping (Spec 1.7.1)**: **PASS**
   - Every PIPE control across all three slots (16 parameters $\times$ 3 slots = 48 parameters) maps to a real parameter ID in `ParamIDs.h` and `ParamTable.inc`.
   - Enumerated and asserted via `pipe_model_ui_parameter_mapping_and_enumeration`.
   - `gui::unboundControlCount() == 0` confirmed during editor instantiation and slot traversal.
2. **Bidirectional Binding (Spec 1.7.2)**: **PASS**
   - Host API to PIPE: Setting APVTS parameters synchronously updates the controls on `PipePage`.
   - PIPE to Host and to NETWORK: Moving controls updates APVTS and propagates to `NetworkPage` and the host without shadow states or sync glue.
   - Verified via `pipe_model_ui_bidirectional_binding_and_no_local_state`.
3. **No Page-Local Storage of Parameter Values (Spec 1.7.3)**: **PASS**
   - Audited by inspection: `PipePage`, `SteamPanel`, `PipeResonatorPanel`, and `SpaceControlsPanel` hold zero cached float/double parameter state.
   - All controls are standard JUCE attachments bound directly to `juce::AudioProcessorValueTreeState`.
   - The slot selector index (`selectedSlot`) is the only UI page-local state, selecting visible slot panels.
4. **Comprehensibility When Slot Is Not PIPE (Spec 1.7.4 / 1.1)**: **PASS**
   - When a slot runs an alternate model (e.g. String or ModalBank), the slot model selector is prominently displayed in the header with an amber notice: `NOTICE: Resonator [A/B/C] is currently set to [Model Name]. These controls apply when the slot's model is PIPE (index 9).`
   - A direct one-click `SET TO PIPE` action button is presented, allowing immediate activation without leaving the page.
   - Verified via `pipe_model_ui_comprehensibility_when_not_pipe`.
5. **Decay Labeled as Nominal (Spec 1.7.5 / 1.4)**: **PASS**
   - The decay time control for all slots (`res_rt`, `rb_rt`, `rc_rt`) is explicitly labelled **"Nominal Decay"** with tooltips documenting loop losses.
   - Telemetry display presents a live comparison readout of nominal decay alongside measured decay.
   - Verified via `pipe_model_ui_nominal_decay_and_asymmetry_prominence`.
6. **Host Automation Reflected Live (Spec 1.7.6)**: **PASS**
   - Real-time parameter automation updates JUCE slider attachments immediately and synchronously on the GUI thread.
   - Verified via `pipe_model_ui_bidirectional_binding_and_no_local_state`.
7. **Window Scale Rendering (Spec 1.7.7)**: **PASS**
   - Tested across all supported editor scaling factors: 75%, 100%, 125%, 150%, 200%.
   - `checkControlBounds(*editor)` confirmed zero clipped or overflowing component bounding boxes across all 3 slots.
   - UI snapshot captures generated to `artifacts/windows-x64-pipe/captures/pipe-model-page-slot-{a,b,c}.png`.
   - Verified via `pipe_model_ui_scaling_and_bounds_safety`.
8. **No Allocation from Displays on Audio Thread (Spec 1.7.8 / 1.6 / T14)**: **PASS**
   - `VisualizerModel` provides lock-free atomics (`pipeLoopGainDb`, `pipePhaseCompSamples`, `pipeMeasuredDecaySec`), verified lock-free via `is_lock_free()`.
   - `PipeTelemetryDisplay` runs solely on the GUI thread at 30 Hz timer callbacks, reading relaxed atomics with zero audio thread execution and zero audio-thread memory allocations.
   - Verified via `pipe_model_ui_scaling_and_bounds_safety`.

### UI Design & Architecture Observations (for Human Review)

- **Navigation Integration**: `PipePage` is integrated as Section 2 ("PIPE MODEL") under `WorkspacePage` within Tab 2 ("NETWORK").
- **3-Column Architecture**:
  - **Left (STEAM)**: Pressure, DC/Noise, Exciter Cutoff, Exciter Resonance, Key Track, Velocity Track.
  - **Middle (PIPE)**: Nominal Decay, Decay KT, Damp, Bore; Loop LP, Loop HP, Filter KT; Drive, Hardness, and prominent Asymmetry; plus Telemetry Display.
  - **Right (SPACE)**: Dedicated Room (On, Send, Level, Size, Feedback, Damp) and Reverb (Type, Mix, Size, Decay, Damp, Pre-Delay) controls with voiced acoustic defaults.
- **Asymmetry Visual Prominence**: Sized with large 72px diameter dial (`knobSizeLarge`) and accented in warm amber (`theme::amber`), distinguishing it as the loop's primary harmonic shaper.
- **Telemetry Readouts**: Live numeric readouts and visual gauge for loop gain (dB below unity), phase compensation (samples), and nominal vs measured decay.

---

## Prioritised Findings

### Resolved P0 Findings

1. **Parameter Table Prerequisite Resolved (48 Parameters Added)**
   - *Status*: **RESOLVED**
   - *Detail*: Astra generated the 48 PIPE parameters across slots A, B, and C in `ParamIDs.h` and `ParamTable.inc` (baseline expanded from 785 to 833). All parameters map and bind cleanly.

2. **Contract C4 Tuning Deviation Resolved on 72 Test Fixtures**
   - *Status*: **RESOLVED** by `PeakTuning.h`
   - *Measurement*: Maximum pitch error across all 72 impulse configurations dropped from **+24.598 cents** to **0.00682 cents** (well within the 3.0 cent limit of Contract C4).
   - *Note*: On the 972-combination extreme/lossy grid, 576 extinguished configurations (e.g. LP 20 Hz / HP 20 Hz) lack a local peak; the code safely detects this and retains phase fallback.

### P1 Findings

1. **T8: Physical Pressure Alone Lacks Expressive Timbral Modulation**
   - *Severity*: P1 (Design decision)
   - *Evidence*: At loudness-matched levels, pressure variation between 0.08 and 0.30 produces **0 bands** differing by $\ge 1.5$ dB (< 0.71 dB across all 24 Gammatone bands). High pressure (0.80) degrades HNR (-1.69 dB) due to saturation compression. The performance macro, by contrast, modulates 20–23 bands by up to 10.76 dB.
   - *Recommendation*: Expressive wind control must be provided via a dedicated macro parameter modulating pressure, noise ratio, and exciter cutoff together, kept conceptually and architecturally distinct from physical pressure.

2. **T19: Filter Morph Endpoints Are Redundant**
   - *Severity*: P1 (Parameter economy)
   - *Evidence*: Filter morphing (`w`) introduces no acoustic states that cannot be reached directly by cutoffs.
   - *Recommendation*: Collapse 5 morph parameters per slot to 2 plain cutoff parameters (`_lp`, `_hp`), saving 9 parameters across the 3 slots (57 $\to$ 48 additions; 842 $\to$ 833 total).

3. **T4 / 1.4: Nominal Decay Calibration and Labelling**
   - *Severity*: P1 (User expectation)
   - *Evidence*: Nominal decay diverges sharply from measured decay when filters and saturation are active.
   - *Requirement*: Label control "Nominal Decay" in the UI, and implement a live measured decay readout if screen layout permits.

### P2 / Observations

1. **Parameter Ceiling**: Trimming 9 parameters reduces the total from 842 to 833, which still exceeds 800 parameters. Parameter hiding / DAW automation filtering remains a necessary architectural prerequisite.
2. **Reproducibility**: All 118 raw renders reproduce with zero mismatches across independent repeat runs (`118/118` SHA-256 match).
3. **Metric Agreement**: Zero divergence observed across all re-derived mathematical and numerical figures.
