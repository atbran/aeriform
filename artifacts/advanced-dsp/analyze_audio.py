from pathlib import Path
import wave, csv
import numpy as np
from PIL import Image,ImageDraw
root=Path(__file__).parent
rows=[]
img=Image.new('RGB',(1200,650),'white'); d=ImageDraw.Draw(img)
d.text((35,15),'Breath source onset: 10 ms RMS (raw levels), MIDI 60 / medium velocity',fill='black')
colors=['#222222','#2255aa','#009988','#bb6622','#aa3388','#668822']
names=['baseline-source-60-60']+[c+'-60-60-source' for c in ['Soft-Exhale','Focused-Jet','Whisper','Rough-Air','Flute-Air']]
for path in list(root.glob('*.wav'))+list((root.parent/'dsp-baseline').glob('*.wav')):
 with wave.open(str(path),'rb') as w:
  sr=w.getframerate(); raw=np.frombuffer(w.readframes(w.getnframes()),dtype=np.uint8).reshape(-1,3).astype(np.int32)
 x=(raw[:,0]|(raw[:,1]<<8)|(raw[:,2]<<16));x=np.where(x>=8388608,x-16777216,x)/8388608.
 rms=np.sqrt(np.mean(x*x)); peak=np.max(np.abs(x)); spec=np.abs(np.fft.rfft(x*np.hanning(len(x))))**2;freq=np.fft.rfftfreq(len(x),1/sr); centroid=np.sum(spec*freq)/max(np.sum(spec),1e-30)
 rows.append([path.name,sr,peak,rms,np.mean(x),centroid])
 if path.stem in names:
  j=names.index(path.stem);n=int(sr*.01);y=np.sqrt(np.mean(x[:len(x)//n*n].reshape(-1,n)**2,axis=1));pts=[(70+i*5.3,570-float(v)*2200) for i,v in enumerate(y[:200])];d.line(pts,fill=colors[j],width=2);d.text((780,30+j*20),path.stem,fill=colors[j])
d.line([(70,100),(70,570),(1130,570)],fill='black');d.text((70,590),'0 s',fill='black');d.text((585,590),'1 s',fill='black');d.text((1100,590),'2 s',fill='black');d.text((10,565),'0',fill='black');d.text((10,345),'0.1',fill='black');d.text((10,125),'0.2',fill='black');img.save(root/'breath-contours.png')
with (root/'audio-analysis.csv').open('w',newline='') as f:
 writer=csv.writer(f);writer.writerow(['file','sample_rate','peak','rms','dc','power_spectral_centroid_hz']);writer.writerows(rows)
print(len(rows),'WAV files analyzed')
