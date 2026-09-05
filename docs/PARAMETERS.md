# AERIFORM parameter reference

Generated from the parameter layout (`AeriformTests --params`).

## BREATH (v0.1 exciter, now Exciter model "Breath")

| ID | Name | Range | Default | Description |
|---|---|---|---|---|
| `exc_noise` | Noise | 0 % .. 100 % | 60 % | Level of the continuous noise (breath) excitation feeding the resonator. |
| `exc_noise_color` | Noise Color | 0 % .. 100 % | 35 % | Spectral tilt of the breath noise: white (0 %) to pink (100 %). |
| `exc_pressure` | Pressure | 0 % .. 100 % | 50 % | Steady air pressure (DC flow) pushed into the resonator. Drives sustained tones and flow-to-pitch effects. |
| `exc_pluck` | Pluck | 0 % .. 100 % | 0 % | Level of the short impulse burst at note-on. Turns the exciter into a pluck or strike. |
| `exc_pluck_len` | Pluck Length | 0.3 ms .. 80.0 ms | 5.0 ms | Duration of the pluck impulse burst. |
| `exc_lp` | Exciter LP | 200 Hz .. 20.0 kHz | 7.00 kHz | Low-pass filter applied to the excitation before it enters the tube. |
| `exc_hp` | Exciter HP | 10.0 Hz .. 5.00 kHz | 40.0 Hz | High-pass filter applied to the excitation. Removes rumble and shapes the breath character. |
| `exc_turb` | Turbulence | 0 % .. 100 % | 25 % | Slow, chaotic fluctuation of the air stream. Adds breathy instability and life. |
| `exc_vel` | Velocity | 0 % .. 100 % | 50 % | How strongly key velocity scales the excitation level and pluck strength. |
| `exc_ext_in` | External In | 0 % .. 100 % | 0 % | Amount of the plug-in audio input (sidechain / standalone input) injected as excitation. |
| `exc_keytrack` | Exciter Key Track | 0 % .. 100 % | 50 % | How far the exciter filters follow the played pitch. |
| `exc_attack_click` | Attack Transient | 0 % .. 100 % | 15 % | Extra tongue / chiff transient added at note-on. |
| `exc_release_noise` | Release Noise | 0 % .. 100 % | 10 % | Short breath puff emitted when a note is released. |
| `exc_breath_random` | Breath Random | 0 % .. 100 % | 15 % | Random per-note and slow drift of the breath pressure, like a human player. |
| `exc_reed` | Reed | 0 % .. 100 % | 0 % | Reed / jet non-linearity at the mouth of the tube. With Reed up, Pressure makes the pipe speak by itself (clarinet, sax, brass); at 0 the tube is driven linearly by noise and plucks. |
| `env_attack` | Attack | 0.5 ms .. 8.00 s | 25.0 ms | Time for the breath pressure to reach full level. |
| `env_decay` | Decay | 1.0 ms .. 8.00 s | 300 ms | Time for the pressure to fall from peak to the sustain level. |
| `env_sustain` | Sustain | 0 % .. 100 % | 80 % | Pressure level held while the key is down. |
| `env_release` | Release | 2.0 ms .. 12.00 s | 250 ms | Time for the pressure to fade after the key is released. |
| `env_vel_pressure` | Vel > Pressure | 0 % .. 100 % | 60 % | How much velocity scales the envelope peak (breath pressure). |
| `art_press_bright` | Pressure > Bright | 0 % .. 100 % | 40 % | Pressure-dependent brightness: blowing harder opens the exciter filter. |
| `art_flow_pitch` | Flow > Pitch | -100 % .. +100 % | +15 % | Flow-to-pitch interaction: air pressure bends the pitch slightly (+/- 50 cents at full). |
| `art_instability` | Instability | 0 % .. 100 % | 10 % | Slow random pitch wander of the resonator, like an unstable air column. |
| `art_variation` | Variation | 0 % .. 100 % | 20 % | Per-voice component variation: each voice gets slightly different tuning, damping and brightness. |
| `art_coupling` | Coupling | 0 % .. 100 % | 0 % | Sympathetic coupling: a little of every other voice leaks into each tube. |

## RESONATOR A

| ID | Name | Range | Default | Description |
|---|---|---|---|---|
| `res_coarse` | Coarse | -24 st .. +24 st | 0 st | Coarse tuning of the resonator in semitones. |
| `res_fine` | Fine | -100 ct .. +100 ct | 0 ct | Fine tuning in cents. |
| `res_length` | Length | 0.500 x .. 2.000 x | 1.000 x | Physical length multiplier of the tube. 1.0 x is in tune with the played note. |
| `res_keytrack` | Key Track | 0 % .. 200 % | 100 % | How much the tube length follows the keyboard. 100 % = equal temperament, 0 % = fixed drone. |
| `res_feedback` | Feedback | 0 % .. 100 % | 90 % | Loop gain of the waveguide. Above ~95 % the tube approaches self-oscillation (always bounded). |
| `res_damping` | Damping | 0 % .. 100 % | 35 % | Frequency-dependent loss: how quickly high harmonics die inside the tube. |
| `res_brightness` | Brightness | 0 % .. 100 % | 50 % | Spectral tilt of the energy injected into the tube. |
| `res_dispersion` | Dispersion | 0 % .. 100 % | 0 % | Inharmonicity / stiffness: spreads the partials like a metal bar or stiff string. |
| `res_shape` | Shape | 0 % .. 100 % | 50 % | Bore shape / excitation position along the tube. Creates comb-like formant colouring. |
| `res_reflect` | Reflection | 0 % .. 100 % | 30 % | End reflection character: hard closed end (0 %) to open, flared bell (100 %). |
| `res_saturation` | Saturation | 0 % .. 100 % | 25 % | Non-linear feedback saturation. Bounds the loop and adds warmth or growl at high feedback. |
| `res_mode` | Mode | Open Pipe / Closed Pipe / String / Comb / Dispersive Tube / Modal Bank / Metallic Bar / Membrane / Formant Body | Open Pipe | Resonator model: waveguide pipes and strings, comb, dispersive tube, modal bar / bell / membrane banks or a formant body. |
| `res_body_freq` | Body Freq | 80.0 Hz .. 8.00 kHz | 900 Hz | Centre frequency of the body / formant filter after the tube. |
| `res_body_res` | Body Res | 0 % .. 100 % | 40 % | Resonance of the body filter. |
| `res_body_mix` | Body Mix | 0 % .. 100 % | 30 % | Amount of the body filter mixed into the voice output. |
| `res_body_track` | Body Track | 0 % .. 100 % | 0 % | How much the body filter frequency follows the played note. |

## MOTION

| ID | Name | Range | Default | Description |
|---|---|---|---|---|
| `lfo1_shape` | LFO 1 Shape | Sine / Triangle / Saw Up / Saw Down / Square / Sample & Hold / Smooth Random | Sine | Waveform of the LFO. |
| `lfo1_rate` | LFO 1 Rate | 0.020 Hz .. 40.00 Hz | 0.500 Hz | LFO speed in Hz (when not tempo-synced). |
| `lfo1_sync` | LFO 1 Sync | off / on | off | Synchronise the LFO rate to host tempo. |
| `lfo1_div` | LFO 1 Division | 8/1 / 4/1 / 2/1 / 1/1 / 1/2 / 1/2 D / 1/2 T / 1/4 / 1/4 D / 1/4 T / 1/8 / 1/8 D / 1/8 T / 1/16 / 1/16 D / 1/16 T / 1/32 | 1/4 | Tempo-synced LFO period. |
| `lfo1_mode` | LFO 1 Mode | Free / Retrigger | Free | Free: continuous phase shared by all voices. Retrigger: restarts at every note-on. |
| `lfo1_fade` | LFO 1 Fade In | 0.0 ms .. 5.00 s | 0.0 ms | Time for the LFO depth to fade in after note-on. |
| `lfo1_phase` | LFO 1 Phase | 0 deg .. 360 deg | 0 deg | Start phase of the LFO when retriggered. |
| `lfo2_shape` | LFO 2 Shape | Sine / Triangle / Saw Up / Saw Down / Square / Sample & Hold / Smooth Random | Sine | Waveform of the LFO. |
| `lfo2_rate` | LFO 2 Rate | 0.020 Hz .. 40.00 Hz | 3.00 Hz | LFO speed in Hz (when not tempo-synced). |
| `lfo2_sync` | LFO 2 Sync | off / on | off | Synchronise the LFO rate to host tempo. |
| `lfo2_div` | LFO 2 Division | 8/1 / 4/1 / 2/1 / 1/1 / 1/2 / 1/2 D / 1/2 T / 1/4 / 1/4 D / 1/4 T / 1/8 / 1/8 D / 1/8 T / 1/16 / 1/16 D / 1/16 T / 1/32 | 1/4 | Tempo-synced LFO period. |
| `lfo2_mode` | LFO 2 Mode | Free / Retrigger | Free | Free: continuous phase shared by all voices. Retrigger: restarts at every note-on. |
| `lfo2_fade` | LFO 2 Fade In | 0.0 ms .. 5.00 s | 0.0 ms | Time for the LFO depth to fade in after note-on. |
| `lfo2_phase` | LFO 2 Phase | 0 deg .. 360 deg | 0 deg | Start phase of the LFO when retriggered. |
| `lfo3_shape` | LFO 3 Shape | Sine / Triangle / Saw Up / Saw Down / Square / Sample & Hold / Smooth Random | Sine | Waveform of the LFO. |
| `lfo3_rate` | LFO 3 Rate | 0.020 Hz .. 40.00 Hz | 0.100 Hz | LFO speed in Hz (when not tempo-synced). |
| `lfo3_sync` | LFO 3 Sync | off / on | off | Synchronise the LFO rate to host tempo. |
| `lfo3_div` | LFO 3 Division | 8/1 / 4/1 / 2/1 / 1/1 / 1/2 / 1/2 D / 1/2 T / 1/4 / 1/4 D / 1/4 T / 1/8 / 1/8 D / 1/8 T / 1/16 / 1/16 D / 1/16 T / 1/32 | 1/4 | Tempo-synced LFO period. |
| `lfo3_mode` | LFO 3 Mode | Free / Retrigger | Free | Free: continuous phase shared by all voices. Retrigger: restarts at every note-on. |
| `lfo3_fade` | LFO 3 Fade In | 0.0 ms .. 5.00 s | 0.0 ms | Time for the LFO depth to fade in after note-on. |
| `lfo3_phase` | LFO 3 Phase | 0 deg .. 360 deg | 0 deg | Start phase of the LFO when retriggered. |
| `menv_attack` | Mod Attack | 0.5 ms .. 8.00 s | 100.0 ms | Modulation envelope attack time. |
| `menv_decay` | Mod Decay | 1.0 ms .. 8.00 s | 600 ms | Modulation envelope decay time. |
| `menv_sustain` | Mod Sustain | 0 % .. 100 % | 20 % | Modulation envelope sustain level. |
| `menv_release` | Mod Release | 2.0 ms .. 12.00 s | 400 ms | Modulation envelope release time. |
| `mod1_src` | Mod 1 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod1_dst` | Mod 1 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod1_depth` | Mod 1 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod2_src` | Mod 2 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod2_dst` | Mod 2 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod2_depth` | Mod 2 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod3_src` | Mod 3 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod3_dst` | Mod 3 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod3_depth` | Mod 3 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod4_src` | Mod 4 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod4_dst` | Mod 4 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod4_depth` | Mod 4 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod5_src` | Mod 5 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod5_dst` | Mod 5 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod5_depth` | Mod 5 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod6_src` | Mod 6 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod6_dst` | Mod 6 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod6_depth` | Mod 6 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod7_src` | Mod 7 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod7_dst` | Mod 7 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod7_depth` | Mod 7 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod8_src` | Mod 8 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod8_dst` | Mod 8 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod8_depth` | Mod 8 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |

## SPACE

| ID | Name | Range | Default | Description |
|---|---|---|---|---|
| `chorus_mix` | Chorus Mix | 0 % .. 100 % | 0 % | Wet amount of the stereo ensemble chorus. |
| `chorus_rate` | Chorus Rate | 0.050 Hz .. 5.00 Hz | 0.400 Hz | Speed of the chorus modulation. |
| `chorus_depth` | Chorus Depth | 0 % .. 100 % | 40 % | Depth of the chorus pitch modulation. |
| `chorus_width` | Chorus Width | 0 % .. 100 % | 80 % | Stereo spread of the chorus voices. |
| `delay_mix` | Delay Mix | 0 % .. 100 % | 0 % | Wet amount of the delay. |
| `delay_time` | Delay Time | 10.0 ms .. 2.00 s | 375 ms | Delay time when not tempo-synced. |
| `delay_sync` | Delay Sync | off / on | on | Synchronise the delay time to host tempo. |
| `delay_div` | Delay Division | 8/1 / 4/1 / 2/1 / 1/1 / 1/2 / 1/2 D / 1/2 T / 1/4 / 1/4 D / 1/4 T / 1/8 / 1/8 D / 1/8 T / 1/16 / 1/16 D / 1/16 T / 1/32 | 1/8 D | Tempo-synced delay time. |
| `delay_feedback` | Delay Feedback | 0 % .. 95 % | 35 % | Amount of delayed signal fed back. |
| `delay_tone` | Delay Tone | 400 Hz .. 20.0 kHz | 4.50 kHz | Low-pass filter in the delay feedback path. |
| `delay_pingpong` | Ping Pong | off / on | on | Alternate the repeats between left and right. |
| `rev_mix` | Reverb Mix | 0 % .. 100 % | 18 % | Wet amount of the algorithmic reverb. |
| `rev_size` | Reverb Size | 0 % .. 100 % | 60 % | Size of the virtual space. |
| `rev_decay` | Reverb Decay | 0 % .. 100 % | 50 % | Decay time of the reverb tail. |
| `rev_damp` | Reverb Damping | 0 % .. 100 % | 40 % | High-frequency absorption of the space. |
| `rev_predelay` | Pre-Delay | 0.0 ms .. 200 ms | 12.0 ms | Delay before the reverb starts. |
| `rev_width` | Reverb Width | 0 % .. 100 % | 100 % | Stereo width of the reverb. |
| `rev_mod` | Reverb Motion | 0 % .. 100 % | 30 % | Slow modulation inside the reverb, smoothing metallic resonances. |

## MASTER

| ID | Name | Range | Default | Description |
|---|---|---|---|---|
| `voice_mode` | Voice Mode | Poly / Mono / Legato | Poly | Polyphonic, monophonic (retrigger) or legato (no retrigger while held). |
| `voice_count` | Voices | 1 .. 16 | 8 | Maximum number of simultaneous voices. |
| `glide_time` | Glide | 0.0 ms .. 2.00 s | 0.0 ms | Portamento time between notes. |
| `glide_legato` | Glide Legato | off / on | on | Only glide when notes overlap. |
| `unison_voices` | Unison | 1 .. 4 | 1 | Number of stacked, detuned tubes per note. |
| `unison_detune` | Unison Detune | 0 ct .. +100 ct | +12 ct | Detune spread between unison tubes. |
| `unison_spread` | Unison Spread | 0 % .. 100 % | 60 % | Stereo spread of unison tubes. |
| `bend_range` | Bend Range | 1 .. 24 | 2 | Pitch-bend range in semitones. |
| `mpe_enable` | MPE | off / on | off | Enable MIDI Polyphonic Expression (per-note pitch, pressure and slide). |
| `out_gain` | Output | -60.0 dB .. 12.0 dB | 0.0 dB | Master output level. |
| `out_hp` | Output HP | 10.0 Hz .. 400 Hz | 24.0 Hz | Final high-pass / DC blocker. |
| `limiter_on` | Limiter | off / on | on | Soft output limiter protecting against runaway levels. |

## EXCITERS

| ID | Name | Range | Default | Description |
|---|---|---|---|---|
| `exa_model` | Exciter A Model | Off / Breath / Wave / Complex / Noise: White / Noise: Pink / Noise: Brown / Noise: Blue / Noise: Violet / Noise: Band / Noise: Velvet / Noise: Crackle / Noise: Steam / Noise: Wind / Noise: Aerosol / Noise: Metallic / Reed / Lip / Bow / Jet / Mallet / Pluck / Scrape / Impact / Sidechain | Breath | Sound source of this exciter slot. Off saves CPU. Breath is the classic AERIFORM breath / pluck exciter. |
| `exa_level` | Exciter A Level | 0 % .. 100 % | 100 % | Output level of the exciter slot. |
| `exa_coarse` | Exciter A Coarse | -24 st .. +24 st | 0 st | Coarse tuning of the exciter in semitones (pitched models). |
| `exa_fine` | Exciter A Fine | -100 ct .. +100 ct | 0 ct | Fine tuning of the exciter in cents. |
| `exa_keytrack` | Exciter A Key Track | 0 % .. 200 % | 100 % | How much the exciter pitch follows the keyboard (100 % = equal temperament, 0 % = fixed). |
| `exa_retrig` | Exciter A Phase Mode | Free / Retrigger / Random | Retrigger | Free: phase continues across notes. Retrigger: restarts at the start phase. Random: random phase per note. |
| `exa_variation` | Exciter A Variation | 0 % .. 100 % | 10 % | Per-voice random offsets of tuning, tone and model character. |
| `exa_vel` | Exciter A Velocity | 0 % .. 100 % | 50 % | Velocity sensitivity of the exciter level and intensity. |
| `exa_press` | Exciter A Pressure | 0 % .. 100 % | 30 % | Aftertouch / MPE pressure response: raises level and model intensity. |
| `exa_drift` | Exciter A Drift | 0 % .. 100 % | 5 % | Slow random drift of pitch and tone. |
| `exa_phase` | Exciter A Start Phase | 0 deg .. 360 deg | 0 deg | Start phase when the phase mode is Retrigger. |
| `exa_tone` | Exciter A Tone | -100 % .. +100 % | 0 % | Spectral tilt of the exciter output: dark (-) to bright (+). |
| `exa_wave_shape` | Exciter A Wave Shape | 0 % .. 100 % | 0 % | Continuous morph: sine -> triangle -> saw -> square / pulse. |
| `exa_wave_pw` | Exciter A Pulse Width | 5 % .. 95 % | 50 % | Pulse width of the square / pulse region of the morph. |
| `exa_wave_sub` | Exciter A Sub | 0 % .. 100 % | 0 % | Level of the one-octave-down sub oscillator. |
| `exa_wave_pd` | Exciter A Phase Distortion | 0 % .. 100 % | 0 % | Phase distortion: warps the waveform read-out for resonant, bright timbres. |
| `exa_cx_complexity` | Exciter A Complexity | 0 % .. 100 % | 30 % | Depth of the phase coupling between the two orbit operators: simple tone -> dense spectrum. |
| `exa_cx_symmetry` | Exciter A Symmetry | -100 % .. +100 % | 0 % | Asymmetry of the orbit waveshaping: even harmonics and DC-free bias. |
| `exa_cx_bend` | Exciter A Bend | 0 % .. 100 % | 20 % | Bends the operator phase response, sharpening the waveform edges. |
| `exa_cx_instab` | Exciter A Instability | 0 % .. 100 % | 0 % | Random walk of the operator ratio and phase, from slight shimmer to wobbling breakdown. |
| `exa_cx_spread` | Exciter A Spread | 0 % .. 100 % | 0 % | Detunes a second operator pair for beating and thickness. |
| `exa_cx_warp` | Exciter A Phase Warp | 0 % .. 100 % | 0 % | Warps the second operator into the first at audio rate (formant-like colour). |
| `exa_cx_feedback` | Exciter A Feedback | 0 % .. 100 % | 20 % | Self-feedback of the first operator: saw-like at moderate values, noisy above. |
| `exa_cx_chaos` | Exciter A Chaos | 0 % .. 100 % | 0 % | Amount of a bounded chaotic map injected into the operator phases (deterministic per note). |
| `exa_cx_ratio` | Exciter A Ratio | 0.250 x .. 8.000 x | 2.000 x | Frequency ratio of the second operator to the first. |
| `exa_nz_color` | Exciter A Noise Color | -100 % .. +100 % | 0 % | Spectral tilt within the noise model: darker (-) to brighter (+). |
| `exa_nz_density` | Exciter A Density | 0 % .. 100 % | 50 % | Event density for sparse models (velvet, crackle, aerosol) and gust activity for wind. |
| `exa_nz_grain` | Exciter A Grain | 1.0 ms .. 100 ms | 20.0 ms | Grain / burst size of granular and crackle models. |
| `exa_nz_bandwidth` | Exciter A Bandwidth | 0 % .. 100 % | 50 % | Bandwidth of band-limited, metallic and steam models. |
| `exa_nz_center` | Exciter A Center | 50.0 Hz .. 12.0 kHz | 1000 Hz | Centre frequency of band-limited and metallic models (key-tracked by Key Track). |
| `exa_nz_correlation` | Exciter A Correlation | 0 % .. 100 % | 0 % | Blends between an independent noise stream per voice (0 %) and one stream shared by all voices (100 %). |
| `exa_nz_seed` | Exciter A Seed | 0 .. 999 | 0 | Random seed: the same seed gives the same noise for preset recall and offline renders. |
| `exa_nz_width` | Exciter A Width | 0 % .. 100 % | 50 % | Stereo spread: random per-voice pan offset of this exciter. |
| `exa_nz_burst` | Exciter A Burst | 1.0 ms .. 500 ms | 40.0 ms | Burst length of crackle, dust and gust events. |
| `exa_nz_burstenv` | Exciter A Burst Shape | 0 % .. 100 % | 50 % | Burst envelope: sharp attack / long tail (0 %) to slow swell (100 %). |
| `exa_nz_turb` | Exciter A Turbulence | 0 % .. 100 % | 30 % | Chaotic amplitude turbulence applied to the noise. |
| `exa_nz_gust` | Exciter A Gust Rate | 0.050 Hz .. 10.00 Hz | 0.500 Hz | Rate of wind gusts and slow spectral sweeps. |
| `exa_ph_stiffness` | Exciter A Stiffness | 0 % .. 100 % | 50 % | Reed stiffness / lip tension / bow pressure / mallet stiffness: the model's main restoring force. |
| `exa_ph_opening` | Exciter A Opening | 0 % .. 100 % | 50 % | Reed or lip opening, bow grip, jet aperture: how much steady flow gets through. |
| `exa_ph_position` | Exciter A Position | 0 % .. 100 % | 30 % | Bow / mallet / pluck position along the virtual string (comb colouring). |
| `exa_ph_speed` | Exciter A Speed | 0 % .. 100 % | 60 % | Bow velocity, jet speed or blowing pressure driving the model. |
| `exa_ph_turb` | Exciter A Turbulence | 0 % .. 100 % | 20 % | Breath / friction noise mixed into the physical model. |
| `exa_ph_hardness` | Exciter A Hardness | 0 % .. 100 % | 50 % | Mallet hardness, pluck sharpness, scrape roughness, impact tightness. |
| `exa_ph_bright` | Exciter A Brightness | 0 % .. 100 % | 50 % | Output brightness of the physical model. |
| `exa_sc_lp` | Exciter A Sidechain LP | 200 Hz .. 20.0 kHz | 20.0 kHz | Low-pass filter on the sidechain input. |
| `exa_sc_hp` | Exciter A Sidechain HP | 10.0 Hz .. 5.00 kHz | 20.0 Hz | High-pass filter on the sidechain input. |
| `exa_sc_follow` | Exciter A Envelope Follow | 0 % .. 100 % | 0 % | Shapes the exciter level with the sidechain's own envelope follower (0 % = raw audio). |
| `exa_sc_transient` | Exciter A Transients | 0 % .. 100 % | 0 % | Extracts and emphasises transients of the sidechain signal. |
| `exa_sc_freeze` | Exciter A Freeze | off / on | off | Freezes the last 250 ms of sidechain audio into a loop while on (not stored in presets). |
| `exb_model` | Exciter B Model | Off / Breath / Wave / Complex / Noise: White / Noise: Pink / Noise: Brown / Noise: Blue / Noise: Violet / Noise: Band / Noise: Velvet / Noise: Crackle / Noise: Steam / Noise: Wind / Noise: Aerosol / Noise: Metallic / Reed / Lip / Bow / Jet / Mallet / Pluck / Scrape / Impact / Sidechain | Off | Sound source of this exciter slot. Off saves CPU. Breath is the classic AERIFORM breath / pluck exciter. |
| `exb_level` | Exciter B Level | 0 % .. 100 % | 100 % | Output level of the exciter slot. |
| `exb_coarse` | Exciter B Coarse | -24 st .. +24 st | 0 st | Coarse tuning of the exciter in semitones (pitched models). |
| `exb_fine` | Exciter B Fine | -100 ct .. +100 ct | 0 ct | Fine tuning of the exciter in cents. |
| `exb_keytrack` | Exciter B Key Track | 0 % .. 200 % | 100 % | How much the exciter pitch follows the keyboard (100 % = equal temperament, 0 % = fixed). |
| `exb_retrig` | Exciter B Phase Mode | Free / Retrigger / Random | Retrigger | Free: phase continues across notes. Retrigger: restarts at the start phase. Random: random phase per note. |
| `exb_variation` | Exciter B Variation | 0 % .. 100 % | 10 % | Per-voice random offsets of tuning, tone and model character. |
| `exb_vel` | Exciter B Velocity | 0 % .. 100 % | 50 % | Velocity sensitivity of the exciter level and intensity. |
| `exb_press` | Exciter B Pressure | 0 % .. 100 % | 30 % | Aftertouch / MPE pressure response: raises level and model intensity. |
| `exb_drift` | Exciter B Drift | 0 % .. 100 % | 5 % | Slow random drift of pitch and tone. |
| `exb_phase` | Exciter B Start Phase | 0 deg .. 360 deg | 0 deg | Start phase when the phase mode is Retrigger. |
| `exb_tone` | Exciter B Tone | -100 % .. +100 % | 0 % | Spectral tilt of the exciter output: dark (-) to bright (+). |
| `exb_wave_shape` | Exciter B Wave Shape | 0 % .. 100 % | 0 % | Continuous morph: sine -> triangle -> saw -> square / pulse. |
| `exb_wave_pw` | Exciter B Pulse Width | 5 % .. 95 % | 50 % | Pulse width of the square / pulse region of the morph. |
| `exb_wave_sub` | Exciter B Sub | 0 % .. 100 % | 0 % | Level of the one-octave-down sub oscillator. |
| `exb_wave_pd` | Exciter B Phase Distortion | 0 % .. 100 % | 0 % | Phase distortion: warps the waveform read-out for resonant, bright timbres. |
| `exb_cx_complexity` | Exciter B Complexity | 0 % .. 100 % | 30 % | Depth of the phase coupling between the two orbit operators: simple tone -> dense spectrum. |
| `exb_cx_symmetry` | Exciter B Symmetry | -100 % .. +100 % | 0 % | Asymmetry of the orbit waveshaping: even harmonics and DC-free bias. |
| `exb_cx_bend` | Exciter B Bend | 0 % .. 100 % | 20 % | Bends the operator phase response, sharpening the waveform edges. |
| `exb_cx_instab` | Exciter B Instability | 0 % .. 100 % | 0 % | Random walk of the operator ratio and phase, from slight shimmer to wobbling breakdown. |
| `exb_cx_spread` | Exciter B Spread | 0 % .. 100 % | 0 % | Detunes a second operator pair for beating and thickness. |
| `exb_cx_warp` | Exciter B Phase Warp | 0 % .. 100 % | 0 % | Warps the second operator into the first at audio rate (formant-like colour). |
| `exb_cx_feedback` | Exciter B Feedback | 0 % .. 100 % | 20 % | Self-feedback of the first operator: saw-like at moderate values, noisy above. |
| `exb_cx_chaos` | Exciter B Chaos | 0 % .. 100 % | 0 % | Amount of a bounded chaotic map injected into the operator phases (deterministic per note). |
| `exb_cx_ratio` | Exciter B Ratio | 0.250 x .. 8.000 x | 2.000 x | Frequency ratio of the second operator to the first. |
| `exb_nz_color` | Exciter B Noise Color | -100 % .. +100 % | 0 % | Spectral tilt within the noise model: darker (-) to brighter (+). |
| `exb_nz_density` | Exciter B Density | 0 % .. 100 % | 50 % | Event density for sparse models (velvet, crackle, aerosol) and gust activity for wind. |
| `exb_nz_grain` | Exciter B Grain | 1.0 ms .. 100 ms | 20.0 ms | Grain / burst size of granular and crackle models. |
| `exb_nz_bandwidth` | Exciter B Bandwidth | 0 % .. 100 % | 50 % | Bandwidth of band-limited, metallic and steam models. |
| `exb_nz_center` | Exciter B Center | 50.0 Hz .. 12.0 kHz | 1000 Hz | Centre frequency of band-limited and metallic models (key-tracked by Key Track). |
| `exb_nz_correlation` | Exciter B Correlation | 0 % .. 100 % | 0 % | Blends between an independent noise stream per voice (0 %) and one stream shared by all voices (100 %). |
| `exb_nz_seed` | Exciter B Seed | 0 .. 999 | 0 | Random seed: the same seed gives the same noise for preset recall and offline renders. |
| `exb_nz_width` | Exciter B Width | 0 % .. 100 % | 50 % | Stereo spread: random per-voice pan offset of this exciter. |
| `exb_nz_burst` | Exciter B Burst | 1.0 ms .. 500 ms | 40.0 ms | Burst length of crackle, dust and gust events. |
| `exb_nz_burstenv` | Exciter B Burst Shape | 0 % .. 100 % | 50 % | Burst envelope: sharp attack / long tail (0 %) to slow swell (100 %). |
| `exb_nz_turb` | Exciter B Turbulence | 0 % .. 100 % | 30 % | Chaotic amplitude turbulence applied to the noise. |
| `exb_nz_gust` | Exciter B Gust Rate | 0.050 Hz .. 10.00 Hz | 0.500 Hz | Rate of wind gusts and slow spectral sweeps. |
| `exb_ph_stiffness` | Exciter B Stiffness | 0 % .. 100 % | 50 % | Reed stiffness / lip tension / bow pressure / mallet stiffness: the model's main restoring force. |
| `exb_ph_opening` | Exciter B Opening | 0 % .. 100 % | 50 % | Reed or lip opening, bow grip, jet aperture: how much steady flow gets through. |
| `exb_ph_position` | Exciter B Position | 0 % .. 100 % | 30 % | Bow / mallet / pluck position along the virtual string (comb colouring). |
| `exb_ph_speed` | Exciter B Speed | 0 % .. 100 % | 60 % | Bow velocity, jet speed or blowing pressure driving the model. |
| `exb_ph_turb` | Exciter B Turbulence | 0 % .. 100 % | 20 % | Breath / friction noise mixed into the physical model. |
| `exb_ph_hardness` | Exciter B Hardness | 0 % .. 100 % | 50 % | Mallet hardness, pluck sharpness, scrape roughness, impact tightness. |
| `exb_ph_bright` | Exciter B Brightness | 0 % .. 100 % | 50 % | Output brightness of the physical model. |
| `exb_sc_lp` | Exciter B Sidechain LP | 200 Hz .. 20.0 kHz | 20.0 kHz | Low-pass filter on the sidechain input. |
| `exb_sc_hp` | Exciter B Sidechain HP | 10.0 Hz .. 5.00 kHz | 20.0 Hz | High-pass filter on the sidechain input. |
| `exb_sc_follow` | Exciter B Envelope Follow | 0 % .. 100 % | 0 % | Shapes the exciter level with the sidechain's own envelope follower (0 % = raw audio). |
| `exb_sc_transient` | Exciter B Transients | 0 % .. 100 % | 0 % | Extracts and emphasises transients of the sidechain signal. |
| `exb_sc_freeze` | Exciter B Freeze | off / on | off | Freezes the last 250 ms of sidechain audio into a loop while on (not stored in presets). |
| `exb_sync` | Exciter B Sync | off / on | off | Hard-syncs Exciter B's phase to Exciter A (pitched models). |

## SHAPING (interaction, pre-shaper, wavefolder)

| ID | Name | Range | Default | Description |
|---|---|---|---|---|
| `mix_mode` | Interaction Mode | Crossfade / Add / Subtract / Ring / AM / FM / PM / Sync / XOR / Min / Max / Rectified Diff / Sample & Hold / Audio Crossfade | Crossfade | How Exciter A and B combine. With one slot Off the other passes through untouched. |
| `mix_interaction` | Interaction | 0 % .. 100 % | 50 % | Central interaction control; its meaning follows the mode (crossfade position, modulation index, sync/threshold...). |
| `mix_balance` | A/B Balance | -100 % .. +100 % | 0 % | Level balance between Exciter A (-) and Exciter B (+). |
| `mix_depth` | Interaction Depth | 0 % .. 100 % | 50 % | Depth of the interaction effect (wet amount of the combined signal versus the plain mix). |
| `mix_b2a` | B > A | 0 % .. 100 % | 0 % | Exciter B modulates Exciter A's pitch / phase (FM, PM and Sync modes) at audio rate. |
| `mix_a2b` | A > B | 0 % .. 100 % | 0 % | Exciter A modulates Exciter B's amplitude at audio rate. |
| `mix_dcblock` | DC Block | off / on | on | Removes DC from the combined exciter signal (ring, rectify and XOR modes create DC). |
| `mix_normalize` | Normalize | 0 % .. 100 % | 50 % | Automatic level normalisation of the combined signal so interaction modes stay comparable in loudness. |
| `mix_drive` | Pre-Fold Drive | 0 % .. 100 % | 0 % | Drive before the shaping / folding stages. |
| `pre_type` | Shaper Filter | Low + High / Band-Pass | Low + High | Filter topology of the pre-shaper: low-pass + high-pass (the classic exciter filters) or a band-pass. |
| `pre_res` | Shaper Resonance | 0 % .. 100 % | 0 % | Resonance of the pre-shaper filters. |
| `pre_drive` | Shaper Drive | 0 % .. 100 % | 0 % | Soft drive inside the pre-shaper. |
| `pre_bias` | Shaper Bias | -100 % .. +100 % | 0 % | Asymmetry: DC bias applied before the drive and removed afterwards (even harmonics). |
| `pre_slew` | Slew | 0 % .. 100 % | 0 % | Slew-rate limiting / edge smoothing of the excitation. |
| `pre_transient` | Transient Emphasis | 0 % .. 100 % | 0 % | Emphasises edges and attacks of the excitation (differentiator mix). |
| `pre_env` | Exciter Envelope | 0 % .. 100 % | 100 % | How much the breath envelope shapes the exciter level (100 % = classic breath behaviour, 0 % = constant while held). |
| `pre_order` | Shaper Order | Shape > Fold / Fold > Shape | Shape > Fold | Whether the pre-shaper filters run before or after the wavefolder. |
| `wf_on` | Wavefolder | off / on | off | Enables the oversampled wavefolder between the exciters and the resonator network. |
| `wf_fold` | Fold | 0 % .. 100 % | 30 % | Fold amount: gain into the folding function. Subtle harmonics at low values, metallic destruction at the top. |
| `wf_drive` | Fold Drive | 0 % .. 100 % | 20 % | Input drive before the folder. |
| `wf_symmetry` | Fold Symmetry | -100 % .. +100 % | 0 % | Different fold gain for the positive and negative half-waves. |
| `wf_bias` | Fold Bias | -100 % .. +100 % | 0 % | DC offset into the folder (removed afterwards): shifts which half folds first. |
| `wf_stages` | Fold Stages | 1 .. 4 | 1 | Number of cascaded fold stages. |
| `wf_mode` | Fold Mode | Smooth / Triangle / Sine / Diode / Chebyshev / Hard / Hybrid | Smooth | Folding function: smooth analogue, triangle, sine, diode (asymmetric), Chebyshev harmonic, hard digital or saturation / fold hybrid. |
| `wf_shape` | Fold Shape | 0 % .. 100 % | 50 % | Continuous shape control within the selected fold mode (knee softness, harmonic weighting, threshold spread). |
| `wf_mix` | Fold Mix | 0 % .. 100 % | 100 % | Wet / dry mix of the wavefolder. |
| `wf_comp` | Fold Compensation | 0 % .. 100 % | 100 % | Automatic output level compensation as fold and drive rise. |
| `wf_lp` | Fold Post LP | 200 Hz .. 20.0 kHz | 20.0 kHz | Low-pass filter after the folder. |
| `dyn_amount` | Dynamics | 0 % .. 100 % | 0 % | Level normaliser after the folder: keeps the excitation feeding the network at a consistent level. |

## RESONATOR A

| ID | Name | Range | Default | Description |
|---|---|---|---|---|
| `res_on` | Resonator A | off / on | on | Enables Resonator A. |
| `res_input` | A Input | 0 % .. 100 % | 100 % | Excitation level into Resonator A. |
| `res_output` | A Output | 0 % .. 100 % | 100 % | Output level of Resonator A. |
| `res_pan` | A Pan | -100 % .. +100 % | 0 % | Stereo position of Resonator A. |
| `res_width` | A Width | 0 % .. 100 % | 0 % | Stereo width: blends a second pickup tap into the opposite channel. |
| `res_pickup` | A Pickup | 0 % .. 100 % | 50 % | Position of the second pickup along the tube / mode set used for Width. |
| `res_inharm` | A Inharmonicity | 0 % .. 100 % | 0 % | Stretches the mode ratios of modal models (bar, bell, membrane). |
| `res_size` | A Size | 0 % .. 100 % | 50 % | Body size of modal / membrane / formant models (mode density and decay). |

## NETWORK (resonators B / C, routing, energy loop)

| ID | Name | Range | Default | Description |
|---|---|---|---|---|
| `rb_on` | Res B Enable | off / on | off | Enables Resonator B. |
| `rb_type` | Res B Type | Open Pipe / Closed Pipe / String / Comb / Dispersive Tube / Modal Bank / Metallic Bar / Membrane / Formant Body | Open Pipe | Resonator model of slot B. |
| `rb_input` | Res B Input | 0 % .. 100 % | 100 % | Excitation level into this resonator (parallel / injection modes). |
| `rb_output` | Res B Output | 0 % .. 100 % | 100 % | Output level of this resonator. |
| `rb_coarse` | Res B Coarse | -24 st .. +24 st | 0 st | Coarse tuning in semitones. |
| `rb_fine` | Res B Fine | -100 ct .. +100 ct | 0 ct | Fine tuning in cents. |
| `rb_ratio` | Res B Ratio | 0.250 x .. 4.000 x | 1.000 x | Frequency ratio relative to the played note (intervals, harmonics). |
| `rb_keytrack` | Res B Key Track | 0 % .. 200 % | 100 % | How much the resonator follows the keyboard. |
| `rb_feedback` | Res B Feedback | 0 % .. 100 % | 90 % | Loop gain / decay of the resonator (always bounded). |
| `rb_damping` | Res B Damping | 0 % .. 100 % | 35 % | Frequency-dependent loss. |
| `rb_brightness` | Res B Brightness | 0 % .. 100 % | 50 % | Spectral tilt of the injected energy. |
| `rb_dispersion` | Res B Dispersion | 0 % .. 100 % | 0 % | Allpass dispersion of waveguide models. |
| `rb_inharm` | Res B Inharmonicity | 0 % .. 100 % | 0 % | Stretches the mode ratios of modal models. |
| `rb_shape` | Res B Shape | 0 % .. 100 % | 50 % | Excitation position / bore shape. |
| `rb_reflect` | Res B Reflection | 0 % .. 100 % | 30 % | End reflection / boundary type. |
| `rb_saturation` | Res B Saturation | 0 % .. 100 % | 25 % | Non-linear loop saturation. |
| `rb_reed` | Res B Reed | 0 % .. 100 % | 0 % | Reed non-linearity at the mouth of this resonator (driven by the breath pressure). |
| `rb_size` | Res B Size | 0 % .. 100 % | 50 % | Body size of modal / membrane / formant models. |
| `rb_pickup` | Res B Pickup | 0 % .. 100 % | 50 % | Second pickup position used for Width. |
| `rb_pan` | Res B Pan | -100 % .. +100 % | -30 % | Stereo position of this resonator. |
| `rb_width` | Res B Width | 0 % .. 100 % | 0 % | Stereo width from the second pickup. |
| `rc_on` | Res C Enable | off / on | off | Enables Resonator C. |
| `rc_type` | Res C Type | Open Pipe / Closed Pipe / String / Comb / Dispersive Tube / Modal Bank / Metallic Bar / Membrane / Formant Body | Modal Bank | Resonator model of slot C. |
| `rc_input` | Res C Input | 0 % .. 100 % | 100 % | Excitation level into this resonator (parallel / injection modes). |
| `rc_output` | Res C Output | 0 % .. 100 % | 100 % | Output level of this resonator. |
| `rc_coarse` | Res C Coarse | -24 st .. +24 st | 0 st | Coarse tuning in semitones. |
| `rc_fine` | Res C Fine | -100 ct .. +100 ct | 0 ct | Fine tuning in cents. |
| `rc_ratio` | Res C Ratio | 0.250 x .. 4.000 x | 1.000 x | Frequency ratio relative to the played note (intervals, harmonics). |
| `rc_keytrack` | Res C Key Track | 0 % .. 200 % | 100 % | How much the resonator follows the keyboard. |
| `rc_feedback` | Res C Feedback | 0 % .. 100 % | 90 % | Loop gain / decay of the resonator (always bounded). |
| `rc_damping` | Res C Damping | 0 % .. 100 % | 35 % | Frequency-dependent loss. |
| `rc_brightness` | Res C Brightness | 0 % .. 100 % | 50 % | Spectral tilt of the injected energy. |
| `rc_dispersion` | Res C Dispersion | 0 % .. 100 % | 0 % | Allpass dispersion of waveguide models. |
| `rc_inharm` | Res C Inharmonicity | 0 % .. 100 % | 0 % | Stretches the mode ratios of modal models. |
| `rc_shape` | Res C Shape | 0 % .. 100 % | 50 % | Excitation position / bore shape. |
| `rc_reflect` | Res C Reflection | 0 % .. 100 % | 30 % | End reflection / boundary type. |
| `rc_saturation` | Res C Saturation | 0 % .. 100 % | 25 % | Non-linear loop saturation. |
| `rc_reed` | Res C Reed | 0 % .. 100 % | 0 % | Reed non-linearity at the mouth of this resonator (driven by the breath pressure). |
| `rc_size` | Res C Size | 0 % .. 100 % | 50 % | Body size of modal / membrane / formant models. |
| `rc_pickup` | Res C Pickup | 0 % .. 100 % | 50 % | Second pickup position used for Width. |
| `rc_pan` | Res C Pan | -100 % .. +100 % | +30 % | Stereo position of this resonator. |
| `rc_width` | Res C Width | 0 % .. 100 % | 0 % | Stereo width from the second pickup. |
| `net_mode` | Routing | Single / Serial / Parallel / Hybrid | Single | Single: exciter -> A. Serial: A -> B -> C. Parallel: all three side by side. Hybrid: A drives B and C. |
| `net_feedback` | Network Feedback | 0 % .. 100 % | 50 % | Global scale of all cross-feedback routes. |
| `net_ab` | Route A > B | 0 % .. 100 % | 0 % | Cross-feedback send from Resonator A into B. |
| `net_ba` | Route B > A | 0 % .. 100 % | 0 % | Cross-feedback send from Resonator B into A. |
| `net_bc` | Route B > C | 0 % .. 100 % | 0 % | Cross-feedback send from Resonator B into C. |
| `net_cb` | Route C > B | 0 % .. 100 % | 0 % | Cross-feedback send from Resonator C into B. |
| `net_ca` | Route C > A | 0 % .. 100 % | 0 % | Cross-feedback send from Resonator C into A. |
| `net_ac` | Route A > C | 0 % .. 100 % | 0 % | Cross-feedback send from Resonator A into C. |
| `net_send_ab` | Serial Send A > B | 0 % .. 100 % | 100 % | Serial / hybrid send level from A into B. |
| `net_send_bc` | Serial Send B > C | 0 % .. 100 % | 100 % | Serial send level from B into C (hybrid: A into C). |
| `net_inject_b` | Dry Inject B | 0 % .. 100 % | 0 % | Direct excitation injected into B in serial / hybrid modes. |
| `net_inject_c` | Dry Inject C | 0 % .. 100 % | 0 % | Direct excitation injected into C in serial / hybrid modes. |
| `net_polarity` | Feedback Polarity | Positive / Negative | Positive | Polarity of the cross-feedback routes. |
| `net_fb_delay` | Feedback Delay | 0.0 ms .. 50.0 ms | 0.0 ms | Extra delay in the cross-feedback routes (comb / echo character). |
| `net_fb_filter` | Feedback Filter | 200 Hz .. 20.0 kHz | 6.00 kHz | Low-pass filter in the cross-feedback routes. |
| `net_fb_drive` | Feedback Drive | 0 % .. 100 % | 30 % | Saturation drive of the cross-feedback routes (always bounded). |
| `net_damping` | Network Damping | 0 % .. 100 % | 20 % | Extra loss applied to all cross-feedback: tames runaway networks. |
| `net_width` | Network Width | 0 % .. 100 % | 50 % | Stereo spread of the resonator pans in parallel / hybrid modes. |
| `net_inject` | Injection Point | Res A / Res B / Res C / All | Res A | Which resonator(s) receive the excitation directly. |
| `net_tap` | Output Tap | Mix / Res A / Res B / Res C / Last | Mix | Which resonator output(s) feed the body and effects. |
| `net_mix` | Network Mix | 0 % .. 100 % | 100 % | Wet / dry: resonated signal versus the folded excitation. |
| `net_repipe` | Repipe | 0 % .. 100 % | 0 % | Macro: morphs from a single conventional resonator into a serial, cross-fed three-resonator network. |
| `loop_on` | Energy Loop | off / on | off | Feeds filtered resonator output back into the excitation chain (bounded, governed). Off by default. |
| `loop_amount` | Loop Return | 0 % .. 100 % | 30 % | Amount of resonator energy returned into the exciter chain. |
| `loop_source` | Loop Source | Mix / Res A / Res B / Res C | Mix | Which resonator output is returned. |
| `loop_dest` | Loop Destination | Shaper In / Folder In / Network In | Folder In | Where the returned energy is injected: pre-shaper input, folder input or network input. |
| `loop_filter` | Loop Filter | 100 Hz .. 12.0 kHz | 3.00 kHz | Low-pass filter in the return path. |
| `loop_delay` | Loop Delay | 0.0 ms .. 100 ms | 5.0 ms | Delay in the return path. |
| `loop_polarity` | Loop Polarity | Positive / Negative | Positive | Polarity of the returned signal. |
| `loop_sat` | Loop Saturation | 0 % .. 100 % | 50 % | Saturation of the return path (also its safety bound). |

## MOTION

| ID | Name | Range | Default | Description |
|---|---|---|---|---|
| `mod9_src` | Mod 9 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod9_dst` | Mod 9 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod9_depth` | Mod 9 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod10_src` | Mod 10 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod10_dst` | Mod 10 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod10_depth` | Mod 10 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod11_src` | Mod 11 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod11_dst` | Mod 11 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod11_depth` | Mod 11 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod12_src` | Mod 12 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod12_dst` | Mod 12 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod12_depth` | Mod 12 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod13_src` | Mod 13 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod13_dst` | Mod 13 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod13_depth` | Mod 13 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod14_src` | Mod 14 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod14_dst` | Mod 14 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod14_depth` | Mod 14 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod15_src` | Mod 15 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod15_dst` | Mod 15 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod15_depth` | Mod 15 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod16_src` | Mod 16 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 / Exciter A Env / Exciter B Env / Sidechain Env / Res A Energy / Res B Energy / Res C Energy / Network Energy / Sample & Hold / Smooth Random / Chaos X / Chaos Y / Note Age / Key Position / Voice Number / Alternate Note | None | Modulation source for this slot. |
| `mod16_dst` | Mod 16 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate / Ex A Level / Ex A Pitch / Ex A Tone / Ex A Shape / Ex A Chaos / Ex B Level / Ex B Pitch / Ex B Tone / Ex B Shape / Ex B Chaos / Interaction / A/B Balance / Shaper Drive / Fold / Fold Drive / Fold Symmetry / Fold Bias / Res B Pitch / Res B Feedback / Res B Damping / Res B Brightness / Res C Pitch / Res C Feedback / Res C Damping / Res C Brightness / Network Feedback / Network Width / Repipe / Loop Return / Res A Pan | None | Parameter modulated by this slot. |
| `mod16_depth` | Mod 16 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |

## MASTER

| ID | Name | Range | Default | Description |
|---|---|---|---|---|
| `quality` | Quality | Eco / Normal / High | Normal | Eco: 2x folder oversampling, 64-sample control rate. Normal: 2x, 32. High: 4x oversampling of the whole exciter chain. |
| `morph_on` | Morph Enabled | off / on | off | Enable A/B snapshot morphing. Controls edit the selected endpoint. |
| `morph_position` | Morph | 0 % .. 100 % | 0 % | Position between snapshot A and B. |
| `morph_mode` | Morph Engine | Parameter / Deep | Parameter | Parameter mode holds selected structural values. Deep mode crossfades two complete engines. |
| `random_mutation` | Mutation | 0 % .. 100 % | 15 % | Reproducible mutation radius around the current patch. |
| `random_wild` | Wild | off / on | off | Expand musical randomization ranges while retaining bounded feedback and protected administration. |
