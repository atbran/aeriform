#!/usr/bin/env python3
"""Single source of truth for AERIFORM parameters.

Generates:
  Source/Params/ParamIDs.h     - enum class P, ids:: string constants, helpers
  Source/Params/ParamTable.inc - ParamDef table consumed by ParameterLayout.cpp

Rules: never rename an ID; only append to choice lists; new parameters must
default to the v0.1 behaviour so old presets and sessions sound identical.
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
rows = []

def F(enum, id_, name, sec, lo, hi, default, unit, fmt, tip, centre=None, step=0.0):
    rows.append(dict(enum=enum, id=id_, name=name, sec=sec, kind='Float', lo=lo, hi=hi, default=default,
                     centre=(centre if centre is not None else 0.0), step=step, unit=unit, fmt=fmt, choices='None', tip=tip))

def C(enum, id_, name, sec, choices, default, tip):
    rows.append(dict(enum=enum, id=id_, name=name, sec=sec, kind='Choice', lo=0, hi=0, default=default,
                     centre=0.0, step=1.0, unit='', fmt='Plain', choices=choices, tip=tip))

def B(enum, id_, name, sec, default, tip):
    rows.append(dict(enum=enum, id=id_, name=name, sec=sec, kind='Bool', lo=0, hi=1, default=(1 if default else 0),
                     centre=0.0, step=1.0, unit='', fmt='Plain', choices='None', tip=tip))

def I(enum, id_, name, sec, lo, hi, default, unit, tip):
    rows.append(dict(enum=enum, id=id_, name=name, sec=sec, kind='Int', lo=lo, hi=hi, default=default,
                     centre=0.0, step=1.0, unit=unit, fmt='Plain', choices='None', tip=tip))

# --------------------------------------------------------------------------
# v0.1 parameters (verbatim: ranges, defaults, names and tooltips unchanged)
# --------------------------------------------------------------------------
S = 'Breath'
F('excNoise', 'exc_noise', 'Noise', S, 0, 1, 0.6, '%', 'Percent', 'Level of the continuous noise (breath) excitation feeding the resonator.')
F('excNoiseColor', 'exc_noise_color', 'Noise Color', S, 0, 1, 0.35, '%', 'Percent', 'Spectral tilt of the breath noise: white (0 %) to pink (100 %).')
F('excPressure', 'exc_pressure', 'Pressure', S, 0, 1, 0.5, '%', 'Percent', 'Steady air pressure (DC flow) pushed into the resonator. Drives sustained tones and flow-to-pitch effects.')
F('excPluck', 'exc_pluck', 'Pluck', S, 0, 1, 0.0, '%', 'Percent', 'Level of the short impulse burst at note-on. Turns the exciter into a pluck or strike.')
F('excPluckLength', 'exc_pluck_len', 'Pluck Length', S, 0.3, 80, 5.0, 'ms', 'Ms', 'Duration of the pluck impulse burst.', centre=6.0)
F('excLowpass', 'exc_lp', 'Exciter LP', S, 200, 20000, 7000, 'Hz', 'Hz', 'Low-pass filter applied to the excitation before it enters the tube.', centre=3000)
F('excHighpass', 'exc_hp', 'Exciter HP', S, 10, 5000, 40, 'Hz', 'Hz', 'High-pass filter applied to the excitation. Removes rumble and shapes the breath character.', centre=250)
F('excTurbulence', 'exc_turb', 'Turbulence', S, 0, 1, 0.25, '%', 'Percent', 'Slow, chaotic fluctuation of the air stream. Adds breathy instability and life.')
F('excVelocity', 'exc_vel', 'Velocity', S, 0, 1, 0.5, '%', 'Percent', 'How strongly key velocity scales the excitation level and pluck strength.')
F('excExternalIn', 'exc_ext_in', 'External In', S, 0, 1, 0.0, '%', 'Percent', 'Amount of the plug-in audio input (sidechain / standalone input) injected as excitation.')
F('excKeyTrack', 'exc_keytrack', 'Exciter Key Track', S, 0, 1, 0.5, '%', 'Percent', 'How far the exciter filters follow the played pitch.')
F('excAttackClick', 'exc_attack_click', 'Attack Transient', S, 0, 1, 0.15, '%', 'Percent', 'Extra tongue / chiff transient added at note-on.')
F('excReleaseNoise', 'exc_release_noise', 'Release Noise', S, 0, 1, 0.1, '%', 'Percent', 'Short breath puff emitted when a note is released.')
F('excBreathRandom', 'exc_breath_random', 'Breath Random', S, 0, 1, 0.15, '%', 'Percent', 'Random per-note and slow drift of the breath pressure, like a human player.')
F('excReed', 'exc_reed', 'Reed', S, 0, 1, 0.0, '%', 'Percent', 'Reed / jet non-linearity at the mouth of the tube. With Reed up, Pressure makes the pipe speak by itself (clarinet, sax, brass); at 0 the tube is driven linearly by noise and plucks.')
F('envAttack', 'env_attack', 'Attack', S, 0.5, 8000, 25, 'ms', 'Ms', 'Time for the breath pressure to reach full level.', centre=200)
F('envDecay', 'env_decay', 'Decay', S, 1, 8000, 300, 'ms', 'Ms', 'Time for the pressure to fall from peak to the sustain level.', centre=300)
F('envSustain', 'env_sustain', 'Sustain', S, 0, 1, 0.8, '%', 'Percent', 'Pressure level held while the key is down.')
F('envRelease', 'env_release', 'Release', S, 2, 12000, 250, 'ms', 'Ms', 'Time for the pressure to fade after the key is released.', centre=400)
F('envVelToPressure', 'env_vel_pressure', 'Vel > Pressure', S, 0, 1, 0.6, '%', 'Percent', 'How much velocity scales the envelope peak (breath pressure).')
F('artPressBright', 'art_press_bright', 'Pressure > Bright', S, 0, 1, 0.4, '%', 'Percent', 'Pressure-dependent brightness: blowing harder opens the exciter filter.')
F('artFlowPitch', 'art_flow_pitch', 'Flow > Pitch', S, -1, 1, 0.15, '%', 'BipolarPercent', 'Flow-to-pitch interaction: air pressure bends the pitch slightly (+/- 50 cents at full).')
F('artInstability', 'art_instability', 'Instability', S, 0, 1, 0.1, '%', 'Percent', 'Slow random pitch wander of the resonator, like an unstable air column.')
F('artVariation', 'art_variation', 'Variation', S, 0, 1, 0.2, '%', 'Percent', 'Per-voice component variation: each voice gets slightly different tuning, damping and brightness.')
F('artCoupling', 'art_coupling', 'Coupling', S, 0, 1, 0.0, '%', 'Percent', 'Sympathetic coupling: a little of every other voice leaks into each tube.')

S = 'Resonator'
F('resCoarse', 'res_coarse', 'Coarse', S, -24, 24, 0, 'st', 'Semi', 'Coarse tuning of the resonator in semitones.', step=1.0)
F('resFine', 'res_fine', 'Fine', S, -100, 100, 0, 'ct', 'Cents', 'Fine tuning in cents.')
F('resLength', 'res_length', 'Length', S, 0.5, 2.0, 1.0, 'x', 'Ratio', 'Physical length multiplier of the tube. 1.0 x is in tune with the played note.', centre=1.0)
F('resKeyTrack', 'res_keytrack', 'Key Track', S, 0, 2, 1.0, '%', 'Percent', 'How much the tube length follows the keyboard. 100 % = equal temperament, 0 % = fixed drone.')
F('resFeedback', 'res_feedback', 'Feedback', S, 0, 1, 0.9, '%', 'Percent', 'Loop gain of the waveguide. Above ~95 % the tube approaches self-oscillation (always bounded).')
F('resDamping', 'res_damping', 'Damping', S, 0, 1, 0.35, '%', 'Percent', 'Frequency-dependent loss: how quickly high harmonics die inside the tube.')
F('resBrightness', 'res_brightness', 'Brightness', S, 0, 1, 0.5, '%', 'Percent', 'Spectral tilt of the energy injected into the tube.')
F('resDispersion', 'res_dispersion', 'Dispersion', S, 0, 1, 0.0, '%', 'Percent', 'Inharmonicity / stiffness: spreads the partials like a metal bar or stiff string.')
F('resShape', 'res_shape', 'Shape', S, 0, 1, 0.5, '%', 'Percent', 'Bore shape / excitation position along the tube. Creates comb-like formant colouring.')
F('resReflection', 'res_reflect', 'Reflection', S, 0, 1, 0.3, '%', 'Percent', 'End reflection character: hard closed end (0 %) to open, flared bell (100 %).')
F('resSaturation', 'res_saturation', 'Saturation', S, 0, 1, 0.25, '%', 'Percent', 'Non-linear feedback saturation. Bounds the loop and adds warmth or growl at high feedback.')
C('resMode', 'res_mode', 'Mode', S, 'ResTypes', 0, 'Resonator model: waveguide pipes and strings, comb, dispersive tube, modal bar / bell / membrane banks or a formant body.')
F('resBodyFreq', 'res_body_freq', 'Body Freq', S, 80, 8000, 900, 'Hz', 'Hz', 'Centre frequency of the body / formant filter after the tube.', centre=800)
F('resBodyRes', 'res_body_res', 'Body Res', S, 0, 1, 0.4, '%', 'Percent', 'Resonance of the body filter.')
F('resBodyMix', 'res_body_mix', 'Body Mix', S, 0, 1, 0.3, '%', 'Percent', 'Amount of the body filter mixed into the voice output.')
F('resBodyTrack', 'res_body_track', 'Body Track', S, 0, 1, 0.0, '%', 'Percent', 'How much the body filter frequency follows the played note.')

S = 'Motion'
for i in (1, 2, 3):
    n = 'LFO %d ' % i
    C('lfo%dShape' % i, 'lfo%d_shape' % i, n + 'Shape', S, 'LfoShapes', 0, 'Waveform of the LFO.')
    F('lfo%dRate' % i, 'lfo%d_rate' % i, n + 'Rate', S, 0.02, 40, {1: 0.5, 2: 3.0, 3: 0.1}[i], 'Hz', 'LfoHz', 'LFO speed in Hz (when not tempo-synced).', centre=2.0)
    B('lfo%dSync' % i, 'lfo%d_sync' % i, n + 'Sync', S, False, 'Synchronise the LFO rate to host tempo.')
    C('lfo%dDiv' % i, 'lfo%d_div' % i, n + 'Division', S, 'SyncDivs', 7, 'Tempo-synced LFO period.')
    C('lfo%dMode' % i, 'lfo%d_mode' % i, n + 'Mode', S, 'LfoModes', 0, 'Free: continuous phase shared by all voices. Retrigger: restarts at every note-on.')
    F('lfo%dFade' % i, 'lfo%d_fade' % i, n + 'Fade In', S, 0, 5000, 0, 'ms', 'Ms', 'Time for the LFO depth to fade in after note-on.', centre=500)
    F('lfo%dPhase' % i, 'lfo%d_phase' % i, n + 'Phase', S, 0, 360, 0, 'deg', 'Degrees', 'Start phase of the LFO when retriggered.')
F('menvAttack', 'menv_attack', 'Mod Attack', S, 0.5, 8000, 100, 'ms', 'Ms', 'Modulation envelope attack time.', centre=200)
F('menvDecay', 'menv_decay', 'Mod Decay', S, 1, 8000, 600, 'ms', 'Ms', 'Modulation envelope decay time.', centre=300)
F('menvSustain', 'menv_sustain', 'Mod Sustain', S, 0, 1, 0.2, '%', 'Percent', 'Modulation envelope sustain level.')
F('menvRelease', 'menv_release', 'Mod Release', S, 2, 12000, 400, 'ms', 'Ms', 'Modulation envelope release time.', centre=400)
for i in range(1, 9):
    n = 'Mod %d ' % i
    C('mod%dSrc' % i, 'mod%d_src' % i, n + 'Source', S, 'ModSources', 0, 'Modulation source for this slot.')
    C('mod%dDst' % i, 'mod%d_dst' % i, n + 'Destination', S, 'ModDests', 0, 'Parameter modulated by this slot.')
    F('mod%dDepth' % i, 'mod%d_depth' % i, n + 'Depth', S, -1, 1, 0, '%', 'BipolarPercent', 'Bipolar modulation depth. Positive raises the destination, negative lowers it.')

S = 'Space'
F('chorusMix', 'chorus_mix', 'Chorus Mix', S, 0, 1, 0.0, '%', 'Percent', 'Wet amount of the stereo ensemble chorus.')
F('chorusRate', 'chorus_rate', 'Chorus Rate', S, 0.05, 5, 0.4, 'Hz', 'LfoHz', 'Speed of the chorus modulation.', centre=0.5)
F('chorusDepth', 'chorus_depth', 'Chorus Depth', S, 0, 1, 0.4, '%', 'Percent', 'Depth of the chorus pitch modulation.')
F('chorusWidth', 'chorus_width', 'Chorus Width', S, 0, 1, 0.8, '%', 'Percent', 'Stereo spread of the chorus voices.')
F('delayMix', 'delay_mix', 'Delay Mix', S, 0, 1, 0.0, '%', 'Percent', 'Wet amount of the delay.')
F('delayTime', 'delay_time', 'Delay Time', S, 10, 2000, 375, 'ms', 'Ms', 'Delay time when not tempo-synced.', centre=300)
B('delaySync', 'delay_sync', 'Delay Sync', S, True, 'Synchronise the delay time to host tempo.')
C('delayDiv', 'delay_div', 'Delay Division', S, 'SyncDivs', 11, 'Tempo-synced delay time.')
F('delayFeedback', 'delay_feedback', 'Delay Feedback', S, 0, 0.95, 0.35, '%', 'Percent', 'Amount of delayed signal fed back.')
F('delayTone', 'delay_tone', 'Delay Tone', S, 400, 20000, 4500, 'Hz', 'Hz', 'Low-pass filter in the delay feedback path.', centre=3000)
B('delayPingPong', 'delay_pingpong', 'Ping Pong', S, True, 'Alternate the repeats between left and right.')
F('reverbMix', 'rev_mix', 'Reverb Mix', S, 0, 1, 0.18, '%', 'Percent', 'Wet amount of the algorithmic reverb.')
F('reverbSize', 'rev_size', 'Reverb Size', S, 0, 1, 0.6, '%', 'Percent', 'Size of the virtual space.')
F('reverbDecay', 'rev_decay', 'Reverb Decay', S, 0, 1, 0.5, '%', 'Percent', 'Decay time of the reverb tail.')
F('reverbDamping', 'rev_damp', 'Reverb Damping', S, 0, 1, 0.4, '%', 'Percent', 'High-frequency absorption of the space.')
F('reverbPreDelay', 'rev_predelay', 'Pre-Delay', S, 0, 200, 12, 'ms', 'Ms', 'Delay before the reverb starts.')
F('reverbWidth', 'rev_width', 'Reverb Width', S, 0, 1, 1.0, '%', 'Percent', 'Stereo width of the reverb.')
F('reverbModulation', 'rev_mod', 'Reverb Motion', S, 0, 1, 0.3, '%', 'Percent', 'Slow modulation inside the reverb, smoothing metallic resonances.')

S = 'Master'
C('voiceMode', 'voice_mode', 'Voice Mode', S, 'VoiceModes', 0, 'Polyphonic, monophonic (retrigger) or legato (no retrigger while held).')
I('voiceCount', 'voice_count', 'Voices', S, 1, 16, 8, '', 'Maximum number of simultaneous voices.')
F('glideTime', 'glide_time', 'Glide', S, 0, 2000, 0, 'ms', 'Ms', 'Portamento time between notes.', centre=200)
B('glideLegatoOnly', 'glide_legato', 'Glide Legato', S, True, 'Only glide when notes overlap.')
I('unisonVoices', 'unison_voices', 'Unison', S, 1, 4, 1, '', 'Number of stacked, detuned tubes per note.')
F('unisonDetune', 'unison_detune', 'Unison Detune', S, 0, 100, 12, 'ct', 'Cents', 'Detune spread between unison tubes.')
F('unisonSpread', 'unison_spread', 'Unison Spread', S, 0, 1, 0.6, '%', 'Percent', 'Stereo spread of unison tubes.')
I('bendRange', 'bend_range', 'Bend Range', S, 1, 24, 2, 'st', 'Pitch-bend range in semitones.')
B('mpeEnabled', 'mpe_enable', 'MPE', S, False, 'Enable MIDI Polyphonic Expression (per-note pitch, pressure and slide).')
F('outGain', 'out_gain', 'Output', S, -60, 12, 0, 'dB', 'Db', 'Master output level.')
F('outHighpass', 'out_hp', 'Output HP', S, 10, 400, 24, 'Hz', 'Hz', 'Final high-pass / DC blocker.', centre=60)
B('limiterOn', 'limiter_on', 'Limiter', S, True, 'Soft output limiter protecting against runaway levels.')

NUM_V01 = len(rows)

# --------------------------------------------------------------------------
# v2.1: exciter slots
# --------------------------------------------------------------------------
def exciter_slot(px, letter, default_model):
    S = 'Exciters'
    L = 'Exciter %s ' % letter
    e = lambda s: px + s[0].upper() + s[1:]
    C(e('model'), px + '_model', L + 'Model', S, 'ExciterModels', default_model, 'Sound source of this exciter slot. Off saves CPU. Breath is the classic AERIFORM breath / pluck exciter.')
    F(e('level'), px + '_level', L + 'Level', S, 0, 1, 1.0, '%', 'Percent', 'Output level of the exciter slot.')
    F(e('coarse'), px + '_coarse', L + 'Coarse', S, -24, 24, 0, 'st', 'Semi', 'Coarse tuning of the exciter in semitones (pitched models).', step=1.0)
    F(e('fine'), px + '_fine', L + 'Fine', S, -100, 100, 0, 'ct', 'Cents', 'Fine tuning of the exciter in cents.')
    F(e('keytrack'), px + '_keytrack', L + 'Key Track', S, 0, 2, 1.0, '%', 'Percent', 'How much the exciter pitch follows the keyboard (100 % = equal temperament, 0 % = fixed).')
    C(e('retrig'), px + '_retrig', L + 'Phase Mode', S, 'RetrigModes', 1, 'Free: phase continues across notes. Retrigger: restarts at the start phase. Random: random phase per note.')
    F(e('variation'), px + '_variation', L + 'Variation', S, 0, 1, 0.1, '%', 'Percent', 'Per-voice random offsets of tuning, tone and model character.')
    F(e('vel'), px + '_vel', L + 'Velocity', S, 0, 1, 0.5, '%', 'Percent', 'Velocity sensitivity of the exciter level and intensity.')
    F(e('press'), px + '_press', L + 'Pressure', S, 0, 1, 0.3, '%', 'Percent', 'Aftertouch / MPE pressure response: raises level and model intensity.')
    F(e('drift'), px + '_drift', L + 'Drift', S, 0, 1, 0.05, '%', 'Percent', 'Slow random drift of pitch and tone.')
    F(e('phase'), px + '_phase', L + 'Start Phase', S, 0, 360, 0, 'deg', 'Degrees', 'Start phase when the phase mode is Retrigger.')
    F(e('tone'), px + '_tone', L + 'Tone', S, -1, 1, 0, '%', 'BipolarPercent', 'Spectral tilt of the exciter output: dark (-) to bright (+).')
    # wave
    F(e('waveShape'), px + '_wave_shape', L + 'Wave Shape', S, 0, 1, 0, '%', 'Percent', 'Continuous morph: sine -> triangle -> saw -> square / pulse.')
    F(e('wavePw'), px + '_wave_pw', L + 'Pulse Width', S, 0.05, 0.95, 0.5, '%', 'Percent', 'Pulse width of the square / pulse region of the morph.')
    F(e('waveSub'), px + '_wave_sub', L + 'Sub', S, 0, 1, 0, '%', 'Percent', 'Level of the one-octave-down sub oscillator.')
    F(e('wavePd'), px + '_wave_pd', L + 'Phase Distortion', S, 0, 1, 0, '%', 'Percent', 'Phase distortion: warps the waveform read-out for resonant, bright timbres.')
    # complex
    F(e('cxComplexity'), px + '_cx_complexity', L + 'Complexity', S, 0, 1, 0.3, '%', 'Percent', 'Depth of the phase coupling between the two orbit operators: simple tone -> dense spectrum.')
    F(e('cxSymmetry'), px + '_cx_symmetry', L + 'Symmetry', S, -1, 1, 0, '%', 'BipolarPercent', 'Asymmetry of the orbit waveshaping: even harmonics and DC-free bias.')
    F(e('cxBend'), px + '_cx_bend', L + 'Bend', S, 0, 1, 0.2, '%', 'Percent', 'Bends the operator phase response, sharpening the waveform edges.')
    F(e('cxInstab'), px + '_cx_instab', L + 'Instability', S, 0, 1, 0, '%', 'Percent', 'Random walk of the operator ratio and phase, from slight shimmer to wobbling breakdown.')
    F(e('cxSpread'), px + '_cx_spread', L + 'Spread', S, 0, 1, 0, '%', 'Percent', 'Detunes a second operator pair for beating and thickness.')
    F(e('cxWarp'), px + '_cx_warp', L + 'Phase Warp', S, 0, 1, 0, '%', 'Percent', 'Warps the second operator into the first at audio rate (formant-like colour).')
    F(e('cxFeedback'), px + '_cx_feedback', L + 'Feedback', S, 0, 1, 0.2, '%', 'Percent', 'Self-feedback of the first operator: saw-like at moderate values, noisy above.')
    F(e('cxChaos'), px + '_cx_chaos', L + 'Chaos', S, 0, 1, 0, '%', 'Percent', 'Amount of a bounded chaotic map injected into the operator phases (deterministic per note).')
    F(e('cxRatio'), px + '_cx_ratio', L + 'Ratio', S, 0.25, 8, 2.0, 'x', 'Ratio', 'Frequency ratio of the second operator to the first.', centre=2.0)
    # noise
    F(e('nzColor'), px + '_nz_color', L + 'Noise Color', S, -1, 1, 0, '%', 'BipolarPercent', 'Spectral tilt within the noise model: darker (-) to brighter (+).')
    F(e('nzDensity'), px + '_nz_density', L + 'Density', S, 0, 1, 0.5, '%', 'Percent', 'Event density for sparse models (velvet, crackle, aerosol) and gust activity for wind.')
    F(e('nzGrain'), px + '_nz_grain', L + 'Grain', S, 1, 100, 20, 'ms', 'Ms', 'Grain / burst size of granular and crackle models.', centre=15)
    F(e('nzBandwidth'), px + '_nz_bandwidth', L + 'Bandwidth', S, 0, 1, 0.5, '%', 'Percent', 'Bandwidth of band-limited, metallic and steam models.')
    F(e('nzCenter'), px + '_nz_center', L + 'Center', S, 50, 12000, 1000, 'Hz', 'Hz', 'Centre frequency of band-limited and metallic models (key-tracked by Key Track).', centre=1000)
    F(e('nzCorrelation'), px + '_nz_correlation', L + 'Correlation', S, 0, 1, 0, '%', 'Percent', 'Blends between an independent noise stream per voice (0 %) and one stream shared by all voices (100 %).')
    I(e('nzSeed'), px + '_nz_seed', L + 'Seed', S, 0, 999, 0, '', 'Random seed: the same seed gives the same noise for preset recall and offline renders.')
    F(e('nzWidth'), px + '_nz_width', L + 'Width', S, 0, 1, 0.5, '%', 'Percent', 'Stereo spread: random per-voice pan offset of this exciter.')
    F(e('nzBurst'), px + '_nz_burst', L + 'Burst', S, 1, 500, 40, 'ms', 'Ms', 'Burst length of crackle, dust and gust events.', centre=40)
    F(e('nzBurstEnv'), px + '_nz_burstenv', L + 'Burst Shape', S, 0, 1, 0.5, '%', 'Percent', 'Burst envelope: sharp attack / long tail (0 %) to slow swell (100 %).')
    F(e('nzTurb'), px + '_nz_turb', L + 'Turbulence', S, 0, 1, 0.3, '%', 'Percent', 'Chaotic amplitude turbulence applied to the noise.')
    F(e('nzGust'), px + '_nz_gust', L + 'Gust Rate', S, 0.05, 10, 0.5, 'Hz', 'LfoHz', 'Rate of wind gusts and slow spectral sweeps.', centre=0.7)
    # physical
    F(e('phStiffness'), px + '_ph_stiffness', L + 'Stiffness', S, 0, 1, 0.5, '%', 'Percent', 'Reed stiffness / lip tension / bow pressure / mallet stiffness: the model\'s main restoring force.')
    F(e('phOpening'), px + '_ph_opening', L + 'Opening', S, 0, 1, 0.5, '%', 'Percent', 'Reed or lip opening, bow grip, jet aperture: how much steady flow gets through.')
    F(e('phPosition'), px + '_ph_position', L + 'Position', S, 0, 1, 0.3, '%', 'Percent', 'Bow / mallet / pluck position along the virtual string (comb colouring).')
    F(e('phSpeed'), px + '_ph_speed', L + 'Speed', S, 0, 1, 0.6, '%', 'Percent', 'Bow velocity, jet speed or blowing pressure driving the model.')
    F(e('phTurb'), px + '_ph_turb', L + 'Turbulence', S, 0, 1, 0.2, '%', 'Percent', 'Breath / friction noise mixed into the physical model.')
    F(e('phHardness'), px + '_ph_hardness', L + 'Hardness', S, 0, 1, 0.5, '%', 'Percent', 'Mallet hardness, pluck sharpness, scrape roughness, impact tightness.')
    F(e('phBright'), px + '_ph_bright', L + 'Brightness', S, 0, 1, 0.5, '%', 'Percent', 'Output brightness of the physical model.')
    # sidechain
    F(e('scLp'), px + '_sc_lp', L + 'Sidechain LP', S, 200, 20000, 20000, 'Hz', 'Hz', 'Low-pass filter on the sidechain input.', centre=4000)
    F(e('scHp'), px + '_sc_hp', L + 'Sidechain HP', S, 10, 5000, 20, 'Hz', 'Hz', 'High-pass filter on the sidechain input.', centre=200)
    F(e('scFollow'), px + '_sc_follow', L + 'Envelope Follow', S, 0, 1, 0, '%', 'Percent', 'Shapes the exciter level with the sidechain\'s own envelope follower (0 % = raw audio).')
    F(e('scTransient'), px + '_sc_transient', L + 'Transients', S, 0, 1, 0, '%', 'Percent', 'Extracts and emphasises transients of the sidechain signal.')
    B(e('scFreeze'), px + '_sc_freeze', L + 'Freeze', S, False, 'Freezes the last 250 ms of sidechain audio into a loop while on (not stored in presets).')

exciter_slot('exa', 'A', 1)   # Breath: identical to v0.1
exciter_slot('exb', 'B', 0)   # Off
B('exbSync', 'exb_sync', 'Exciter B Sync', 'Exciters', False, 'Hard-syncs Exciter B\'s phase to Exciter A (pitched models).')

# --------------------------------------------------------------------------
# v2.1: interaction, pre-shaper, wavefolder, dynamics
# --------------------------------------------------------------------------
S = 'Shaping'
C('mixMode', 'mix_mode', 'Interaction Mode', S, 'InteractionModes', 0, 'How Exciter A and B combine. With one slot Off the other passes through untouched.')
F('mixInteraction', 'mix_interaction', 'Interaction', S, 0, 1, 0.5, '%', 'Percent', 'Central interaction control; its meaning follows the mode (crossfade position, modulation index, sync/threshold...).')
F('mixBalance', 'mix_balance', 'A/B Balance', S, -1, 1, 0, '%', 'BipolarPercent', 'Level balance between Exciter A (-) and Exciter B (+).')
F('mixDepth', 'mix_depth', 'Interaction Depth', S, 0, 1, 0.5, '%', 'Percent', 'Depth of the interaction effect (wet amount of the combined signal versus the plain mix).')
F('mixB2A', 'mix_b2a', 'B > A', S, 0, 1, 0, '%', 'Percent', 'Exciter B modulates Exciter A\'s pitch / phase (FM, PM and Sync modes) at audio rate.')
F('mixA2B', 'mix_a2b', 'A > B', S, 0, 1, 0, '%', 'Percent', 'Exciter A modulates Exciter B\'s amplitude at audio rate.')
B('mixDcBlock', 'mix_dcblock', 'DC Block', S, True, 'Removes DC from the combined exciter signal (ring, rectify and XOR modes create DC).')
F('mixNormalize', 'mix_normalize', 'Normalize', S, 0, 1, 0.5, '%', 'Percent', 'Automatic level normalisation of the combined signal so interaction modes stay comparable in loudness.')
F('mixDrive', 'mix_drive', 'Pre-Fold Drive', S, 0, 1, 0, '%', 'Percent', 'Drive before the shaping / folding stages.')

C('preType', 'pre_type', 'Shaper Filter', S, 'PreFilterTypes', 0, 'Filter topology of the pre-shaper: low-pass + high-pass (the classic exciter filters) or a band-pass.')
F('preRes', 'pre_res', 'Shaper Resonance', S, 0, 1, 0, '%', 'Percent', 'Resonance of the pre-shaper filters.')
F('preDrive', 'pre_drive', 'Shaper Drive', S, 0, 1, 0, '%', 'Percent', 'Soft drive inside the pre-shaper.')
F('preBias', 'pre_bias', 'Shaper Bias', S, -1, 1, 0, '%', 'BipolarPercent', 'Asymmetry: DC bias applied before the drive and removed afterwards (even harmonics).')
F('preSlew', 'pre_slew', 'Slew', S, 0, 1, 0, '%', 'Percent', 'Slew-rate limiting / edge smoothing of the excitation.')
F('preTransient', 'pre_transient', 'Transient Emphasis', S, 0, 1, 0, '%', 'Percent', 'Emphasises edges and attacks of the excitation (differentiator mix).')
F('preEnv', 'pre_env', 'Exciter Envelope', S, 0, 1, 1.0, '%', 'Percent', 'How much the breath envelope shapes the exciter level (100 % = classic breath behaviour, 0 % = constant while held).')
C('preOrder', 'pre_order', 'Shaper Order', S, 'ShaperOrders', 0, 'Whether the pre-shaper filters run before or after the wavefolder.')

B('wfOn', 'wf_on', 'Wavefolder', S, False, 'Enables the oversampled wavefolder between the exciters and the resonator network.')
F('wfFold', 'wf_fold', 'Fold', S, 0, 1, 0.3, '%', 'Percent', 'Fold amount: gain into the folding function. Subtle harmonics at low values, metallic destruction at the top.')
F('wfDrive', 'wf_drive', 'Fold Drive', S, 0, 1, 0.2, '%', 'Percent', 'Input drive before the folder.')
F('wfSymmetry', 'wf_symmetry', 'Fold Symmetry', S, -1, 1, 0, '%', 'BipolarPercent', 'Different fold gain for the positive and negative half-waves.')
F('wfBias', 'wf_bias', 'Fold Bias', S, -1, 1, 0, '%', 'BipolarPercent', 'DC offset into the folder (removed afterwards): shifts which half folds first.')
I('wfStages', 'wf_stages', 'Fold Stages', S, 1, 4, 1, '', 'Number of cascaded fold stages.')
C('wfMode', 'wf_mode', 'Fold Mode', S, 'FoldModes', 0, 'Folding function: smooth analogue, triangle, sine, diode (asymmetric), Chebyshev harmonic, hard digital or saturation / fold hybrid.')
F('wfShape', 'wf_shape', 'Fold Shape', S, 0, 1, 0.5, '%', 'Percent', 'Continuous shape control within the selected fold mode (knee softness, harmonic weighting, threshold spread).')
F('wfMix', 'wf_mix', 'Fold Mix', S, 0, 1, 1.0, '%', 'Percent', 'Wet / dry mix of the wavefolder.')
F('wfComp', 'wf_comp', 'Fold Compensation', S, 0, 1, 1.0, '%', 'Percent', 'Automatic output level compensation as fold and drive rise.')
F('wfLp', 'wf_lp', 'Fold Post LP', S, 200, 20000, 20000, 'Hz', 'Hz', 'Low-pass filter after the folder.', centre=4000)
F('dynAmount', 'dyn_amount', 'Dynamics', S, 0, 1, 0, '%', 'Percent', 'Level normaliser after the folder: keeps the excitation feeding the network at a consistent level.')

# --------------------------------------------------------------------------
# v2.1: resonator A extras, resonators B and C, network, energy loop
# --------------------------------------------------------------------------
S = 'Resonator'
B('resOn', 'res_on', 'Resonator A', S, True, 'Enables Resonator A.')
F('resInput', 'res_input', 'A Input', S, 0, 1, 1.0, '%', 'Percent', 'Excitation level into Resonator A.')
F('resOutput', 'res_output', 'A Output', S, 0, 1, 1.0, '%', 'Percent', 'Output level of Resonator A.')
F('resPan', 'res_pan', 'A Pan', S, -1, 1, 0, '%', 'BipolarPercent', 'Stereo position of Resonator A.')
F('resWidth', 'res_width', 'A Width', S, 0, 1, 0, '%', 'Percent', 'Stereo width: blends a second pickup tap into the opposite channel.')
F('resPickup', 'res_pickup', 'A Pickup', S, 0, 1, 0.5, '%', 'Percent', 'Position of the second pickup along the tube / mode set used for Width.')
F('resInharm', 'res_inharm', 'A Inharmonicity', S, 0, 1, 0, '%', 'Percent', 'Stretches the mode ratios of modal models (bar, bell, membrane).')
F('resSize', 'res_size', 'A Size', S, 0, 1, 0.5, '%', 'Percent', 'Body size of modal / membrane / formant models (mode density and decay).')

def resonator_slot(px, letter, default_type, default_pan):
    S = 'Network'
    L = 'Res %s ' % letter
    e = lambda s: px + s[0].upper() + s[1:]
    B(e('on'), px + '_on', L + 'Enable', S, False, 'Enables Resonator %s.' % letter)
    C(e('type'), px + '_type', L + 'Type', S, 'ResTypes', default_type, 'Resonator model of slot %s.' % letter)
    F(e('input'), px + '_input', L + 'Input', S, 0, 1, 1.0, '%', 'Percent', 'Excitation level into this resonator (parallel / injection modes).')
    F(e('output'), px + '_output', L + 'Output', S, 0, 1, 1.0, '%', 'Percent', 'Output level of this resonator.')
    F(e('coarse'), px + '_coarse', L + 'Coarse', S, -24, 24, 0, 'st', 'Semi', 'Coarse tuning in semitones.', step=1.0)
    F(e('fine'), px + '_fine', L + 'Fine', S, -100, 100, 0, 'ct', 'Cents', 'Fine tuning in cents.')
    F(e('ratio'), px + '_ratio', L + 'Ratio', S, 0.25, 4, 1.0, 'x', 'Ratio', 'Frequency ratio relative to the played note (intervals, harmonics).', centre=1.0)
    F(e('keytrack'), px + '_keytrack', L + 'Key Track', S, 0, 2, 1.0, '%', 'Percent', 'How much the resonator follows the keyboard.')
    F(e('feedback'), px + '_feedback', L + 'Feedback', S, 0, 1, 0.9, '%', 'Percent', 'Loop gain / decay of the resonator (always bounded).')
    F(e('damping'), px + '_damping', L + 'Damping', S, 0, 1, 0.35, '%', 'Percent', 'Frequency-dependent loss.')
    F(e('brightness'), px + '_brightness', L + 'Brightness', S, 0, 1, 0.5, '%', 'Percent', 'Spectral tilt of the injected energy.')
    F(e('dispersion'), px + '_dispersion', L + 'Dispersion', S, 0, 1, 0, '%', 'Percent', 'Allpass dispersion of waveguide models.')
    F(e('inharm'), px + '_inharm', L + 'Inharmonicity', S, 0, 1, 0, '%', 'Percent', 'Stretches the mode ratios of modal models.')
    F(e('shape'), px + '_shape', L + 'Shape', S, 0, 1, 0.5, '%', 'Percent', 'Excitation position / bore shape.')
    F(e('reflect'), px + '_reflect', L + 'Reflection', S, 0, 1, 0.3, '%', 'Percent', 'End reflection / boundary type.')
    F(e('saturation'), px + '_saturation', L + 'Saturation', S, 0, 1, 0.25, '%', 'Percent', 'Non-linear loop saturation.')
    F(e('reed'), px + '_reed', L + 'Reed', S, 0, 1, 0, '%', 'Percent', 'Reed non-linearity at the mouth of this resonator (driven by the breath pressure).')
    F(e('size'), px + '_size', L + 'Size', S, 0, 1, 0.5, '%', 'Percent', 'Body size of modal / membrane / formant models.')
    F(e('pickup'), px + '_pickup', L + 'Pickup', S, 0, 1, 0.5, '%', 'Percent', 'Second pickup position used for Width.')
    F(e('pan'), px + '_pan', L + 'Pan', S, -1, 1, default_pan, '%', 'BipolarPercent', 'Stereo position of this resonator.')
    F(e('width'), px + '_width', L + 'Width', S, 0, 1, 0, '%', 'Percent', 'Stereo width from the second pickup.')

resonator_slot('rb', 'B', 0, -0.3)
resonator_slot('rc', 'C', 5, 0.3)   # Modal Bank

S = 'Network'
C('netMode', 'net_mode', 'Routing', S, 'NetModes', 0, 'Single: exciter -> A. Serial: A -> B -> C. Parallel: all three side by side. Hybrid: A drives B and C.')
F('netFeedback', 'net_feedback', 'Network Feedback', S, 0, 1, 0.5, '%', 'Percent', 'Global scale of all cross-feedback routes.')
F('netAB', 'net_ab', 'Route A > B', S, 0, 1, 0, '%', 'Percent', 'Cross-feedback send from Resonator A into B.')
F('netBA', 'net_ba', 'Route B > A', S, 0, 1, 0, '%', 'Percent', 'Cross-feedback send from Resonator B into A.')
F('netBC', 'net_bc', 'Route B > C', S, 0, 1, 0, '%', 'Percent', 'Cross-feedback send from Resonator B into C.')
F('netCB', 'net_cb', 'Route C > B', S, 0, 1, 0, '%', 'Percent', 'Cross-feedback send from Resonator C into B.')
F('netCA', 'net_ca', 'Route C > A', S, 0, 1, 0, '%', 'Percent', 'Cross-feedback send from Resonator C into A.')
F('netAC', 'net_ac', 'Route A > C', S, 0, 1, 0, '%', 'Percent', 'Cross-feedback send from Resonator A into C.')
F('netSendAB', 'net_send_ab', 'Serial Send A > B', S, 0, 1, 1.0, '%', 'Percent', 'Serial / hybrid send level from A into B.')
F('netSendBC', 'net_send_bc', 'Serial Send B > C', S, 0, 1, 1.0, '%', 'Percent', 'Serial send level from B into C (hybrid: A into C).')
F('netInjectB', 'net_inject_b', 'Dry Inject B', S, 0, 1, 0, '%', 'Percent', 'Direct excitation injected into B in serial / hybrid modes.')
F('netInjectC', 'net_inject_c', 'Dry Inject C', S, 0, 1, 0, '%', 'Percent', 'Direct excitation injected into C in serial / hybrid modes.')
C('netPolarity', 'net_polarity', 'Feedback Polarity', S, 'Polarities', 0, 'Polarity of the cross-feedback routes.')
F('netFbDelay', 'net_fb_delay', 'Feedback Delay', S, 0, 50, 0, 'ms', 'Ms', 'Extra delay in the cross-feedback routes (comb / echo character).')
F('netFbFilter', 'net_fb_filter', 'Feedback Filter', S, 200, 20000, 6000, 'Hz', 'Hz', 'Low-pass filter in the cross-feedback routes.', centre=3000)
F('netFbDrive', 'net_fb_drive', 'Feedback Drive', S, 0, 1, 0.3, '%', 'Percent', 'Saturation drive of the cross-feedback routes (always bounded).')
F('netDamping', 'net_damping', 'Network Damping', S, 0, 1, 0.2, '%', 'Percent', 'Extra loss applied to all cross-feedback: tames runaway networks.')
F('netWidth', 'net_width', 'Network Width', S, 0, 1, 0.5, '%', 'Percent', 'Stereo spread of the resonator pans in parallel / hybrid modes.')
C('netInject', 'net_inject', 'Injection Point', S, 'InjectPoints', 0, 'Which resonator(s) receive the excitation directly.')
C('netTap', 'net_tap', 'Output Tap', S, 'OutputTaps', 0, 'Which resonator output(s) feed the body and effects.')
F('netMix', 'net_mix', 'Network Mix', S, 0, 1, 1.0, '%', 'Percent', 'Wet / dry: resonated signal versus the folded excitation.')
F('netRepipe', 'net_repipe', 'Repipe', S, 0, 1, 0, '%', 'Percent', 'Macro: morphs from a single conventional resonator into a serial, cross-fed three-resonator network.')

B('loopOn', 'loop_on', 'Energy Loop', S, False, 'Feeds filtered resonator output back into the excitation chain (bounded, governed). Off by default.')
F('loopAmount', 'loop_amount', 'Loop Return', S, 0, 1, 0.3, '%', 'Percent', 'Amount of resonator energy returned into the exciter chain.')
C('loopSource', 'loop_source', 'Loop Source', S, 'LoopSources', 0, 'Which resonator output is returned.')
C('loopDest', 'loop_dest', 'Loop Destination', S, 'LoopDests', 1, 'Where the returned energy is injected: pre-shaper input, folder input or network input.')
F('loopFilter', 'loop_filter', 'Loop Filter', S, 100, 12000, 3000, 'Hz', 'Hz', 'Low-pass filter in the return path.', centre=2000)
F('loopDelay', 'loop_delay', 'Loop Delay', S, 0, 100, 5, 'ms', 'Ms', 'Delay in the return path.')
C('loopPolarity', 'loop_polarity', 'Loop Polarity', S, 'Polarities', 0, 'Polarity of the returned signal.')
F('loopSat', 'loop_sat', 'Loop Saturation', S, 0, 1, 0.5, '%', 'Percent', 'Saturation of the return path (also its safety bound).')

# --------------------------------------------------------------------------
# v2.1: matrix slots 9..16, quality
# --------------------------------------------------------------------------
S = 'Motion'
for i in range(9, 17):
    n = 'Mod %d ' % i
    C('mod%dSrc' % i, 'mod%d_src' % i, n + 'Source', S, 'ModSources', 0, 'Modulation source for this slot.')
    C('mod%dDst' % i, 'mod%d_dst' % i, n + 'Destination', S, 'ModDests', 0, 'Parameter modulated by this slot.')
    F('mod%dDepth' % i, 'mod%d_depth' % i, n + 'Depth', S, -1, 1, 0, '%', 'BipolarPercent', 'Bipolar modulation depth. Positive raises the destination, negative lowers it.')

C('quality', 'quality', 'Quality', 'Master', 'QualityModes', 1, 'Eco: 2x folder oversampling, 64-sample control rate. Normal: 2x, 32. High: 4x oversampling of the whole exciter chain.')

# --------------------------------------------------------------------------
# validation + emit
# --------------------------------------------------------------------------
# Experimental additions are appended; every existing index remains stable.
B('morphOn','morph_on','Morph Enabled','Master',False,'Enable A/B snapshot morphing. Controls edit the selected endpoint.')
F('morphPosition','morph_position','Morph','Master',0,1,0,'%', 'Percent','Position between snapshot A and B.')
C('morphMode','morph_mode','Morph Engine','Master','MorphModes',0,'Parameter mode holds selected structural values. Deep mode crossfades two complete engines.')
F('randomMutation','random_mutation','Mutation','Master',0,1,0.15,'%', 'Percent','Reproducible mutation radius around the current patch.')
B('randomWild','random_wild','Wild','Master',False,'Expand musical randomization ranges while retaining bounded feedback and protected administration.')

from network_params import register as register_network
register_network(F,C,B,I)

ids = [r['id'] for r in rows]
enums = [r['enum'] for r in rows]
assert len(set(ids)) == len(ids), 'duplicate id'
assert len(set(enums)) == len(enums), 'duplicate enum'
for r in rows:
    assert re.match(r'^[a-z][a-z0-9_]*$', r['id']), r['id']

def cstr(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'

def fnum(x):
    return ('%.6g' % float(x)) + ('f' if '.' in ('%.6g' % float(x)) or 'e' in ('%.6g' % float(x)) else '.0f')

h = []
h.append('#pragma once\n')
h.append('// GENERATED by scripts/gen_params.py - do not edit by hand. Stable parameter IDs: never rename.\n')
h.append('#include <juce_core/juce_core.h>\n\nnamespace aeriform\n{\n')
h.append('/** Every parameter, in layout order. Index == position in parameterInfos(). */\n')
h.append('enum class P : int\n{\n')
for r in rows:
    h.append('    %s,\n' % r['enum'])
h.append('    Count\n};\n\n')
h.append('inline constexpr int kNumParams = (int) P::Count;\n')
h.append('inline constexpr int kNumParamsV01 = %d;   // parameters that existed in v0.1\n\n' % NUM_V01)
h.append('namespace ids\n{\n')
for r in rows:
    h.append('inline constexpr const char* %s = "%s";\n' % (r['enum'], r['id']))
h.append('\ninline constexpr const char* const all[kNumParams] = {\n')
for r in rows:
    h.append('    "%s",\n' % r['id'])
h.append('};\n\n')
h.append('inline constexpr const char* id (P p) noexcept { return all[(int) p]; }\n\n')
h.append('inline constexpr int numLFOs     = 3;\n')
h.append('inline constexpr int numModSlots = 16;\n')
h.append('inline constexpr int numModSlotsV01 = 8;\n\n')
h.append('inline constexpr const char* lfoShapeSuffix = "_shape";\ninline constexpr const char* lfoRateSuffix = "_rate";\n')
h.append('inline constexpr const char* lfoSyncSuffix = "_sync";\ninline constexpr const char* lfoDivSuffix = "_div";\n')
h.append('inline constexpr const char* lfoModeSuffix = "_mode";\ninline constexpr const char* lfoFadeSuffix = "_fade";\n')
h.append('inline constexpr const char* lfoPhaseSuffix = "_phase";\n')
h.append('inline constexpr const char* modSrcSuffix = "_src";\ninline constexpr const char* modDstSuffix = "_dst";\n')
h.append('inline constexpr const char* modDepthSuffix = "_depth";\n\n')
h.append('inline juce::String lfoParam (int lfoIndex1Based, const char* suffix) { return "lfo" + juce::String (lfoIndex1Based) + suffix; }\n')
h.append('inline juce::String modParam (int slotIndex1Based, const char* suffix) { return "mod" + juce::String (slotIndex1Based) + suffix; }\n\n')
h.append('enum class LfoField { Shape, Rate, Sync, Div, Mode, Fade, Phase };\n')
h.append('enum class ModField { Src, Dst, Depth };\n')
h.append('/** Enum of an LFO parameter (LFO 1..3). */\n')
h.append('inline constexpr P lfoP (int lfoIndex1Based, LfoField f) noexcept { return (P) ((int) P::lfo1Shape + (lfoIndex1Based - 1) * 7 + (int) f); }\n')
h.append('/** Enum of a matrix slot parameter (slot 1..16). */\n')
h.append('inline constexpr P modP (int slotIndex1Based, ModField f) noexcept\n{\n')
h.append('    return slotIndex1Based <= numModSlotsV01 ? (P) ((int) P::mod1Src + (slotIndex1Based - 1) * 3 + (int) f)\n')
h.append('                                             : (P) ((int) P::mod9Src + (slotIndex1Based - 9) * 3 + (int) f);\n}\n')
h.append('} // namespace ids\n} // namespace aeriform\n')
open(os.path.join(ROOT, 'Source/Params/ParamIDs.h'), 'w', encoding='utf-8', newline='\n').write(''.join(h))

t = []
t.append('// GENERATED by scripts/gen_params.py - do not edit by hand.\n')
t.append('// { P, id, name, section, kind, min, max, default, centre, step, unit, format, choices, tooltip }\n')
t.append('static const ParamDef kParamDefs[kNumParams] = {\n')
for r in rows:
    t.append('    { P::%s, "%s", %s, ParamSection::%s, ParamKind::%s, %s, %s, %s, %s, %s, "%s", Fmt::%s, ChoiceList::%s, %s },\n' % (
        r['enum'], r['id'], cstr(r['name']), r['sec'], r['kind'], fnum(r['lo']), fnum(r['hi']), fnum(r['default']),
        fnum(r['centre']), fnum(r['step']), r['unit'], r['fmt'], r['choices'], cstr(r['tip'])))
t.append('};\n')
open(os.path.join(ROOT, 'Source/Params/ParamTable.inc'), 'w', encoding='utf-8', newline='\n').write(''.join(t))

print('generated %d parameters (%d from v0.1, %d new)' % (len(rows), NUM_V01, len(rows) - NUM_V01))
