"""Independent circular truth and epoch parsing for batch-estimator OEM files.

GMAT generates the noiseless measurements used by its estimator. This oracle
does not independently qualify measurement physics: it qualifies each written
Cartesian ephemeris state against a prescribed point-mass circular orbit.
"""
import datetime
import json
import math
import re
from pathlib import Path

MU = 398600.4415
RADIUS = 7000.
BASE = datetime.datetime(2012,6,10)
UTC_MJD = (BASE-datetime.datetime(1941,1,5,12)).total_seconds()/86400
TAI_OFFSET = 34.


def state(seconds):
    rate=math.sqrt(MU/RADIUS**3);speed=math.sqrt(MU/RADIUS)
    angle=rate*seconds;c=math.sqrt(.5)
    return [RADIUS*math.cos(angle),RADIUS*math.sin(angle)*c,RADIUS*math.sin(angle)*c,
            -speed*math.sin(angle),speed*math.cos(angle)*c,speed*math.cos(angle)*c]


def seconds(stamp):
    match=re.fullmatch(r'(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d)(\.\d+)?Z?',stamp)
    assert match
    whole=datetime.datetime.fromisoformat(match[1])
    return (whole-BASE).total_seconds()+float(match[2]or'0')


def fixtures():
    truth=state(0.)
    offset=[1.,-.5,.25,.0005,-.0003,.0002]
    return dict(cases=[dict(key=key,initial_guess=[t+sign*d for t,d in zip(truth,offset)])
                       for key,sign in [('Positive',1),('Negative',-1)]],
                stations=[dict(name=('North'if lat>0 else'South')+str(i),latitude=lat,longitude=i*60.)for lat in [30.,-30.]for i in range(6)],
                mu=MU,radius=RADIUS,truth=truth,epoch_utc='10 Jun 2012 00:00:00.000',
                utc_mjd=UTC_MJD,tai_minus_utc=TAI_OFFSET,a1_minus_tai=.0343817,
                a1_epoch=UTC_MJD+(TAI_OFFSET+.0343817)/86400,duration=28800,
                position_tolerance=2e-5,velocity_tolerance=2e-8,
                recovered_position_tolerance=1e-5,recovered_velocity_tolerance=1e-8,
                metadata_time_tolerance=.002,min_observations=30,min_oem_points=100,
                scope='Two biased initial Cartesian states for a noiseless eight-hour Range_Skin batch '
                'estimation of one prescribed45-degree circular Earth point-mass orbit. '
                'Independent closed-form truth for every written OEM state;source/GUI/history isolation '
                'and initial-state recovery. Explicit twelve-station30-degree latitude/60-degree longitude network;native simulator observations and shared time/frame/measurement '
                'inputs are not independent measurement physics. No SPK/Code500/STK,EKF/smoother supported '
                'format boundaries,nested/failed/stopped estimators,arbitrary dynamics/data/noise, '
                'native OS input/GPU/hardware/platform/stress/current-private NASA or full-build qualification. '
                'E406 remains uncredited while broader format/segment and estimator boundaries remain open.')


if __name__=='__main__':
    p=Path(__file__).with_name('NasaBatchEphemerisCases.json')
    p.write_text(json.dumps(fixtures(),indent=2)+'\n');print(p)
