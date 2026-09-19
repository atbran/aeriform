# PIPE mathematical review

## Corrections used by the prototype

1. Gain law is correct; the original prose reversed both comparisons. RT is nominal.
2. First-order allpass phase is -w + 2 atan2(c sin(w), 1+c cos(w)); c=0 is a unit
   delay and c=1 is unity (excluding the removable Nyquist singularity). No allpass
   is needed in P1.
3. Existing Lagrange weights must be included. For fraction f, define
   C(w,f)=c0 exp(jw)+c1+c2 exp(-jw)+c3 exp(-2jw).
   Interpolation phase error relative to the ideal fraction is arg(C)+wf.
   Its extra delay is -arg(C)/w-f. Solve the delay length iteratively with this
   correction, rather than counting the fractional delay twice.
4. After push(), readLagrange(D) reads a sample D-1 time steps old. The explicit
   feedback return from the previous sample adds one step. Thus the complete loop
   has D samples plus interpolation/filter/ADAA phase delay, not D+1.
5. ADAA in the linear region is (x[n]+x[n-1])/2: H=(1+z^-1)/2, phase delay 0.5,
   magnitude cos(w/2). Half-sample compensation does not remove that attenuation.
   A nonlinear driven waveform does not have one universal LTI phase response;
   report amplitude-dependent tuning separately.
6. With changing drive/knees, evaluate both primitive endpoints using the CURRENT
   curve. Reuse the cached primitive only while that curve is unchanged. Otherwise
   equal raw inputs can produce a spurious impulse from mismatched primitives.
7. Hardness 0=soft, 1=hard requires k=0.9*h. The supplied k=0.9*(1-h) reverses that
   label. The primitive in the draft is continuous and differentiates to the
   specified positive/negative branches when each endpoint selects its own knee.
8. DC injection is stored in the delay before reaching the highpass. The previous
   claim that this topology does not offset the delay line is false. A linear
   interpolator's phase does not depend on DC amplitude. DC can bias saturation;
   pure steady DC does not guarantee a continuing pitched tone in a passive loop.
9. Fixed-loop filter magnitude bounds are not a complete nonlinear/network
   contraction proof. Lagrange has negative taps (no simple sup-norm contraction),
   ADAA has state, and coupling/modulation need separate treatment. A bounded
   saturator followed by stable fixed filters supplies a useful isolated bounded-
   output argument for bounded excitation, not a global passivity proof.
10. Normalized HP and LP coefficients use exp(-2 pi fc/fs). Their named frequency
    is a pole parameter, not an exact digital -3 dB cutoff near Nyquist.

## Primary references

- Julius O. Smith, [First-order allpass interpolation](https://www.dsprelated.com/freebooks/pasp/First_Order_Allpass_Interpolation.html).
- Bilbao et al., [Antiderivative Antialiasing for Stateful Systems, DAFx 2019](https://www.dafx.de/paper-archive/2019/DAFx2019_paper_4.pdf).

The corrections above also follow directly from the equations and the current
FractionalDelay implementation. Numerical findings are recorded by the P1 tools;
they are not an independent acceptance verdict.

## Post-review correction: tune the small-signal response peak

The original phase solution leaves closed-filter spectral peaks sharp. The revised
prototype uses that length as a starting point and solves for a stationary maximum
of the complete output response, including its numerator. With q=exp(-jw), forward
transfer F=delay(D-1)*interpolation*ADAA*HP*LP, and L=sign*g*q*F:

```
T = F / (1-L)
A = (dF/dw) / F
d(log T)/dw = (A - j*L) / (1-L)
d(log |T|)/dw = real((A - j*L) / (1-L))
```

For a target w, vary D until the last expression is zero. The prototype brackets
the nearest positive-to-negative crossing within +/-20% of the phase solution,
bisects 36 times, and checks that frequency slope changes from positive below the
target to negative above it. Fixed search counts bound work; repeated unchanged
tuning configurations reuse the result. Pressure/cutoff excitation changes alone
do not invalidate this small-signal tuning cache.

When no suitable peak is found in the bounded search, retain the clamped phase
solution and expose `peak_tuned=false`. A near-zero loop gain or highly attenuating
filters may not leave a useful pitched resonance. This is not a claim that no
peak exists anywhere outside the search, nor compliance with every possible
filter setting. Phase residual is now intentionally nonzero: peak location, not
zero phase residual, is the tuning objective.

This calculation describes low-amplitude response around zero. Driven saturation,
exciter coloration, and pitch inferred from a sustained noisy signal need separate
measurements. Do not extrapolate the 72 impulse results into those regimes.
