def register(F,C,B,I):
 for i in range(1,4):
  e=f'filter{i}';id=f'filter{i}_';name=f'Filter {i} '
  B(e+'On',id+'on',name+'Enabled','Shaping',False,'Enable this movable filter block.')
  C(e+'Position',id+'position',name+'Position','Shaping','FilterPositions',5,'Insert this block at the selected point. Position changes crossfade.')
  C(e+'Type',id+'type',name+'Type','Shaping','FilterModels',0,'Surgical, character or comb filtering.')
  F(e+'Cutoff',id+'cutoff',name+'Cutoff','Shaping',20,20000,4000,'Hz','Hz','Cutoff or comb/modal tuning frequency.',centre=1500)
  F(e+'Resonance',id+'resonance',name+'Resonance','Shaping',0,1,.1,'%','Percent','Resonance. Loop placements normalize gain for stability.')
  F(e+'Drive',id+'drive',name+'Drive','Shaping',0,1,0,'%','Percent','Antiderivative-antialiased gain-compensated soft drive.')
  F(e+'Keytrack',id+'keytrack',name+'Key Track','Shaping',0,2,0,'%','Percent','Cutoff tracking relative to MIDI C4.')
  F(e+'Env',id+'env',name+'Envelope','Shaping',-4,4,0,'oct','Plain','Amplitude envelope cutoff offset in octaves.')
  F(e+'Morph',id+'morph',name+'Morph','Shaping',0,1,.5,'%','Percent','SVF response blend, tilt or comb feedback polarity.')
  C(e+'Slope',id+'slope',name+'Slope','Shaping','FilterSlopes',0,'Two or four poles for SVF/ladder types.')
  C(e+'Vowel',id+'vowel',name+'Vowel','Shaping','FilterVowels',0,'Formant vowel selection.')
  F(e+'Mix',id+'mix',name+'Mix','Shaping',0,1,1,'%','Percent','Smoothed wet/dry amount.')

 B('contactOn','contact_on','Collision Enabled','Network',False,'Enable a bounded nonlinear contact route between two running resonators.')
 C('contactSource','contact_source','Collision Source','Network','ContactNodes',0,'Source displacement is compared with the contact gap.')
 C('contactDestination','contact_destination','Collision Destination','Network','ContactNodes',1,'Receiving resonator. A reaction is also applied to the source. Both slots must be running.')
 F('contactGap','contact_gap','Contact Gap','Network',0,1,.05,'','Plain','Displacement threshold for contact.',centre=.1)
 F('contactStiffness','contact_stiffness','Contact Stiffness','Network',0,1,.5,'%','Percent','Nonlinear stiffness, bounded before network injection.')
 F('contactHardness','contact_hardness','Contact Hardness','Network',1,4,1.5,'','Plain','Exponent of the penetration response.')
 F('contactDamping','contact_damping','Contact Damping','Network',0,1,.2,'%','Percent','Increases dissipative equalization during contact.')
 F('contactFriction','contact_friction','Contact Friction','Network',0,1,0,'%','Percent','Bounded corrugation of the contact surface for buzz and chatter.')
 F('contactAsymmetry','contact_asymmetry','Contact Asymmetry','Network',-1,1,0,'%','BipolarPercent','Different positive and negative displacement gaps.')
 F('contactAmount','contact_amount','Collision Amount','Network',0,1,.3,'%','Percent','Smoothed route strength with destination-loss normalization.')
 C('contactPolarity','contact_polarity','Contact Polarity','Network','Polarities',0,'Positive or inverted destination scattering.')
 C('contactQuality','contact_quality','Contact Quality','Network','QualityModes',1,'Contact oversampling: Eco 1x, Normal 2x, High 4x. Changes crossfade.')


 C('stereoMode','stereo_mode','Physical Stereo Mode','Network','PhysicalStereoModes',0,'Economy preserves the original single network. Physical runs independent left and right resonators.')
 F('stereoDivergence','stereo_divergence','Length Divergence','Network',0,30,8,'ct','Cents','Total pitch separation between the left and right physical networks.')
 F('stereoCoupling','stereo_coupling','Stereo Cross Coupling','Network',0,1,.1,'%','Percent','Bounded, resonator-loss-scaled exchange between left and right networks.')
 F('stereoExciterSpread','stereo_exciter_spread','Exciter Spread','Network',0,1,0,'%','Percent','Places the A-minus-B excitation difference across the physical networks.')
 F('stereoPickupSpread','stereo_pickup_spread','Pickup Spread','Network',0,1,.1,'%','Percent','Different pickup positions in each physical network.')
 F('stereoDamping','stereo_damping','Damping Divergence','Network',-1,1,0,'%','BipolarPercent','Opposite damping offsets for the two networks.')
 F('stereoRotation','stereo_rotation','Stereo Rotation','Network',-1,1,0,'%','BipolarPercent','Rotate the mid/side field up to 45 degrees, before mono bass.')
 F('stereoWidth','stereo_width','Physical Stereo Width','Network',0,2,1,'%','Percent','Mid/side width. Zero produces identical channels in Physical mode.')
 F('stereoMonoBass','stereo_mono_bass','Mono Bass','Network',20,1000,120,'Hz','Hz','Remove low frequencies from the side channel. Minimum disables convergence.',centre=150)


 B('symOn','sym_on','Sympathetic Bank Enabled','Network',False,'One shared bank of twelve tuned modes responds to all voices.')
 F('symSend','sym_send','Sympathetic Send','Network',0,1,.4,'%','Percent','Stereo voice send with source-derived attacks and a bounded modal energy budget.')
 F('symReturn','sym_return','Sympathetic Return','Network',0,2,.4,'%','Percent','Shared bank return level before the global effects.')
 F('symDamper','sym_damper','Damper Position','Network',0,1,0,'%','Percent','Raises damping to quickly stop sympathetic ringing.')
 F('symDecay','sym_decay','Sympathetic Decay','Network',100,60000,6000,'ms','Ms','Approximate time for an isolated mode to decay by 60 dB before damping.',centre=6000)
 F('symDamping','sym_damping','Sympathetic Damping','Network',0,1,.3,'%','Percent','Frequency-dependent decay loss.')
 F('symBrightness','sym_brightness','Sympathetic Brightness','Network',0,1,.5,'%','Percent','Relative output of the upper tuned modes.')
 F('symDetune','sym_detune','Sympathetic Detune','Network',-30,30,0,'ct','Cents','Deterministic spread of mode frequencies in cents.')
 F('symSpread','sym_spread','Sympathetic Spread','Network',0,1,.7,'%','Percent','Stereo placement of the twelve tuned modes.')
 C('symTuning','sym_tuning','Sympathetic Tuning','Network','SympatheticTunings',1,'Scale, custom intervals, harmonic series, held notes or a captured MIDI chord.')
 I('symRoot','sym_root','Sympathetic Root','Network',0,127,48,'MIDI','Root note for scale and interval tuning.')
 I('symCount','sym_count','Active Sympathetic Modes','Network',1,12,12,'','Number of active tuned modes.')
 F('symThreshold','sym_threshold','Sympathetic Threshold','Network',-96,0,-72,'dB','Db','Ignore excitation below this level.')
 B('symFreeze','sym_freeze','Sympathetic Hold','Network',False,'Smoothly stop decay and new excitation, retaining stored modal energy.')
 B('symClear','sym_clear','Clear Sympathetic Energy','Network',False,'Each toggle clears the stored modal energy.')
 B('symCapture','sym_capture','Capture Sympathetic Chord','Network',False,'Each toggle captures currently held MIDI notes. Captured chord is stored with the patch.')
 for i,interval in enumerate([0,4,7,12,16,19,24,28,31,36,40,43]):
  I(f'symInterval{i+1}',f'sym_interval{i+1}',f'Sympathetic Interval {i+1}','Network',-24,48,interval,'st','Custom chord interval relative to the root.')

 B('netBypass','net_bypass','Bypass Resonators','Network',False,'Route excitation directly past the resonators. Also happens automatically when no resonator is enabled. Overrides Repipe while bypassed.')


 B('roomOn','room_on','Coupled Room Enabled','Network',False,'One shared small-room model receives voices and returns bounded energy to the physical network.')
 F('roomSize','room_size','Room Size','Network',0,1,.3,'%','Percent','Physical path length scale of the eight-line room.')
 F('roomShape','room_shape','Room Shape','Network',0,1,.5,'%','Percent','Aspect and path-length pattern of the room.')
 F('roomWallDamping','room_wall_damping','Wall Damping','Network',0,1,.4,'%','Percent','Loss of high frequencies at room reflections.')
 F('roomDiffusion','room_diffusion','Room Diffusion','Network',0,1,.7,'%','Percent','Mixing between reflection paths.')
 F('roomAir','room_air','Air Absorption','Network',0,1,.3,'%','Percent','Frequency-dependent absorption along the room paths.')
 F('roomSend','room_send','Room Voice Send','Network',0,1,.4,'%','Percent','Voice sum sent to the shared room, normalized by active voice count.')
 F('roomNetworkReturn','room_network_return','Room Network Return','Network',0,1,.2,'%','Percent','Delayed room excitation, limited by an energy budget funded only by the voice exciters.')
 F('roomReturnDelay','room_return_delay','Room Return Delay','Network',1,250,15,'ms','Ms','Return path delay. Causal minimum is 32 samples, independent of host buffer size.',centre=30)
 F('roomReturnFilter','room_return_filter','Room Return Filter','Network',100,12000,3000,'Hz','Hz','Return low-pass filter, with a fixed 30 Hz DC/high-pass filter.',centre=2000)
 F('roomFeedback','room_feedback','Room Feedback','Network',0,1,.6,'%','Percent','Internal room feedback. Input gain follows loop loss for bounded excitation.')
 F('roomWidth','room_width','Room Width','Network',0,1,1,'%','Percent','Width of the audible room return.')
 F('roomLevel','room_level','Room Output Level','Network',0,2,.3,'%','Percent','Audible stereo room output; independent of network return strength.')
 B('roomFreeze','room_freeze','Room Freeze','Network',False,'Hold the room by stopping excitation and removing damping; paths settle to integer lengths.')
 B('roomClear','room_clear','Clear Room Energy','Network',False,'Each toggle clears all room and return-delay energy.')

CHOICES={
 'SympatheticTunings':['Chromatic','Major','Minor','Pentatonic','Whole tone','Custom intervals','Harmonic series','Held notes','Captured chord'],
 'PhysicalStereoModes':['Economy','Physical stereo'],
 'ContactNodes':['Res A','Res B','Res C'],
 'FilterPositions':['Exciter A','Exciter B','Combined exciters','Before wavefolder','After wavefolder','Network input','Res A input','Res B input','Res C input','Res A output','Res B output','Res C output','Cross feedback','Energy loop','Res A loop / modal input','Res B loop / modal input','Res C loop / modal input','Post body','Pre effects','Post effects'],
 'FilterModels':['Low-pass','High-pass','Band-pass','Notch','SVF morph','Driven SVF','Ladder low-pass','Formant / vowel','Comb','Modal bank','Tilt EQ'],
 'FilterSlopes':['12 dB / octave','24 dB / octave'],
 'FilterVowels':['A','E','I','O','U']}
