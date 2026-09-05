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

CHOICES={
 'PhysicalStereoModes':['Economy','Physical stereo'],
 'ContactNodes':['Res A','Res B','Res C'],
 'FilterPositions':['Exciter A','Exciter B','Combined exciters','Before wavefolder','After wavefolder','Network input','Res A input','Res B input','Res C input','Res A output','Res B output','Res C output','Cross feedback','Energy loop','Res A loop / modal input','Res B loop / modal input','Res C loop / modal input','Post body','Pre effects','Post effects'],
 'FilterModels':['Low-pass','High-pass','Band-pass','Notch','SVF morph','Driven SVF','Ladder low-pass','Formant / vowel','Comb','Modal bank','Tilt EQ'],
 'FilterSlopes':['12 dB / octave','24 dB / octave'],
 'FilterVowels':['A','E','I','O','U']}
