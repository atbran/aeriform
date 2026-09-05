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
CHOICES={
 'FilterPositions':['Exciter A','Exciter B','Combined exciters','Before wavefolder','After wavefolder','Network input','Res A input','Res B input','Res C input','Res A output','Res B output','Res C output','Cross feedback','Energy loop','Res A loop / modal input','Res B loop / modal input','Res C loop / modal input','Post body','Pre effects','Post effects'],
 'FilterModels':['Low-pass','High-pass','Band-pass','Notch','SVF morph','Driven SVF','Ladder low-pass','Formant / vowel','Comb','Modal bank','Tilt EQ'],
 'FilterSlopes':['12 dB / octave','24 dB / octave'],
 'FilterVowels':['A','E','I','O','U']}
