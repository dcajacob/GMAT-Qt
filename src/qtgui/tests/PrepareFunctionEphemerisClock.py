"""Generate supplemental GT controls without changing the canonical 32 cases."""
import argparse
import copy
from decimal import Decimal, localcontext
import hashlib
import json
from pathlib import Path

PROFILES={'FromSpacecraft':('FromSpacecraft',30),'EphemStart':('EphemStart',0),'ScriptStart':('01 Jan 2000 12:00:30.000',30),'RealScriptStart':('01 Jan 2000 12:00:30.000',30),'FractionLittle':('EphemStart',.123456789),'FractionBig':('EphemStart',.123456789)}
FORMATS=['Code500','STK','CCSDS-OEM']
def generate(repo,profile):
    path=repo/'src/qtgui/tests/NasaFunctionOwnedTypeCases.json'
    baseline=json.loads(path.read_text());data=copy.deepcopy(baseline)
    selector,offset=PROFILES[profile]
    formats=['Code500']if profile.startswith('Fraction')else FORMATS
    for case in data['cases']:
        if case['key']not in formats:continue
        case['real_clock_profile'if profile=='RealScriptStart'else'clock_profile']=profile
        if profile!='RealScriptStart':case['precision_stop_tolerance_seconds']=1e-7
        case['expected_start_selector']=selector
        case['expected_start_offset_seconds']=offset
        if profile.startswith('Fraction'):
            case['file_clock_shift_seconds']=offset
            case['file_output_endian']=1 if profile=='FractionLittle'else 2
        for scope,name in [('root','Shared'),('private','FunctionOwned')]:
            key=scope+'_configuration'
            case[key]=case[key].replace(".StartEpoch = 'FromSpacecraft';",f".StartEpoch = '{selector}';")
            if profile in ['ScriptStart','RealScriptStart']:case[key]+=f'{name}.EpochFormat = UTCGregorian;\n'
            owner='MainSC'if scope=='root'else'FunctionSC'
            if profile.startswith('Fraction'):
                # Gregorian input is millisecond-limited; the supported precise
                # MJD string denotes the same authored nanosecond epoch.
                with localcontext() as context:
                    context.prec=60
                    epoch=format(Decimal(21545)+Decimal(str(offset))/86400,'.26f')
                case['supplemental_'+scope+'_spacecraft_configuration']=f"{owner}.DateFormat = UTCModJulian;\n{owner}.Epoch = '{epoch}';\n"
            else:
                clock=f'{offset:02d}.000'
                case['supplemental_'+scope+'_spacecraft_configuration']=f"{owner}.DateFormat = UTCGregorian;\n{owner}.Epoch = '01 Jan 2000 12:00:{clock}';\n"
    data['supplemental_clock_control']=dict(profile=profile,formats=formats,canonical_catalog=str(path.relative_to(repo)),canonical_catalog_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),all_original_numeric_bounds_preserved=True,original_default_clock_cases_reused=True,new_requirement_credit=0,whole_row_qualified=False)
    return data
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();repo=Path(__file__).resolve().parents[3]
    args.output.mkdir(parents=True,exist_ok=False)
    for profile in PROFILES:
        (args.output/f'NasaFunctionEphemerisClock{profile}Cases.json').write_text(json.dumps(generate(repo,profile),indent=2)+'\n')
if __name__=='__main__':main()
