"""Public named-spacecraft epoch ABI and sandbox lifetime acceptance (no GUI)."""
import argparse
from pathlib import Path
import uuid
import RunCapiAcceptance as base


def fixtures():
    primary = base.fixture('stm').replace("A.Epoch = '21545'", "A.Epoch = '21545.125'").replace("B.Epoch = '21545'", "B.Epoch = '21600.875'")
    prefix, mission = primary.split('BeginMissionSequence;')
    unpropagated = "Create Spacecraft C;\nC.DateFormat = A1ModJulian;\nC.Epoch = '22000.0625';\n"
    prefix += unpropagated
    primary = prefix + 'BeginMissionSequence;' + mission
    first = "Propagate P1(A) {A.ElapsedSecs = 10};\n"
    assets = {
        'prepare.script': prefix + 'PrepareMissionSequence;' + mission,
        'stopped.script': prefix + 'BeginMissionSequence;\n' + first + 'Stop;\n' + mission,
        'runtime-error.script': prefix + 'Create Variable Value;\nValue = -1;\nBeginMissionSequence;\n' + first + 'Value = sqrt(Value);\n' + mission,
        'initialization-error.script': prefix + 'FM1.PointMasses = {};\nBeginMissionSequence;' + mission,
        'replacement.script': prefix.replace(unpropagated, '').replace("'21545.125'", "'23000.25'").replace("'21600.875'", "'24000.5'") + 'BeginMissionSequence;\nPropagate BackProp P1(A) {A.ElapsedSecs = -20};\n',
        'assigned.script': prefix + 'BeginMissionSequence;\nA.A1ModJulian = 25000.375;\nB.A1ModJulian = 26000.625;\n',
    }
    return primary, assets


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    p.add_argument('--executable', type=Path, required=True)
    dest = p.add_mutually_exclusive_group(required=True)
    dest.add_argument('--output', type=Path)
    dest.add_argument('--output-root', type=Path)
    a = p.parse_args()
    script, assets = fixtures()
    output = a.output or a.output_root / ('spacecraftepoch-' + uuid.uuid4().hex[:12])
    raise SystemExit(base.run(a.repo.resolve(), a.executable, 'spacecraftepoch', output,
                             fixture_text=script, fixture_assets=assets))
