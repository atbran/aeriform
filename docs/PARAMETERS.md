# AERIFORM parameter reference

Generated from the parameter layout (`AeriformTests --params`).

## BREATH

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

## RESONATOR

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
| `res_mode` | Mode | Open Pipe / Closed Pipe / String | Open Pipe | Resonator topology: open pipe (all harmonics), closed pipe (odd harmonics, reed-like) or string. |
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
| `mod1_src` | Mod 1 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 | None | Modulation source for this slot. |
| `mod1_dst` | Mod 1 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate | None | Parameter modulated by this slot. |
| `mod1_depth` | Mod 1 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod2_src` | Mod 2 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 | None | Modulation source for this slot. |
| `mod2_dst` | Mod 2 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate | None | Parameter modulated by this slot. |
| `mod2_depth` | Mod 2 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod3_src` | Mod 3 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 | None | Modulation source for this slot. |
| `mod3_dst` | Mod 3 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate | None | Parameter modulated by this slot. |
| `mod3_depth` | Mod 3 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod4_src` | Mod 4 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 | None | Modulation source for this slot. |
| `mod4_dst` | Mod 4 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate | None | Parameter modulated by this slot. |
| `mod4_depth` | Mod 4 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod5_src` | Mod 5 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 | None | Modulation source for this slot. |
| `mod5_dst` | Mod 5 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate | None | Parameter modulated by this slot. |
| `mod5_depth` | Mod 5 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod6_src` | Mod 6 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 | None | Modulation source for this slot. |
| `mod6_dst` | Mod 6 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate | None | Parameter modulated by this slot. |
| `mod6_depth` | Mod 6 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod7_src` | Mod 7 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 | None | Modulation source for this slot. |
| `mod7_dst` | Mod 7 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate | None | Parameter modulated by this slot. |
| `mod7_depth` | Mod 7 Depth | -100 % .. +100 % | 0 % | Bipolar modulation depth. Positive raises the destination, negative lowers it. |
| `mod8_src` | Mod 8 Source | None / LFO 1 / LFO 2 / LFO 3 / Mod Env / Amp Env / Velocity / Mod Wheel / Aftertouch / Pitch Bend / MPE Slide / Key Track / Random / Breath CC2 / Expression CC11 | None | Modulation source for this slot. |
| `mod8_dst` | Mod 8 Destination | None / Pressure / Noise / Noise Color / Exciter LP / Exciter HP / Turbulence / Pitch / Feedback / Damping / Brightness / Dispersion / Shape / Reflection / Body Freq / Body Mix / Pan / Amp / Chorus Mix / Delay Mix / Reverb Mix / LFO 1 Rate / LFO 2 Rate / LFO 3 Rate | None | Parameter modulated by this slot. |
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
