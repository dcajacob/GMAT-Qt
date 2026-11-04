#!/usr/bin/env python3
"""Deterministic synthetic input writer and independent calendar/column oracle.

No GMAT imports, generated trajectory references or SolarFluxReader algorithms.
CSSI column meanings: https://celestrak.org/SpaceData/SpaceWx-format.php
Ap windows: shipped NRLMSISE-00 GTD7 input contract, nrlmsise00_sub.f:45-59.
"""
import argparse,datetime as dt,hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent
DATA=HERE/'weather-choice-data'
EPOCH=dt.datetime(1941,1,5,12)
HISTORY=['ConstantFluxAndGeoMag','CSSISpaceWeatherFile']
ERROR=['Nominal','PlusTwoSigma','MinusTwoSigma']
TIMING=['NominalCycle','EarlyCycle','LateCycle']

def fixtures():
 start=dt.date(2024,1,1);observed=[];predicted=[]
 for i in range(91):
  day=start+dt.timedelta(days=i);ap=[2,3,4,5,6,7,9,12][i%8];kp=[3,7,10,13,17,20,23,27][i%8];obs=110 if i<10 else 210
  prefix=f'{day.year:4d}{day.month:3d}{day.day:3d}{2500:5d}{i%27+1:3d}'+''.join(f'{kp:3d}'for _ in range(8))+f'{kp*8:4d}'+''.join(f'{ap:4d}'for _ in range(8))+f'{ap:4d}{0.0:4.1f}{0:2d}{0:4d}'
  assert len(prefix)==92
  row=prefix+f'{obs+200:6.1f}{0:2d}{obs+210:6.1f}{obs+220:6.1f}{obs:6.1f}{obs+10:6.1f}{obs+20:6.1f}'
  (observed if i<10 else predicted).append(row)
 cssi='DATATYPE CssiSpaceWeather\nVERSION 1.2\nBEGIN OBSERVED\n'+'\n'.join(observed)+'\nEND OBSERVED\nBEGIN DAILY_PREDICTED\n'+'\n'.join(predicted)+'\nEND DAILY_PREDICTED\nBEGIN MONTHLY_PREDICTED\nEND MONTHLY_PREDICTED\n'
 schatten='           NOMINAL TIMING      EARLY TIMING        LATE TIMING\n mo. yr. mean +2sig -2sig ap mean +2sig -2sig ap mean +2sig -2sig ap\nBEGIN_DATA\n'+''.join(f'{m:2d} 2024 130 140 120 2 160 170 150 4 190 200 180 6\n'for m in range(1,5))+'END_DATA\n'
 return {'cssi.txt':cssi,'schatten.txt':schatten}

def oracle(files):
 # The oracle reads semantic whitespace columns, independently of writer widths.
 rows={};section=None
 for line in files['cssi.txt'].splitlines():
  if line.startswith('BEGIN '):section=line[6:];continue
  if line.startswith('END '):section=None;continue
  fields=line.split()
  if section not in ['OBSERVED','DAILY_PREDICTED']or len(fields)!=33:continue
  day=dt.date(*map(int,fields[:3]));rows[day]=dict(ap=list(map(float,fields[14:22])),daily_ap=float(fields[22]),flux=float(fields[30]),average=float(fields[31]),section=section)
 predictions=[]
 for line in files['schatten.txt'].splitlines():
  fields=line.split()
  if len(fields)==14 and fields[0].isdigit():predictions.append(list(map(float,fields[2:])))
 assert len(rows)==91 and len(predictions)==4 and all(row==predictions[0]for row in predictions)
 def inputs(day,slot):
  instant=dt.datetime.combine(day,dt.time())+dt.timedelta(hours=slot*3,minutes=30)
  def lag(n):
   at=instant-dt.timedelta(hours=3*n)
   return rows[at.date()]['ap'][at.hour//3]
  return dict(utc_gmat_mjd=(instant-EPOCH).total_seconds()/86400,utc=instant.isoformat(),obsF107=rows[day]['flux'],obsCtrF107a=rows[day]['average'],ap=[rows[day]['daily_ap']]+[lag(i)for i in range(4)]+[sum(lag(i)for i in range(4,12))/8,sum(lag(i)for i in range(12,20))/8])
 probes={name:[inputs(day,slot)for slot in range(8)]for name,day in [('historic',dt.date(2024,1,6)),('forecast',dt.date(2024,2,6))]}
 variants=[]
 for hi,h in enumerate(HISTORY):
  for pred in ['CSSISpaceWeatherFile','ConstantFluxAndGeoMag','SchattenFile']:
   options=[(e,t)for t in TIMING for e in ERROR]if pred=='SchattenFile'else[('Nominal','NominalCycle')]
   for e,t in options:
    key=('CssiHistory'if hi else'ConstantHistory')+('Schatten'+str(TIMING.index(t))+str(ERROR.index(e))if pred=='SchattenFile'else'CssiPredict'if pred=='CSSISpaceWeatherFile'else'ConstantPredict')
    vals=predictions[0];ti=TIMING.index(t);ei=ERROR.index(e)
    variants.append(dict(key=key,history=h,predicted=pred,error=e,timing=t,schatten_flux=vals[ti*4+ei],schatten_ap=vals[ti*4+3]))
 return dict(schema=1,cases=variants,probes=probes,forecast_epoch='06 Feb 2024 12:30:00.000',forecast_utc_mjd=(dt.datetime(2024,2,6,12,30)-EPOCH).total_seconds()/86400,duration_seconds=5,mu=398600.4415,radius_km=7000,constant_F107=150,constant_F107A=155,constant_Kp=3,position_bound_km=2e-5,velocity_bound_km_s=2e-8,weather_value_bound=1e-12,fixture_sha256={n:hashlib.sha256(s.encode()).hexdigest()for n,s in files.items()},sources=['doc/help/src/Resource_ForceModel.xml','plugins/Msise00Plugin/src/base/atmosphere/nrlmsise00_sub.f'],limits=['Earth NRLMSISE00 and Spherical drag only;other models,bodies and spacecraft shapes remain open.','Synthetic flat solar-flux columns distinguish sources and all nine Schatten selections;monthly interpolation,real weather provenance and empirical density truth are not qualified.','Daily Ap varies by date and is flat inside each day;all eight slot lookups exercise cross-day histories,but arbitrary within-day patterns remain open.','Cached forecast file data is checked before explicit public-API UTC lookup probes on the runtime clone. Constant branches have no effective-input public readback;configured/runtime constants are checked without treating an inactive file buffer as their result.','Null-area mission checks the zero-force invariant and finite positive density,not nonzero drag accuracy.'])

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');a=parser.parse_args();files=fixtures();expected=oracle(files);targets={DATA/n:s for n,s in files.items()};targets[HERE/'WeatherChoiceCases.json']=json.dumps(expected,indent=2)+'\n'
 for p,text in targets.items():
  if a.check:assert p.read_text()==text,p
  else:p.parent.mkdir(exist_ok=True);p.write_text(text)
 print(json.dumps(dict(cases=len(expected['cases']),lookups_per_case=16,fixture_bytes=sum(len(s)for s in files.values()),verified=a.check)))
if __name__=='__main__':main()
