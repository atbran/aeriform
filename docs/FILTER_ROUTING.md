# Movable filters

Three independent blocks provide low-pass, high-pass, band-pass, notch, morphing SVF, driven SVF, ladder-style low-pass, formant, comb, modal-bank and tilt responses. All default off. The original 358 parameter IDs retain their order; 36 filter parameters are appended.

The FILTERS page exposes all 20 insertion positions, with separate A/B/C loop positions. Filters at the same position run in block-number order. Exciter and folder positions run at exciter oversampling rate. Voice positions have independent state per voice; pre/post-effects positions operate once on the shared stereo sum. Modal resonators expose an input placement instead of a nonexistent waveguide delay loop.

Model, position, slope and vowel changes use two independent states with a 12 ms linear crossfade. Enable/mix are ramped. Continuous cutoff (logarithmic), resonance, drive and response morph use 12 ms control smoothing. Rapid structural changes finish the current transition before the next requested structure is applied. A block moving between two points briefly contributes at both positions; this is a local insert crossfade, not two complete parallel signal chains.

Feedback and loop placements normalize filter peak gain. This intentionally reduces sustain at strong resonance. Drive uses gain-compensated first-order antiderivative antialiasing. Ladder is a bounded two/four-pole feedback cascade, not a transistor circuit emulation. Comb delay is fractional and smoothed.

## Tuning measurements and limitations

The waveguide subtracts the added filter's fundamental phase delay from its delay line. This is phase-delay compensation, not group-delay compensation. Analytic SVF, ladder, tilt and comb responses include wet/dry and transition weights. Multiple inserts combine their complex responses.

Reference audible ring-down test: open pipe, notes C3/C4/C5 (MIDI 48/60/72), 44.1/48/96 kHz, cutoff 1/4 kHz, resonance 0.018 (Q about 0.707), no drive, feedback 0.99. The test requires less than 5 cents error. Separate sine-response measurements cover 126 combinations of sample rate, cutoff, resonance and seven filter models, requiring predicted phase delay within 0.03 samples.

This is not an exact-tuning guarantee across the control range. Strong filter loss can extinguish the fundamental before a late pitch measurement; high resonance can favor another mode. Exploratory late-window estimates in those cases were unreliable (up to about 90 cents away) and are not presented as valid tuning measurements. Driven filters use a small-signal ADAA phase approximation. Formant and modal-bank filters currently have no phase compensation. Comb phase wrapping, wet/dry cancellation, very low cutoff and the waveguide's minimum delay can also prevent exact compensation. Use these settings as experimental timbral effects.

## Verification

Tests compare enabled audio against disabled audio for every position, require bit-identical output for disabled-position changes, exercise every model, check position/model/enable transitions, measure phase and ring-down tuning, and verify deterministic reset after a transition. The page screenshot is `experimental/filters.png`.
