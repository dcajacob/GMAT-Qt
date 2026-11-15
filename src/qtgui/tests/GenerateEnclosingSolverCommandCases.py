"""Supplement missing solver contexts without changing retained command oracles."""
from pathlib import Path
import copy
import hashlib
import json

ROOT = Path(__file__).resolve().parent
FAMILIES = ['BeginFiniteBurn', 'Maneuver', 'Toggle', 'Save', 'Global',
            'ClearPlot', 'PenUp', 'PenDown', 'MarkPoint', 'Write', 'CommandEcho',
            'CallBuiltinGmatFunction', 'CallGmatFunction', 'CallPythonFunction',
            'UpdateDynamicData', 'NestedEvent', 'Set', 'Navigation', 'NoOp', 'SaveMissionImmediate']


def cases():
    for family in FAMILIES:
        source_family = 'SaveMissionFollowedInside' if family == 'SaveMissionImmediate' else family
        origin = next(p for p in [ROOT/folder/(source_family+'.json') for folder in
                      ['script-event-commands', 'script-event-backends',
                       'mission-command-names', 'script-event-markers']] if p.exists())
        base = json.loads(origin.read_text())
        for context in ['Target', 'Optimize']:
            spec = copy.deepcopy(base)
            setup, body = spec['script'].split('BeginMissionSequence;', 1)
            setup += '\nCreate Variable MatrixControl;\nMatrixControl = 1;\n'
            if context == 'Target':
                setup += "Create DifferentialCorrector MatrixSolver;\nMatrixSolver.ReportFile = '@DC@';\n"
            else:
                setup += "Create Yukon MatrixSolver;\nMatrixSolver.ReportFile = '@OPT@';\n"
            for name in ['Values', 'Extra', 'Trace', 'Control']:
                setup += name+'.SolverIterations = Current;\n'
            spec['script'] = setup+'BeginMissionSequence;'+body
            spec.update(outer_context=context,
                        context_prefix=context+" 'MatrixContext' MatrixSolver {SolveMode = RunInitialGuess, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary MatrixSolver(MatrixControl = 1, {Perturbation = 0.001, Lower = 0, Upper = 5, MaxStep = 1});\n",
                        context_suffix=('Achieve MatrixSolver(MatrixControl = 1, {Tolerance = 1e-9});\nEndTarget;\n'
                                        if context == 'Target' else 'Minimize MatrixSolver(MatrixControl);\nEndOptimize;\n'),
                        supplementary_origin=str(origin.relative_to(ROOT.parents[2])),
                        supplementary_origin_sha256=hashlib.sha256(origin.read_bytes()).hexdigest())
            if family == 'NoOp':
                spec['marker_plain_control'] = False
            if family == 'SaveMissionImmediate':
                old_save = "SaveMission '@MISSION@';"
                assert spec['script'].count(old_save) == 1
                spec['script'] = spec['script'].replace(old_save, "SaveMission '@MISSION@' {ExecutionPolicy = Immediate};")
                spec['expect_saved'] = True
            # Every retained numerical expectation and bound is copied verbatim.
            for key, value in base.items():
                if key not in ['script', 'marker_plain_control'] and not (family == 'SaveMissionImmediate' and key == 'expect_saved'):
                    assert spec[key] == value
            yield family+context, spec


if __name__ == '__main__':
    destination = ROOT/'enclosing-solver-commands'
    destination.mkdir(exist_ok=True)
    created = verified = 0
    for key, spec in cases():
        path = destination/(key+'.json')
        if path.exists():
            assert json.loads(path.read_text()) == spec, 'Existing fixture differs: '+str(path)
            verified += 1
        else:
            with path.open('x') as stream:
                json.dump(spec, stream, indent=2)
                stream.write('\n')
            created += 1
    print(json.dumps(dict(created=created, unchanged=verified, native_tests_run=0)))
