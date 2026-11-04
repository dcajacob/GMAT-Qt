"""Independent bounded January-2000 clock recipes; no GMAT converter imports."""
import datetime as dt
import math

NEW_TYPES=['A1Gregorian','TAIModJulian','TAIGregorian','TTModJulian','TTGregorian','TDBModJulian','TDBGregorian','UTCModJulian','ElapsedDays','ElapsedSecs']
RELATED_TYPES=['A1ModJulian','CurrA1MJD','UTCGregorian']
TYPES=NEW_TYPES+RELATED_TYPES
SCOPES=['root','private','shared']
BASES=[21545,21546,21547]
STEPS=[0,9,18]
A1_MINUS_TAI=.0343817
TT_MINUS_TAI=32.184
TAI_MINUS_UTC=32
DAY=86400
MJD_ZERO=dt.datetime(1941,1,5,12)


def periodic_seconds(value):
    angle=math.radians(357.5277233+35999.05034*(value-21545)/36525)
    return .001658*math.sin(angle)+.00001385*math.sin(2*angle)


def system(key):
    if key in ['CurrA1MJD','ElapsedDays','ElapsedSecs']:return 'A1'
    return key.removesuffix('Gregorian').removesuffix('ModJulian')


def to_a1(value,kind):
    if kind=='A1':return value
    if kind=='TAI':return value+A1_MINUS_TAI/DAY
    if kind=='TT':return value+(A1_MINUS_TAI-TT_MINUS_TAI)/DAY
    if kind=='UTC':return value+(A1_MINUS_TAI+TAI_MINUS_UTC)/DAY
    if kind=='TDB':return value+(A1_MINUS_TAI-TT_MINUS_TAI-periodic_seconds(value))/DAY
    raise ValueError(kind)


def gregorian(day):
    # Inputs are whole selected-system seconds,away from civil/leap boundaries.
    value=MJD_ZERO+dt.timedelta(milliseconds=round(day*DAY*1000))
    return value.strftime('%d %b %Y %H:%M:%S.')+f'{value.microsecond//1000:03d}'


def recipe(key,scope,calls=0):
    i=SCOPES.index(scope);elapsed=key.startswith('Elapsed');seconds=calls*STEPS[i] if calls else 0;value=BASES[i]+seconds/DAY;kind=system(key);a1=to_a1(value,kind);native=value+2/DAY
    expected=(seconds/DAY if key=='ElapsedDays' else seconds) if elapsed else gregorian(value) if key.endswith('Gregorian') else value
    return dict(time_system=kind,date_format='A1ModJulian' if key in ['CurrA1MJD','ElapsedDays','ElapsedSecs'] else key,selected_mjd=value,input_value=gregorian(value) if key.endswith('Gregorian') else value,expected=expected,a1_epoch=a1,initial_a1_epoch=to_a1(BASES[i],kind),native_target=gregorian(native) if key.endswith('Gregorian') else native,native_target_mjd=native,native_a1_epoch=to_a1(native,kind),native_delta_seconds=2,script_offset_seconds=seconds,value_bound=2e-5 if key=='ElapsedSecs' else 2e-10,epoch_bound_days=2e-10)
