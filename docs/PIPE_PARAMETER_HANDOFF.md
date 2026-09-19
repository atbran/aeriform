# PIPE parameter handoff for UI and DSP integration

Selected set: **48 additions**, 16 per resonator, **833 total**. The four filter
morph endpoints and morph position are replaced by direct LP/HP cutoffs. No new
macro, blow-on switch, envelope controls, one-pole option or pitch drift parameter.

## Binding contract

Host string prefixes are `res_`, `rb_`, `rc_`. Enum/constant prefixes are
`resPipe`, `rbPipe`, `rcPipe`. For example:

```cpp
P::resPipePressure       // generated enum
ids::resPipePressure     // "res_pressure", for APVTS attachments
P::rbPipeLp              // "rb_lp"
P::rcPipeBore            // "rc_bore"
```

Every row below exists for all three slots. Percent ranges use **percentage
points**, not normalized 0..1 values; DSP divides them by 100. Hz/Q/seconds/drive
are already physical values. Attach UI controls directly to APVTS.

| ID suffix | Enum suffix after `resPipe` / `rbPipe` / `rcPipe` | Range | Default |
| --- | --- | --- | --- |
| pressure | Pressure | 0–100 % | 50 |
| dcnoise | DcNoise | 0–100 % | 50 |
| exc_cut | ExcCut | 20–20000 Hz | 2000 |
| exc_res | ExcRes | 0.5–8 Q | 0.7 |
| exc_kt | ExcKt | 0–150 % | 50 |
| exc_vt | ExcVt | 0–100 % | 50 |
| rt | Rt | 0.001–30 s, logarithmic | 0.5 |
| rt_kt | RtKt | 0–150 % | 50 |
| damp | Damp | 0–100 % | 0 |
| lp | Lp | 20–20000 Hz | 4000 |
| hp | Hp | 20–2000 Hz | 40 |
| filt_kt | FiltKt | 0–150 % | 100 |
| sat_drive | SatDrive | 1–32 x | 2 |
| sat_knee | SatKnee | 0–100 %, soft to hard | 40 |
| sat_sym | SatSym | -100–100 % | 0 |
| bore | Bore | Cone=0, Cylinder=1 | Cone |

`rt` is named **Nominal Decay**, stored in seconds, formatted with three decimal
places and an `s` suffix. It uses exact logarithmic host normalization; its tooltip
explains additional loss and shared-release truncation. `sat_knee` is displayed
as Hardness and uses the corrected soft-to-hard direction.

## Model selector

`ResMode::Pipe == 9`, `ResMode::Count == 10`. Existing entries 0..8 retain their
indices. The shared `choices::resModes()` list now includes `PIPE` at index 9.
Use existing selector IDs `res_mode`, `rb_type`, `rc_type`; no separate enable flag.
Bore uses `PipeBore::{Cone,Cylinder}` and `ChoiceList::PipeBores`.

All new controls are in `ParamSection::Network`, with slot-qualified PIPE names.
There is no page-local parameter storage or second set of IDs for the PIPE page.

## Verification and remaining work

Regeneration yields 833 unique IDs. The first 785 generated table rows and enum
indices were compared to a pre-edit snapshot and are unchanged, including their
defaults and ranges. A second generation is byte-identical; auxiliary generated
modulation/rack binding files are unchanged.

The `Aeriform` shared-code target builds successfully with GCC 14.2 Release.
Independent code review found no concrete issue in the parameter definitions,
choice indices, seconds formatting or logarithmic range mapping. This is not a
full-plugin acceptance or normalized-host-automation verdict.

Extending the model selector changes normalized host choice mapping; unchanged
saved indices do not prove unchanged normalized automation. Independent state and
host automation verification remains required.

This handoff adds real host parameters and choice metadata. It does **not** connect
the prototype PIPE processor to the production voice, add modulation destinations,
or provide loop-gain/phase telemetry. UI bindings can now be implemented; displays
must await actual DSP telemetry rather than estimating it from controls. Full DSP,
allocator, CPU and coupled-network acceptance still require production integration.
