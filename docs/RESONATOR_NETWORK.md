# Resonator network


## Experimental contact route

The CONTACT page connects one selected source to one selected destination among the three running resonators. Twelve appended parameters cover enable, source, destination, gap, stiffness, hardness, damping, friction, asymmetry, amount, polarity and independent 1x/2x/4x oversampling. Disabled by default. Sending to the same slot produces no contact.

Source displacement must exceed a polarity-dependent gap. A bounded penetration curve controls scattering of the relative source/destination displacement. The undelayed instantaneous map applies equal source reaction and signed destination transfer; its coefficient never exceeds 0.45, so it cannot increase the sum of squared displacements. Contact damping increases dissipative equalization; friction adds a bounded corrugated surface response. This is a musical scattering model, not a calibrated mechanical simulation.

Interpolation/decimation adds memory, so the instantaneous proof alone does not prove energy conservation of the complete delayed network. Injection is therefore independently capped and scaled by 0.15 times the destination's actual resonator loss (a small floor at maximum feedback, conservative fixed loss for modal slots). The existing network saturators and governor remain active. The master output limiter is not needed for contact bounds. Continuous controls are smoothed; source, destination, polarity and quality changes use two states with a 12 ms crossfade.

Five focused tests passed with 486787 checks: randomized instantaneous energy contraction, gap/bound checks, alias suppression, direct-network extreme settings without the master output stage at 44.1/48/96 kHz, smooth route/quality/bypass transitions, real host-audio changes, state restoration and page rendering. One coherent high-frequency test measured 101.55 dB reduction of the folded third harmonic from 1x to 4x. That is a single-bin measurement, not a broadband alias-rejection specification; Eco remains deliberately alias-prone. See build/contact-tests.log and experimental/contact.png.


## True stereo network

The PHYSICAL page switches between CONTACT and TRUE STEREO controls. Economy preserves the existing single physical network. Physical mode runs independent left/right resonator instances with length divergence (cents), loss-scaled interchannel coupling, A-minus-B exciter placement, pickup and damping offsets, mid/side rotation, width and low-frequency convergence. Mode changes fade over 15 ms; the second network stops after the fade to Economy. No voice count or exciter quality changes occur automatically.

Zero width makes the network channels identical. Later body/pan processing and stereo effects can still alter the final channel relationship. Mono sums retain the mid signal; physical detuning can still produce acoustic beating. Cross coupling is capped and scaled to the smallest active resonator loss. Both networks retain their own collision and filter states; the Energy Loop return averages the two physical networks.

Five focused tests passed with 220685 checks (build/stereo-tests-2.log): bit-exact Economy comparison with the original network, independent energy with finite output at three sample rates, zero-width equality, low-frequency side suppression, mode shutdown, actual host audio changes, state restoration and page rendering. All seven filter regressions also passed after optimizing unused comb resets and reducing unused oversampled lane storage. Screenshot: experimental/stereo.png. Final CPU/memory profiling remains pending.
