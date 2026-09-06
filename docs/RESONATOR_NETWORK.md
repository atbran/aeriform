# Resonator network


## Experimental contact route

The CONTACT page connects one selected source to one selected destination among the three running resonators. Twelve appended parameters cover enable, source, destination, gap, stiffness, hardness, damping, friction, asymmetry, amount, polarity and independent 1x/2x/4x oversampling. Disabled by default. Sending to the same slot produces no contact.

Source displacement must exceed a polarity-dependent gap. A bounded penetration curve controls scattering of the relative source/destination displacement. The undelayed instantaneous map applies equal source reaction and signed destination transfer; its coefficient never exceeds 0.45, so it cannot increase the sum of squared displacements. Contact damping increases dissipative equalization; friction adds a bounded corrugated surface response. This is a musical scattering model, not a calibrated mechanical simulation.

Interpolation/decimation adds memory, so the instantaneous proof alone does not prove energy conservation of the complete delayed network. Injection is therefore independently capped and scaled by 0.9 times the destination's actual resonator loss (a small floor at maximum feedback, conservative fixed loss for modal slots). The existing network saturators and governor remain active. The master output limiter is not needed for contact bounds. Continuous controls are smoothed; source, destination, polarity and quality changes use two states with a 12 ms crossfade.

Five focused tests passed with 486787 checks: randomized instantaneous energy contraction, gap/bound checks, alias suppression, direct-network extreme settings without the master output stage at 44.1/48/96 kHz, smooth route/quality/bypass transitions, real host-audio changes, state restoration and page rendering. One coherent high-frequency test measured 101.55 dB reduction of the folded third harmonic from 1x to 4x. That is a single-bin measurement, not a broadband alias-rejection specification; Eco remains deliberately alias-prone. See build/contact-tests.log and experimental/contact.png.


## True stereo network

The PHYSICAL page switches between CONTACT and TRUE STEREO controls. Economy preserves the existing single physical network. Physical mode runs independent left/right resonator instances with length divergence (cents), loss-scaled interchannel coupling, A-minus-B exciter placement, pickup and damping offsets, mid/side rotation, width and low-frequency convergence. Mode changes fade over 15 ms; the second network stops after the fade to Economy. No voice count or exciter quality changes occur automatically.

Zero width makes the network channels identical. Later body/pan processing and stereo effects can still alter the final channel relationship. Mono sums retain the mid signal; physical detuning can still produce acoustic beating. Cross coupling is capped and scaled to the smallest active resonator loss. Both networks retain their own collision and filter states; the Energy Loop return averages the two physical networks.

Five focused tests passed with 220685 checks (build/stereo-tests-2.log): bit-exact Economy comparison with the original network, independent energy with finite output at three sample rates, zero-width equality, low-frequency side suppression, mode shutdown, actual host audio changes, state restoration and page rendering. All seven filter regressions also passed after optimizing unused comb resets and reducing unused oversampled lane storage. Screenshot: experimental/stereo.png. Final CPU/memory profiling remains pending.


## Shared sympathetic bank

A single twelve-mode bank per engine responds to the stereo voice sum before global effects. Sustained excitation is calibrated per second independently of decay, and rising source envelopes deposit short attack pulses. Mid/side enter orthogonal modal coordinates so antiphase input can still excite the bank. A smooth input budget bounds the norm of the complete complex state vector to 0.75; a separate radial finite-state guard protects individual modes. Voice bookkeeping does not attenuate the input. Output normalization considers active mode weights and coincident frequencies. Hold stops excitation and approaches unit-radius rotation; damping shortens higher modes substantially, and damper shortens all ringing. Disabling fades out and clears stored sound. See AUDIBILITY_RESULTS.md for the current calibration.

Tuning includes chromatic, major, minor, pentatonic, whole tone, twelve custom intervals, harmonic series, held MIDI notes (including sustain pedal), and a captured chord. Held-note tuning retains the last nonempty chord for natural ringing after release. Captured notes are common MIDI/chord memory shared by both morph endpoints, saved in PatchTools XML; they are not separately morphed. GUI capture is undoable. Clear and host-automated capture are edge commands and excluded from randomization and endpoint interpolation. Changing count fades mode weights. Preparation/sample-rate changes preserve captured chord requests but clear sound energy.

The complete checkpoint passed 107 tests / 2245948 checks, zero failures (build/sympathetic-all-tests.log). Focused tests measure pitch within 2 cents at three sample rates, known modal decay, damper behavior, frozen-energy retention, clearing, scale frequencies, held-note/pedal/capture behavior, 1/8/16-voice normalized input, audio-path changes, chord/session restoration before prepare, undo/redo, and navigation restoration. Screenshot: experimental/sympathetic.png.

## Navigation after user testing

Six main tabs: MAIN, EXCITERS, NETWORK, MOTION, SPACE, ADVANCED. NETWORK contains resonator routing, contact/stereo and the sympathetic bank. SPACE contains existing effects and modular filters. ADVANCED contains morphing and patch tools. Saved sub-sections restore, and the two removed experimental top-level tabs migrate to their new sections. Existing parameter IDs are unchanged.


## User-test corrections

Explicit Bypass Resonators now routes excitation directly through the network input/output path. Turning off every resonator does the same and overrides Repipe's automatic slot engagement. Bypass uses the existing gain smoothing and stops the Energy Loop return. Defaults preserve enabled-resonator presets.

Contact stiffness now reaches its bounded response at musically useful displacements and uses a larger loss-normalized send. The reference patch's enabled-minus-bypassed difference is -27.73 dB RMS relative to its output (about 4.1%). The updated coherent folded-third-harmonic test measures 77.62 dB suppression at 4x. Energy and extreme direct-network tests still pass; these numbers do not promise a large change for every gap or tuning combination.

The reported Repipe pitch reset was not reproduced in a two-second control/DSP-target test: A/B/C coarse edits all retained their values and target frequencies. Repipe enables additional resonators at their independent tunings; a pitched exciter and the other slots can still dominate the perceived fundamental. The test verifies retained tuning targets, not a guarantee that one audible spectral peak follows a single slot in a coupled network.

For knobs with existing matrix destinations, right-click Assign modulation source adds a route in an empty matrix slot. Existing routes are never silently replaced. Select a source to edit when several are assigned, then drag the teal ring (or Alt-drag the knob) to set signed depth. Shift-drag is finer. A complete depth drag is one undo action, and the base parameter is unchanged. Knobs without a matrix destination report that limitation in the menu; MIDI learn remains separate.


## Shared coupled room

NETWORK / COUPLED ROOM exposes an eight-line, shared feedback-delay network with shape/size, diffusion, wall damping, air absorption, voice send, feedback, width/output, freeze/clear and a filtered delayed return to the voices. It precedes the global effects and follows the sympathetic bank. The appended Reed in a Small Room preset demonstrates its audible path without changing the original factory presets.

The instrument room soft-bounds the stereo sum without dividing its send by voice count. The eight-line feedback core remains contracting below freeze, with maximum gain 0.98 and loss-scaled excitation. A separate feedforward layer supplies positive-delay early reflections; size, shape, diffusion, wall damping and air absorption change their timing and tone. Calibrated early/late output gains sit outside the physical-return path. Shimmer explicitly selects the legacy room configuration, preserving its earlier input/output math and feedback gain.

The physical return is filtered, DC-blocked, bounded, delayed by at least 32 samples, and smoothly distributed across contributors. Each voice funds additional room injection from its own exciter output power measured before movable filters, the energy-loop return, and the resonator network. An energy budget credits one quarter of that measured power, leaks over 0.5 seconds, and cannot be replenished by the return signal itself. Peak and cumulative-energy limits act before the resonators, independently of the output limiter. This bounds added excitation; it does not assert that the complete nonlinear synth is a passive physical model. High feedback can substantially extend or reshape ringing, especially on plucked patches.

Both module pages expose AUDITION RETURN. It monitors the actual stereo return before global effects with a 20 ms fade, after running the normal graph. Only one target can be monitored. Monitoring is temporary and clears on preset/session load, processor reset/prepare, and editor close. Actual input and return RMS meters accompany the existing energy displays.

Current implementation measurements, stability/lifecycle tests, compatibility checks and delivery status are in AUDIBILITY_RESULTS.md. Enabled experimental patches intentionally sound different; original presets keep the new modules disabled. The whole-plugin allocation/lock audit and listening sign-off remain outstanding.
