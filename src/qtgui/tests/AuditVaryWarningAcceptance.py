"""Admit FRC!E52 from exact source-versioned native Vary warning evidence.

The literal DEFERRED requirement is retained. Generic capability combinations
qualify warning dispatch, not unavailable proprietary solver numerics or MATLAB.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile
import xml.etree.ElementTree as ET

import NasaGuiCoverage as nasa
from BuildNasaCoverageExtension import sha, load, require, validate_visible

KEYS = ['FRC!E52']
FIELDS = ['Perturbation', 'Lower', 'Upper', 'MaxStep', 'AdditiveScaleFactor', 'MultiplicativeScaleFactor']
GROUPS = [1, 2, 2, 4, 8, 8]
GUI_CASES = ['Explicit', 'Omitted', 'DefaultExplicit', 'DCSupported', 'YukonUpper', 'YukonLower']
REGRESSIONS = ['QtGui.NativeNasaVF13ad' + x for x in ['Initial', 'Perturbation', 'MaxStep', 'Additive', 'Multiplicative']] + ['QtGui.NasaVaryParameter']
SELECTED = ['QtGui.NasaVaryWarningGui', 'QtGui.NasaVaryWarningContract'] + REGRESSIONS


def check_matrix(proof):
    require(proof['outcome'] == 'passed' and proof['mode'] == 'Contract', 'Contract proof absent')
    records = proof['records']
    require(len(records) == 114, 'Incomplete capability/lifecycle census')
    index = 0
    for mask in range(16):
        for field in [-1] + list(range(6)):
            expected = [] if field == -1 or mask & GROUPS[field] else [FIELDS[field]]
            require(records[index] == dict(mask=mask, field='omitted' if field == -1 else FIELDS[field], warnings=expected), 'Independent capability warning matrix differs')
            index += 1
    phases = [('all-options', FIELDS), ('clone-after-serialization', FIELDS), ('assignment', FIELDS), ('omitted', []), ('assignment-clears-options', []), ('reparse-clears-options', []), ('string-setter', ['Lower']), ('numeric-setter', ['Lower'])]
    require(records[112] == {'lifecycle': [dict(phase=p, warnings=w) for p, w in phases]}, 'Authored option lifetime lost')
    census = records[113]['factory_census']
    actual = {r['type']: r['capabilities'] for r in census if r['vary_eligible']}
    require(actual == {'DifferentialCorrector': [True]*6, 'VF13ad': [True, False, False, True, True, True], 'Yukon': [True]*6}, 'Uncovered registered solver or incorrect capability declaration')
    require(len(census) == len({r['type'] for r in census}), 'Duplicate factory census')
    return actual


def validate_values(rows, final=False, lower=False):
    require(bool(rows) and (not final or len(rows) == 1), 'Missing numeric witness')
    for row in rows:
        require(len(row) == 3 and all(math.isfinite(x) for x in row), 'Malformed numeric witness')
        x, cost, sentinel = row
        require(abs(cost-(x-3)**2) < 1e-8 and sentinel == 41, 'Independent quadratic or sentinel differs')
    if final:
        require(abs(rows[0][0]-(4 if lower else 2)) < 1e-6 and abs(rows[0][1]-1) < 2e-6, 'False scalar bound optimum')


def check_gui(proof, directory=None):
    require(proof['outcome'] == 'passed' and proof['mode'] == 'Gui', 'GUI proof absent')
    require([(r['case'], r['phase']) for r in proof['records']] == [(c, p) for c in GUI_CASES for p in ['first', 'repeat']], 'Incomplete actual solver/repeated Run witnesses')
    for row in proof['records']:
        case, phase = row['case'], row['phase']
        supported = case == 'DCSupported' or case.startswith('Yukon')
        typ = 'DifferentialCorrector' if case == 'DCSupported' else 'Yukon' if case.startswith('Yukon') else 'VF13ad'
        expected = ['Lower', 'Upper'] if case in ['Explicit', 'DefaultExplicit'] else []
        require(row['warnings'] == expected and row['capabilities'] == ([True]*6 if supported else [True,False,False,True,True,True]), 'Missing or spurious GUI warning/capability')
        result = row['result']
        validate_values(result['final'], True, case == 'YukonLower')
        trials = result['trials'] if supported else result['raw_trials']
        validate_values(trials)
        require(len(trials) >= 2 and abs(trials[0][0]-(5 if case == 'YukonLower' else 1)) < 1e-8, 'Wrong initial trial')
        if directory is not None:
            for name, data in [('trials', trials), ('final', result['final'])]:
                raw = [[float(x) for x in line.split(',')] for line in (directory/f'{case}-{phase}.{name}.csv').read_text().splitlines()]
                require(raw == data, 'Proof diverges from raw solver output')
            messages = (directory/f'{case}-{phase}-messages.txt').read_text()
            for field in FIELDS:
                warning = f"*** Warning *** Vary for 'X': setting '{field}' is not supported by solver 'Opt' ({typ})."
                require(messages.count(warning) == (1 if field in expected else 0), 'Actual displayed warning differs')
            require(messages.count('is not supported by solver') == len(expected), 'Unexpected displayed warning')
            script = (directory/f'{case}.script').read_text()
            if case.startswith('Yukon'):
                require('NonlinearConstraint' not in script and ('Lower = 4, Upper = 100' if case == 'YukonLower' else 'Lower = -100, Upper = 2') in script, 'Yukon bound witness uses a substitute constraint')


def negative_controls(gui, contract):
    checks = []
    for name, original, mutate, validate in [
        ('missing-warning', gui, lambda p: p['records'][0].update(warnings=[]), check_gui),
        ('spurious-omitted-warning', gui, lambda p: p['records'][2].update(warnings=['Lower']), check_gui),
        ('false-interior-optimum', gui, lambda p: p['records'][8]['result'].update(final=[[1.5,2.25,41]]), check_gui),
        ('lost-clone-provenance', contract, lambda p: p['records'][112]['lifecycle'][1].update(warnings=[]), check_matrix),
        ('unsupported-dispatch-missing', contract, lambda p: p['records'][1].update(warnings=[]), check_matrix),
        ('factory-census-incomplete', contract, lambda p: p['records'][113]['factory_census'].pop(), check_matrix),
    ]:
        changed = copy.deepcopy(original); mutate(changed)
        try: validate(changed)
        except (ValueError, AssertionError): checks.append(name)
        else: raise ValueError('Auditor accepted negative control: '+name)
    return checks


def qualify(repo, path, inventory):
    q = load(path)
    require(q['contract_kind'] == 'source-versioned-Vary-unsupported-setting-warning' and q['qualified_keys'] == KEYS, 'Changed warning scope')
    require(q['authority_sha256'] == inventory['sources'], 'Original NASA authority changed')
    known = {r['key']: r for r in inventory['requirements']}
    require(q['original_requirements'] == [dict(key=k, fingerprint=known[k]['fingerprint'], source_text=known[k]['text']) for k in KEYS], 'Original warning obligation changed')
    require(all(q.get(k) is False for k in ['technical_core_approval', 'full_current_build_qualified', 'full_NASA_qualified']) and q['deferred_wording_preserved'], 'Qualification exceeds scope')
    for name, digest in q['artifact_sha256'].items(): require(sha(name) == digest, 'Changed warning artifact: '+name)
    def bound(p):
        require(str(p) in q['artifact_sha256'], 'Consumed artifact omitted: '+str(p)); return Path(p)
    before, after = (load(bound(q[k])) for k in ['inputs_before', 'inputs_after'])
    require(before == after and load(bound(q['launch']))['inputs_unchanged'], 'Inputs changed during native execution')
    with tarfile.open(bound(q['source_archive'])) as tar:
        frozen = {m.name: tar.extractfile(m).read() for m in tar.getmembers()}
    require({p:hashlib.sha256(b).hexdigest() for p,b in frozen.items()} == before['sources'], 'Executed source archive differs')
    guide = ET.fromstring(frozen['doc/help/src/Command_Vary.xml'])
    table = next(x for x in guide.iter() if x.attrib.get('{http://www.w3.org/XML/1998/namespace}id') == 'VaryCommandOptions')
    rows = [list(x) for x in table.iter() if x.tag.split('}')[-1] == 'row']
    options = {''.join(row[0].itertext()).strip(): ['X' in ''.join(c.itertext()) for c in row[1:]] for row in rows[1:]}
    declared = {t:[options[f][i] for f in FIELDS] for t,i in [('DifferentialCorrector',0),('VF13ad',2),('Yukon',4)]}
    require(declared == {'DifferentialCorrector':[True]*6,'VF13ad':[True,False,False,True,True,True],'Yukon':[True]*6}, 'Independent documented capability table changed')
    mapped = load(bound(q['loaded_images']))['processes']; gui = contract = None
    for mode in ['headless', 'visible']:
        receipt = load(bound(q['runs'][mode])); require(receipt['mode'] == mode and receipt.get('finished_utc') and receipt['end_reason'] == 'completed-selection', 'Nonterminal native invocation')
        require(all(before['sources'].get(p) == h for p,h in receipt['source_hashes'].items()), 'Native source version differs')
        active = {r['name']:r for r in receipt['results'] if r['status'] != 'not-run'}
        require(set(active) == set(SELECTED), 'Incomplete affected selection')
        evidence = {}
        for name, row in active.items():
            log = bound(row['log']); junit = bound(log.parent/'ctest.xml'); proof_path=log.parent/'proof.json'
            require(row['status'] == 'passed' and row['returncode'] == 0, 'Failed native case')
            cases = ET.parse(junit).getroot().findall('testcase')
            require(len(cases) == 1 and cases[0].attrib['name'] == name and cases[0].attrib.get('status') == 'run' and cases[0].find('failure') is None and cases[0].find('skipped') is None, 'Exact passing JUnit absent')
            executables = [p for p in row['registered_command'] if p in receipt['binary_hashes']]
            require(len(executables) == 1 and before['images'].get(executables[0]) == receipt['binary_hashes'][executables[0]], 'Executable identity differs')
            evidence[name] = dict(log=str(log),log_sha256=sha(log),junit=str(junit),junit_sha256=sha(junit))
            if mode == 'visible':
                traces = list(log.parent.glob('trace-*.jsonl')); require(traces, 'Native exposure trace absent')
                for trace in traces:
                    bound(trace); process=mapped.get(trace.stem.removeprefix('trace-'))
                    require(process and process['executable'] == executables[0], 'Actual process image mapping absent')
                    require(process['images'].get(executables[0]) == before['images'][executables[0]], 'Mapped executable differs')
                    for image,digest in before['images'].items():
                        if Path(image).name.startswith(('libGmatBase','libGmatUtil','libYukon')):
                            require(process['images'].get(image) == digest, 'Mapped engine/plugin differs')
                    require(any('libVF13adOptimizer' in p for p in process['images']), 'Third-party VF13ad not loaded')
            if name in SELECTED[:2]:
                proof = load(bound(proof_path))
                # Native stdout, persisted proof and independent reader must agree.
                match = re.findall(r'NASA_CHECK (\{[^\n]+\})', log.read_text())
                require(len(match) == 1 and json.loads(match[0]) == proof, 'Proof differs from native stdout')
                for artifact in log.parent.iterdir():
                    if artifact.is_file(): bound(artifact)
                if name.endswith('Gui'): check_gui(proof,log.parent); gui=proof
                else: require(check_matrix(proof) == declared, 'Factory/documented capability mismatch'); contract=proof
        if mode == 'visible': validate_visible(receipt,evidence)
    controls = negative_controls(gui,contract)
    # Reuse the fourteen passing subcases from a mixed invocation; its four
    # missing executables remain not-run. The separate built support run closes them.
    support = {}
    for filename in q['support_junit']:
        for case in ET.parse(bound(filename)).getroot().findall('testcase'):
            if case.attrib.get('status') == 'run' and case.find('failure') is None and case.find('skipped') is None:
                require(case.attrib['name'] not in support, 'Duplicate support result'); support[case.attrib['name']] = case.findtext('system-out','')
    require(len(support) == 18, 'Affected Yukon support conjunction incomplete')
    for kind, value in [('Upper',2),('Lower',4)]:
        text = support['QtAcceptance.NasaYukonParallel'+kind+'BoundSteps']
        require(f'x={value} cost=1 state=1' in text and 'PASS YUKON_VARIABLE_BOUND' in text, 'Analytic MaxStep/bounds regression failed')
    # Explicitly retain the two pre-fix false-convergence witnesses.
    failed = ET.parse(bound(q['prefixed_yukon_junit'])).getroot().findall('testcase')
    require(len(failed) == 2 and all(x.find('failure') is not None for x in failed), 'Pre-fix failures erased')
    q['audit_summary'] = dict(matrix_cases_per_mode=112, lifecycle_cases_per_mode=8, gui_runs_per_mode=12, original_gui_regressions_per_mode=6, yukon_support_cases=18, negative_controls=controls)
    return q, KEYS


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--qualification',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();repo=Path(__file__).resolve().parents[3]
    q,keys=qualify(repo,args.qualification,nasa.inventory(repo))
    args.output.write_text(json.dumps(dict(status='passed',qualified_keys=keys,summary=q['audit_summary'],technical_core_approval=False,full_NASA_qualified=False),indent=2)+'\n')
    print(json.dumps(q['audit_summary']))
