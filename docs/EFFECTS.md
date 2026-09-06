# Experimental effects

The original chorus, delay and reverb remain available and unchanged by default. New effects are disabled in existing patches. SPACE contains their controls. Post-effects modular filters and the common output stage follow the effects.

## Resonant delay

After the original delay and before reverb, this stereo delay passes each feedback return through six contracting complex modes. Four ratio families provide harmonic, metallic-bar, membrane and vowel-like colours. Normalized positive mode weights sum to one, each pole radius is below one, and its input coefficient is one minus radius. The dry/modal blend is convex. Feedback is capped at 0.98 and the new-input coefficient follows the remaining loop loss. A bounded, gain-compensated soft saturator and an independent state guard protect the loop; the master limiter is not required. More feedback reduces fresh-input level intentionally.

Time glides over 40 ms, with independent left/right offsets up to 50 ms total. Sync resolves against the host tempo and a note division, then clamps to the 1–4000 ms delay range. Model changes glide modal frequencies without resetting phase. Dispersion stretches modal ratios rather than inserting a waveguide allpass. Last-note tracking references A3/MIDI 57 and excludes per-voice bend/glide/MPE; the effect is shared across voices. Tuning describes the internal mode frequencies, not a promise that the strongest repeat peak equals that frequency, since the delay also contributes a comb response.

The first repeat preserves the source; subsequent repeats acquire modal colour. Bypass fades over 15 ms then clears energy. Dry/wet uses a linear blend and the dry signal has no added delay. Modal Echo Pluck demonstrates the effect. Six focused tests passed: 1616330 checks, zero failures (`build/resdelay-tests.log`). They check exact disabled output, impulse timing and repeat gains, max-feedback bounds across three rates/all four models, audible differences after the first repeat, modal targets, chunk independence, transition shutdown, host audio/state and page rendering (`experimental/resonant-delay.png`). Full regression last passed at the preceding room checkpoint; final effect validation remains pending.


## Shimmer reverb

An independent eight-line reflection network follows the original reverb. Its outer feedback passes through two overlapping variable-delay grains per channel, then high/low cuts and a soft bound. Outer return gain is capped at 0.35; new input and return form a convex blend. The shared room core independently normalizes its excitation/feedback and guards its state. It does not need the final output limiter for tested stability.

The fixed grain window is 80 ms. Ratio changes glide over 30 ms; wraparound occurs where the corresponding grain window is zero. This produces a granular, sometimes chorus-like texture rather than transparent pitch correction. Wet reflection/grain delays are intentional; the dry path is immediate and no plug-in latency is added. A one-pole prefilter reduces high-frequency content before upward shifting; this is not a claim of full-band, alias-free resampling. Zero interval still includes the grain delay. Width affects the audible field and its feedback. Bypass fades and clears both reflection and pitch-shifter energy.

Controls include custom +/-24 semitones (0.01-semitone resolution), octave/fifth/fourth shortcuts, feedback, diffusion, damping, size, spread, low/high cut and mix. Fractional semitone values now display correctly without changing integer tuning labels.

Four focused tests passed with 1881052 checks (`build/shimmer-tests.log`). A coherent 200 Hz probe verified -12/+5/+7/+12 shifts across 44.1/48/96 kHz; measured fourth/fifth frequencies were within 0.022 Hz of target. This is a coherent reference, not a universal pitch-error specification for all grain/input relationships. Interval changes stayed continuous in that probe. Maximum feedback and extreme size/interval changes stayed finite and triggered no room-state guard. An enabled shifted-versus-unshifted tail measured squared difference 0.019733528, confirming the shifted feedback path reaches the output. Host audio, page rendering, custom-value display and session restore also pass. Screenshot: experimental/shimmer.png.


## Spectral freeze (host controls; dedicated page pending)

A fixed 2048-point FFT with 512-sample hops captures the post-effects stereo signal. FFT tables, analysis history, phase tracking, spectral bins and overlap-add buffers are preallocated. The radix-two FFT implementation has no transform-time allocation or lock. Enabled analyzes live audio; Hold or Capture takes the next complete frame and sustains its estimated bin frequencies. Release crossfades back to live audio. Blur redistributes captured spectral energy, Shift transposes bins and phase advances, Random Phase adds deterministic phase diffusion, and Decay applies a per-hop exponential envelope (zero means indefinite hold). Capture/Release are command edges excluded from randomization/morph interpolation. Captured audio is temporary and is not serialized.

The dry signal has no added delay. Capturing requires a complete 2048-sample history (46.4/42.7/21.3 ms at 44.1/48/96 kHz), then a frame boundary; transitions use a 20 ms wet ramp and overlapping synthesis windows. Spectral processing is a texture effect, not transparent pitch correction. The dedicated page is unfinished in this checkpoint; enable and operate it through the host's exposed parameters: Spectral Freeze Enabled, Spectral Hold, Capture Spectrum, Release Spectrum, Spectral Blur/Shift/Random Phase/Decay/Mix.

Four focused tests passed with 1339488 checks (`build/spectral-tests.log`). FFT known-bin and inverse tests pass; a 375 Hz reference holds at all three rates and its octave shift measures 750 Hz. Blur and decay produce measured audio changes, release restores exact live values, and repeated captures/transform changes cause zero intercepted C++ allocations or frees. This probe covers C++ new/delete on the test thread, not all C malloc calls or the entire synth. The remaining JUCE MPE/controller realtime audit is still pending.
