"""Independent declared A1/TAI/UTC/TT/TDB output calendars and circle states.

A1=TAI+0.0343817s is an explicit private interface convention. TT=TAI+32.184s.
TDB uses the independent NAIF LSK model K*sin(M+EB*sin(M)),iterated in TDB,
not GMAT's converter or reader. Reference:
https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/time.html
"""
import copy
from datetime import datetime,timedelta
import json
import math
from pathlib import Path

SYSTEMS=['A1','TAI','UTC','TT','TDB']
J2000=datetime(2000,1,1,12)
MU=398600.4415
RADIUS=7000.
A1_TAI=.0343817
TT_TAI=32.184
NAIF_K=.001657
NAIF_EB=.01671
NAIF_M0=6.239996
NAIF_M1=1.99096871e-7
EPOCHS={'J2000':(datetime(2000,1,1,12),32,'01 Jan 2000 12:00:00.000'),
        'April2000':(datetime(2000,4,1,12),32,'01 Apr 2000 12:00:00.000'),
        'Leap2016':(datetime(2016,12,31,23,59,55),36,'31 Dec 2016 23:59:55.000')}


def iso(value):
    rounded=J2000+timedelta(milliseconds=round((value-J2000).total_seconds()*1000))
    return rounded.strftime('%Y-%m-%dT%H:%M:%S.')+f'{rounded.microsecond//1000:03d}'


def tag(family,system,seconds):
    start,dat,_=EPOCHS[family]
    if system=='UTC':
        if family=='Leap2016':
            if seconds==5:return '2016-12-31T23:59:60.000'
            if seconds>5:seconds-=1
        return iso(start+timedelta(seconds=seconds))
    tai=start+timedelta(seconds=dat+seconds)
    if system=='TAI':return iso(tai)
    if system=='A1':return iso(tai+timedelta(seconds=A1_TAI))
    tt=tai+timedelta(seconds=TT_TAI)
    if system=='TT':return iso(tt)
    if system=='TDB':
        tt_seconds=(tt-J2000).total_seconds();tdb=tt_seconds
        for _ in range(5):
            anomaly=NAIF_M0+NAIF_M1*tdb
            tdb=tt_seconds+NAIF_K*math.sin(anomaly+NAIF_EB*math.sin(anomaly))
        return iso(J2000+timedelta(seconds=tdb))
    raise ValueError(system)


def state(seconds):
    rate=math.sqrt(MU/RADIUS**3);speed=math.sqrt(MU/RADIUS);a=rate*seconds
    return [RADIUS*math.cos(a),RADIUS*math.sin(a),0.,-speed*math.sin(a),speed*math.cos(a),0.]


def catalog():
    base=json.loads(Path(__file__).with_name('NasaOemRecordSupportCases.json').read_text())
    template=next(c for c in base['cases']if c['key']=='ShortArc')
    cases=[]
    for family in EPOCHS:
        for system in SYSTEMS:
            c=copy.deepcopy(template)
            c.update(key=system+'_'+family,time_system=system,epoch_family=family,epoch_utc=EPOCHS[family][2],duration=20,
                sampling='IntegratorSteps',scenario='RawSteps',input_epoch_format='UTCGregorian',
                mission="Propagate 'Entire arc' PropA(Sat) {Sat.ElapsedSecs = 20};\n",
                expected_segments=[[[t]+state(t)for t in [0,5,10,15,20]]],expected_epochs=[tag(family,system,t)for t in [0,5,10,15,20]],
                expected_final=[20]+state(20)+[9],expected_degree=4,expected_degrees=[4])
            cases.append(c)
    c=copy.deepcopy(next(c for c in cases if c['key']=='TAI_J2000'))
    c.update(key='TAI_TTInputWindow',input_epoch_format='TTGregorian',user_interval=True,scenario='UserInterval',
        initial_epoch='01 Jan 2000 12:01:09.184',final_epoch='01 Jan 2000 12:01:19.184',
        expected_segments=[[[t]+state(t)for t in [5,10,15]]],expected_epochs=[tag('J2000','TAI',t)for t in [5,10,15]],
        expected_degree=2,expected_degrees=[2])
    cases.append(c)
    return dict(cases=cases,object_id='GMAT-OEM-output-time',
        test_interface_profile=dict(sender='New NasaOemOutputTimeTests GUI fixture',receiver='RunOemOutputTime independent calendar/state checker',
            object_id='GMAT-OEM-output-time',object_name='Sat',originator='GMAT USER',version='1.0',center='EARTH',frame='EME2000',
            time_systems=SYSTEMS,A1_definition='TAI+0.0343817SIseconds',units=['km','km/s'],
            filenames=['Every state Δ.oem','mission.oem'],exchange='private local unchanged retained bytes,SHA256-bound;no retroactive agreement'),
        reference=dict(A1_TAI_seconds=A1_TAI,TT_TAI_seconds=TT_TAI,NAIF_K=NAIF_K,NAIF_EB=NAIF_EB,NAIF_M0=NAIF_M0,NAIF_M1=NAIF_M1,
            source='https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/time.html',
            leap_reference='2016-12-31 positive UTC leap;TAI-UTC36before,37after',calendar_precision_seconds=.001),
        scope='All five original FRR2.2 output systems,three epochs including leap and nonzeroTDB-TT,plus independent input-bound format. Original assertions/tolerances and all scientific/specification gaps remain.')


def support_catalog():
    cases=[]
    for system in SYSTEMS:
        labels=[]
        for seconds in [0.,.0004,.0008]:
            tai=J2000+timedelta(seconds=seconds-A1_TAI)
            if system=='A1':label=iso(J2000+timedelta(seconds=seconds))
            elif system=='TAI':label=iso(tai)
            elif system=='UTC':label=iso(tai-timedelta(seconds=32))
            elif system=='TT':label=iso(tai+timedelta(seconds=TT_TAI))
            else:
                tt=(tai+timedelta(seconds=TT_TAI)-J2000).total_seconds();tdb=tt
                for _ in range(5):
                    anomaly=NAIF_M0+NAIF_M1*tdb
                    tdb=tt+NAIF_K*math.sin(anomaly+NAIF_EB*math.sin(anomaly))
                label=iso(J2000+timedelta(seconds=tdb))
            labels.append(label)
        groups=[]
        for i,label in enumerate(labels):
            if i==len(labels)-1 or labels[i+1]!=label:groups.append(dict(epoch=label,marker=i+1))
        cases.append(dict(system=system,input_a1_mjd=21545.,offsets_seconds=[0.,.0004,.0008],expected_labels=labels,expected_groups=groups,expected_count=len(groups)))
    return dict(cases=cases,object_id='GMAT-OEM-clock-support',test_interface_profile=dict(sender='OemOutputTimeWriterTests new native fixture',receiver='RunOemOutputTimeSupport independent byte checker',object_id='GMAT-OEM-clock-support',object_name='ClockSat',originator='GMAT output-time writer support',version='1.0',center='EARTH',frame='EME2000',time_systems=SYSTEMS,A1_definition='TAI+0.0343817SIseconds',units=['km','km/s'],exchange='private unchanged local bytes,SHA256-bound'),reference=catalog()['reference'])


if __name__=='__main__':
    Path(__file__).with_name('NasaOemOutputTimeCases.json').write_text(json.dumps(catalog(),indent=2)+'\n')
    Path(__file__).with_name('OemOutputTimeSupportCases.json').write_text(json.dumps(support_catalog(),indent=2)+'\n')
