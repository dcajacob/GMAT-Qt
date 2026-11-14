"""Missing ScriptEvent command contexts, reusing the existing circle fixture.

These are command-support/effect contracts, not full model qualification or
whole FRC!E138 credit. Existing command fixtures and six branch tests are intact.
"""
import argparse
import hashlib
import json
from pathlib import Path


def cases():
    root = Path(__file__).resolve().parent
    source = root/'mission-command-names/Propagate.json'
    old = json.loads(source.read_text())
    setup, body = old['script'].split('BeginMissionSequence;')
    lines = body.strip().splitlines()
    props = [line for line in lines if line.startswith('Propagate ')]
    report = next(line for line in lines if line.startswith("Report 'Witness' Values "))
    assert len(props) == 2
    base = '\n'.join(props)+'\n'
    values = []

    def add(family, command, commands, tail=(0, 1, 0, 0), extra='', **flags):
        values.append(dict(family=family, command=command,
            script=setup+extra+'BeginMissionSequence;\n'+commands,
            expected_tail=list(tail), velocity_increment=[0, 0, 0],
            fixture_origin='mission-command-names/Propagate.json',
            fixture_origin_sha256=hashlib.sha256(source.read_bytes()).hexdigest(), **flags))

    add('SaveMissionTerminal', 'SaveMission', base+report+"\nSaveMission '@MISSION@';\n",
        extra='Sat.NAIFId = -9104001;\nSat.NAIFIdReferenceFrame = -9104002;\n',
        mission_save=True, expect_saved=True, plain_control=True)
    add('SaveMissionNested', 'SaveMission', base+report+
        "\nBeginScript 'Inner';\nSaveMission '@MISSION@';\nEndScript;\n",
        mission_save=True, expect_saved=True, marker_pairs=2)
    add('SaveMissionFollowedInside', 'SaveMission', base+
        "SaveMission '@MISSION@';\nCount = 1;\n"+report+'\n', (1, 1, 0, 0),
        mission_save=True, expect_saved=False)
    add('SaveMissionFollowedOutside', 'SaveMission', base+"SaveMission '@MISSION@';\n",
        (0, 1, 0, 3), mission_save=True, expect_saved=False,
        suffix='Quantity = 3; % meaningful following main command\n'+report+'\n% Outside event Ω\n')
    add('SaveMissionBranch', 'SaveMission', base+report+
        "\nIf Keep == 41;\nSaveMission '@MISSION@';\nEndIf;\n",
        mission_save=True, expect_saved=False)
    function = 'function [out] = Accept(input)\nCreate Variable out;\nBeginMissionSequence;\nout = input * input + 3;\n'
    declaration = "Create GmatFunction Accept;\nAccept.FunctionPath = '@FUNCTION@';\n"
    add('SaveMissionFunction', 'CallGmatFunction', base+'[Cost] = Accept(U);\n'+report+'\n',
        (0, 1, 4, 0), extra=declaration, mission_save=True, expect_saved=False,
        support_files=[dict(name='Accept.gmf', text=function+"SaveMission '@MISSION@';\n")])
    add('NestedEvent', 'BeginScript', base+
        "Count = 1;\nBeginScript 'Inner';\nCount = Count + 2;\nEndScript;\nCount = Count * 4;\n"+report+'\n',
        (12, 1, 0, 0), marker_pairs=2)
    add('Stop', 'Stop', base+report+
        "\nStop 'Stop inside event';\nCount = 999;\nReport Extra Count;\n",
        expected_outcome='Stopped', skipped_extra_report=True)
    for command in ['ClearPlot', 'PenUp', 'PenDown', 'MarkPoint']:
        start='PenUp Trace;\n' if command == 'PenDown' else ''
        add(command, command, start+props[0]+'\n'+command+' Trace;\n'+props[1]+'\n'+report+'\n')
    add('Write', 'Write', base+'Write EventText {Style = Concise};\n'+report+'\n',
        extra="Create String EventText;\nEventText = 'GUI_EVENT_WRITE_41_Ω';\n",
        expected_message='GUI_EVENT_WRITE_41_Ω')
    add('CommandEcho', 'CommandEcho', base+
        'CommandEcho On;\nReport Extra Keep;\nCommandEcho Off;\n'+report+'\n')
    builtin = json.loads((root/'NasaFunctionBuiltinCases.json').read_text())['cases'][0]
    declarations = builtin['declarations'].replace('NativeHelper', 'QuatHelper').replace('P[1,4]', 'Product[1,4]')
    add('CallBuiltinGmatFunction', 'CallBuiltinGmatFunction', base+
        'Product = QuatHelper(Qx, Qy);\nCount = Product(1,1);\nU = Product(1,2);\nCost = Product(1,3);\nQuantity = Product(1,4);\n'+report+'\n',
        (0.5, 0.5, 0.5, 0.5), extra=declarations,
        oracle='Existing literal unit-quaternion product [0.5,0.5,0.5,0.5]; no unique inverse/physics claim.')
    add('CallGmatFunction', 'CallGmatFunction', base+'[Cost] = Accept(U);\n'+report+'\n',
        (0, 1, 4, 0), extra=declaration, support_files=[dict(name='Accept.gmf', text=function)])
    add('CallPythonFunction', 'CallPythonFunction', base+'U = -7;\n[Cost] = Python.builtins.abs(U);\n'+report+'\n',
        (0, -7, 7, 0))
    add('UpdateDynamicData', 'UpdateDynamicData', base+
        'Count = 7;\nUpdateDynamicData Display;\nCount = 99;\n'+report+'\n', (99, 1, 0, 0),
        extra='Create DynamicDataDisplay Display;\nDisplay.AddParameters = {1, Keep, Count};\n')
    return values


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output-dir', type=Path, required=True)
    a = p.parse_args()
    a.output_dir.mkdir(parents=True, exist_ok=True)
    for case in cases():
        destination = a.output_dir/(case['family']+'.json')
        if destination.exists():
            raise SystemExit('Refusing to overwrite '+str(destination))
        destination.write_text(json.dumps(case, indent=2)+'\n')
    print('Generated', len(cases()), 'additional command contexts; existing fixtures unchanged')
