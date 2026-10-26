"""Development-only import of the documented CC0 sources (numpy + soundfile)."""
from pathlib import Path
import io, json, hashlib, sys, zipfile
import numpy as np
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'build/audio-tools'))
import soundfile as sf
out=root/'data/audio'; raw=root/'build/audio-source'
archive=zipfile.ZipFile(raw/'kenney.zip')
sources={}
def finish(data,rate,loop=False):
    data=np.asarray(data,dtype=np.float32)
    if data.ndim>1:data=data.mean(axis=1)
    if loop:
        n=min(int(rate*.12),len(data)//4);t=np.linspace(0,1,n,dtype=np.float32)
        data=np.concatenate((data[n:-n],data[-n:]*(1-t)+data[:n]*t))
    else:
        n=min(int(rate*.003),len(data)//4)
        data[:n]*=np.linspace(0,1,n);data[-n:]*=np.linspace(1,0,n)
    peak=float(np.max(np.abs(data)))
    if peak:data*=.75/peak
    return data
for stem,name,loop in [
    *[(f'laserSmall_{i:03}',f'shot_{i}',False) for i in range(3)],
    *[(f'impactMetal_{i:03}',f'hit_{i}',False) for i in range(3)],
    *[(f'explosionCrunch_{i:03}',f'explosion_{i}',False) for i in range(2)],
    ('spaceEngineLarge_000','ufo_engine',True),('spaceEngineSmall_000','player_engine',True)]:
    data,rate=sf.read(io.BytesIO(archive.read('Audio/'+stem+'.ogg')),dtype='float32')
    path=out/'sfx'/f'{name}.wav';sf.write(path,finish(data,rate,loop),rate,subtype='PCM_16')
    sources[path.relative_to(out).as_posix()]=dict(author='Kenney',source='https://kenney.nl/assets/sci-fi-sounds',original=stem+'.ogg',license='CC0-1.0',processing='mono, level normalization, edge fades or loop crossfade')
(out/'licenses/kenney.txt').write_bytes(archive.read('License.txt'))
for name,author,url in [('outer_space','wipics','https://opengameart.org/content/outer-space-loop'),('space_adventure','MintoDog','https://opengameart.org/content/space-adventure')]:
    source=root/f'data/audio/music/{name}.mp3'
    data,rate=sf.read(source,dtype='float32',always_2d=True)
    level=np.max(np.abs(data),axis=1);active=np.flatnonzero(level>.0001)
    if len(active):data=data[max(0,int(active[0])-int(.01*rate)):min(len(data),int(active[-1])+int(.01*rate)+1)]
    n=min(int(.08*rate),len(data)//4);t=np.linspace(0,1,n,dtype=np.float32)[:,None]
    data=np.concatenate((data[n:-n],data[-n:]*(1-t)+data[:n]*t))
    data*=.75/max(.001,float(np.max(np.abs(data))))
    path=out/'music'/f'{name}.flac';sf.write(path,data,rate,subtype='PCM_16')
    sources[path.relative_to(out).as_posix()]=dict(author=author,source=url,original=source.name,license='CC0-1.0',processing='trim encoder silence, short seam crossfade, level normalization, FLAC')
    source.replace(raw/source.name)
# Small original synthesized UI/result cues; no external samples.
for name,notes,duration in [('confirm',[660,990],.18),('victory',[440,554.365,659.255],1.0),('defeat',[330,311.127,220],1.1)]:
    rate=48000;t=np.arange(int(rate*duration))/rate
    data=sum(np.sin(2*np.pi*note*t)*np.exp(-t*(4 if duration>.5 else 18)) for note in notes)/len(notes)
    path=out/'sfx'/f'{name}.wav';sf.write(path,finish(data,rate),rate,subtype='PCM_16')
    sources[path.relative_to(out).as_posix()]=dict(author='CG2015 project',source='scripts/prepare_audio_assets.py',license='CC0-1.0',processing='original oscillator synthesis')
for name,record in sources.items():record['sha256']=hashlib.sha256((out/name).read_bytes()).hexdigest()
(out/'sources.json').write_text(json.dumps(sources,indent=2)+'\n')
print('Prepared',len(sources),'audio assets.')
