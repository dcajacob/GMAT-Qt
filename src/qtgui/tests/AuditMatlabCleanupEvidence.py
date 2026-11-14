"""Audit failed-initialization close/recovery repair, preserving wider I/O gaps."""
import argparse
import copy
import difflib
import hashlib
import json
from pathlib import Path
import re
import tarfile
import xml.etree.ElementTree as ET
import NasaGuiCoverage as nasa
from AuditMatlabFunctionCreation import archive, assertions, load, sha
from BuildNasaCoverageExtension import require, validate_visible

PREFIX = 'QtGui.NasaMatlabFunctionIo'
CASES = ['Scalar', 'LiteralRecovery', 'LiteralClose']
DRIVER = 'src/qtgui/tests/NasaMatlabFunctionIoTests.cpp'
CORE = 'src/base/executive/Sandbox.cpp'
PATCH = '''   // Function sequences can retain calls to other sandbox functions after
   // failed initialization. Finalize them before deleting any of their
   // function or parameter owners. The main sequence is borrowed and may
   // already have been deleted by Moderator, so it cannot be used here.
   std::vector<UserDefinedFunction*> pendingFunctions;
   for (const ObjectMap *map : {&objectMap, &globalObjectMap})
      for (const auto &entry : *map)
         if (auto *function = dynamic_cast<UserDefinedFunction*>(entry.second))
            if (std::find(pendingFunctions.begin(), pendingFunctions.end(),
                          function) == pendingFunctions.end())
               pendingFunctions.push_back(function);
   for (auto *function : pendingFunctions)
      if (!function->IsFcsFinalized())
         function->Finalize(true);

'''


def check_proof(proof, case, directory=None):
    require(proof['case'] == case and proof['new_requirement_credit'] == 0, 'Wrong cleanup proof/credit')
    if case == 'LiteralClose':
        require(proof['checkpoint'] == 'matlab-function-literal-close'
                and proof['outcome'] == 'rejection-checked' and proof['close_pending'] is True
                and proof['expected_rejection'] is True
                and proof['diagnostic'] == 'FunctionRunner::ValidateInputs() function is NULL',
                'Original rejected-call/close contract changed')
        if directory:
            require(proof['diagnostic'] in (directory/'rejected-messages.txt').read_text()
                    and '[Main] = MatIo(-3.5);' in (directory/'rejected-caller.script').read_text(),
                    'Original rejected input/diagnostic absent')
        return
    require(proof['checkpoint'] == 'matlab-function-io-'+case and proof['outcome'] == 'passed'
            and proof['kind'] == 'Scalar', 'Missing typed normal/recovery proof')
    inputs = [2.5, -3, 8, 4]
    outputs = [3*x+1 for x in inputs]
    report = '2.5,-3,8,4,8.5,-8,25,13,41,3\n42,3\n'
    require([r['phase'] for r in proof['phases']] == ['initial', 'no-build-repeat', 'reopened'],
            'Missing repeat/reopen')
    for row in proof['phases']:
        require(row['inputs'] == inputs and row['outputs'] == outputs and row['report'] == report
                and row['function_calls'] == 3 and row['matlab_calls'] == 4
                and row['viewer_matches'] is True, 'Independent call/caller/raw/viewer control differs')
        if directory:
            require((directory/(row['phase']+'-report.csv')).read_text() == report
                    and load(directory/(row['phase']+'-inputs.json')) == inputs
                    and load(directory/(row['phase']+'-outputs.json')) == outputs,
                    'Raw typed artifacts differ')
    if case == 'LiteralRecovery':
        require(proof['recovery'] == dict(expected_rejection=True,
                diagnostic='FunctionRunner::ValidateInputs() function is NULL', rebuilt=True,
                initialization_failure_cleanup=True), 'Same-GUI recovery contract absent')
        if directory:
            require('[Main] = MatIo(-3.5);' in (directory/'rejected-caller.script').read_text()
                    and '[Main] = MatIo(Input);' in (directory/'recovered-caller.script').read_text()
                    and '[out] = MatIo(input);' in (directory/'recovered-Exercise.gmf').read_text(),
                    'Explicit source correction was not preserved')


def audit(repo, stage):
    inputs = load(stage/'cleanup-audit-inputs-v1.json')
    for p, h in inputs['artifact_sha256'].items():
        require(Path(p).is_file() and not Path(p).is_symlink() and sha(p) == h, 'Changed artifact '+p)
    inv = nasa.inventory(repo)
    require(inv['sources'] == inputs['authority_sha256'] and len(inv['requirements']) == 1158
            and len(inv['historical_cases']) == 2367, 'Original NASA authority changed')
    frozen = {i: archive(stage/f'executed-sources-v{i}.tar.gz',
              load(stage/f'executed-source-sha256-v{i}.json')) for i in [1, 5, 6, 7]}
    helper = load(stage/'runtime-helper-binding-v1.json')
    require(sha(helper['source_archive']) == helper['source_archive_sha256']
            == load(inputs['hold_receipt'])['source_archive_sha256'],
            'Prior sealed offline-helper source archive differs')
    with tarfile.open(helper['source_archive']) as bundle:
        require(hashlib.sha256(bundle.extractfile(helper['member']).read()).hexdigest()
                == helper['source_sha256'], 'Prior recorded helper source differs')
    before, after = frozen[5][CORE].decode(), frozen[7][CORE].decode()
    marker = '   sequence  = NULL;\n   current   = NULL;'
    # The marker also occurs elsewhere. This exact replacement is within Clear.
    at = before.index(marker, before.index('void Sandbox::Clear()'))
    require(after == before[:at]+PATCH+before[at:], 'Unexpected Sandbox core delta')
    require(frozen[5][DRIVER] == frozen[7][DRIVER], 'Before/after native assertions changed')
    diff = ''.join(difflib.unified_diff(before.splitlines(True), after.splitlines(True),
                  fromfile='before/'+CORE, tofile='after/'+CORE))
    require((stage/'Sandbox-initialization-cleanup-v2.patch').read_text() == diff,
            'Exact core increment patch differs')
    old_assertions = [a for a in assertions(frozen[1][DRIVER].decode()) if 'Unknown MATLAB I/O case' not in a]
    final_assertions = assertions(frozen[7][DRIVER].decode())
    cursor = 0
    for item in final_assertions:
        if cursor < len(old_assertions) and item == old_assertions[cursor]:
            cursor += 1
    require(cursor == len(old_assertions) == 24, 'Original runtime assertions removed or changed')
    original_modes = re.search(r'require\(QStringList\{([^}]+)\}\.contains\(mode\)', frozen[1][DRIVER].decode())[1]
    final_modes = re.search(r'require\(QStringList\{([^}]+)\}\.contains\(mode\)', frozen[7][DRIVER].decode())[1]
    require(set(re.findall(r'"([^"]+)"', final_modes)) == set(re.findall(r'"([^"]+)"', original_modes))
            | {'LiteralRecovery', 'LiteralClose'}, 'CLI census changed beyond new regression cases')
    def active(name):
        return {r['name']: r for r in load(stage/name/'results.json')['results'] if r['status'] != 'not-run'}
    before_row = active('literal-close-before-v1')[PREFIX+'LiteralClose']
    require(before_row['status'] == 'failed' and before_row['returncode'] != 0
            and 'Segmentation fault' in Path(before_row['log']).read_text(), 'Original close crash lost')
    check_proof(load(Path(before_row['log']).parent/'proof.json'), 'LiteralClose')
    original_recovery = active('literal-recovery-before-v1')[PREFIX+'LiteralRecovery']
    require(original_recovery['status'] == 'passed' and original_recovery['returncode'] == 0,
            'Original working rebuild recovery was reclassified')
    failed_repair = active('cleanup-headless-v1')
    for case in ['LiteralClose', 'LiteralRecovery']:
        row = failed_repair[PREFIX+case]
        require(row['status'] == 'failed' and 'Segmentation fault' in Path(row['log']).read_text(),
                'Failed first repair lost')
    require('FunctionManager::Cleanup()' in (stage/'literal-gdb-v1.log').read_text()
            and 'GmatFunction::~GmatFunction()' in (stage/'literal-gdb-v1.log').read_text()
            and 'Sandbox::Clear()' in (stage/'literal-close-gdb-v2.log').read_text(),
            'Both ownership-boundary debugger traces omitted')
    modes, proofs = {}, {}
    for mode, name in [('headless', 'cleanup-headless-v2'), ('visible', 'cleanup-visible-v1')]:
        receipt = load(stage/name/'results.json')
        mapped = load(stage/(name+'-loaded-images.json'))
        require(receipt['mode'] == mode and receipt.get('finished_utc')
                and receipt['end_reason'] == 'completed-selection' and mapped['returncode'] == 0
                and mapped['inputs_unchanged'] is True and mapped['runtime_binary_copies'] == 0,
                'Nonterminal/failed/mutated cleanup run')
        source_version = dict(mapped['source_sha256'], **{helper['member']: helper['source_sha256']})
        require(mapped['source_sha256'] == load(stage/'executed-source-sha256-v7.json')
                and all(source_version.get(p) == h for p, h in receipt['source_hashes'].items()),
                'Native source custody differs')
        rows = active(name)
        require(set(rows) == {PREFIX+c for c in CASES}, 'Missing/extra cleanup case')
        primary = {p['command'][-1]: p for p in mapped['processes'] if p['role'] == 'I/O-test'}
        require(set(primary) == set(CASES), 'Missing actual GUI process')
        evidence = {}
        for case in CASES:
            row = rows[PREFIX+case]
            require(row['status'] == 'passed' and row['returncode'] == 0, 'Cleanup case failed')
            directory = Path(row['log']).parent
            tests = list(ET.parse(directory/'ctest.xml').getroot().iter('testcase'))
            require(len(tests) == 1 and tests[0].get('name') == PREFIX+case
                    and not any(tests[0].find(k) is not None for k in ['failure', 'error', 'skipped']),
                    'Missing exact passing JUnit')
            proof = load(directory/'proof.json')
            records = re.findall(r'NASA_CHECK (\{[^\n]+\})', Path(row['log']).read_text())
            require(len(records) == 1 and json.loads(records[0]) == proof, 'Proof differs from actual stdout')
            check_proof(proof, case, directory)
            trace_path = directory/('trace-'+str(primary[case]['pid'])+'.jsonl')
            trace = [json.loads(l) for l in trace_path.read_text().splitlines()]
            require(trace[0]['event'] == 'start' and trace[-1]['event'] == 'summary'
                    and trace[-1]['screenshots'] == 0 and row['trace']['instrumented_processes'] == 1,
                    'Missing terminal observer; close outcome not proved')
            if mode == 'visible':
                require(sum(r['event'] == 'native_exposure' and r.get('exposed', False)
                            for r in trace) == row['trace']['native_exposure_events'] > 0,
                        'Actual observer exposure census differs')
            require(primary[case]['executable'] in row['registered_command']
                    and mapped['loaded_project_images'][primary[case]['executable']]
                    == receipt['binary_hashes'][primary[case]['executable']], 'Native executable identity differs')
            evidence[PREFIX+case] = dict(log=row['log'], log_sha256=sha(row['log']),
                junit=str(directory/'ctest.xml'), junit_sha256=sha(directory/'ctest.xml'))
            proofs[case] = proof
        image_union = {}
        for process in mapped['processes']:
            require(process['role'] in ['I/O-test', 'function-validation-worker'], 'Unexpected mapped process role')
            if process['role'] == 'function-validation-worker':
                require('--validate-functions' in process['command'], 'Unbound validation worker')
            for snap in process['snapshots']:
                require(hashlib.sha256(snap['maps'].encode()).hexdigest() == snap['maps_sha256']
                        and all(p in snap['maps'] and mapped['loaded_project_images'][p] == h
                                for p, h in snap['project_images'].items()), 'Raw mapped image binding differs')
                image_union.update(snap['project_images'])
        require(image_union == mapped['loaded_project_images'], 'Actual mapped image census differs')
        if mode == 'visible':
            validate_visible(receipt, evidence)
        modes[mode] = dict(native_GUI_processes=len(primary), validation_workers=len(mapped['processes'])-len(primary),
                          native_exposures=sum(r['trace']['native_exposure_events'] for r in rows.values()),
                          loaded_images=len(mapped['loaded_project_images']))
    old_images = load(stage/'literal-close-before-v1-loaded-images.json')['loaded_project_images']
    new_images = load(stage/'cleanup-headless-v2-loaded-images.json')['loaded_project_images']
    changed = [p for p in set(old_images)|set(new_images) if old_images.get(p) != new_images.get(p)]
    require(len(changed) == 1 and Path(changed[0]).name.startswith('libGmatBase.so'), 'Unexpected image delta')
    prior_core, core = (load(inputs[k]) for k in ['prior_core_inventory', 'core_inventory'])
    old = {r['path']: r['working_sha256'] for r in prior_core['files']}
    new = {r['path']: r['working_sha256'] for r in core['files']}
    require(len(old) == len(new) == 247 and set(old) == set(new)
            and [p for p in old if old[p] != new[p]] == [CORE], 'Unexpected pending core increment')
    negatives = []
    for label, mutate in [('wrong-result', lambda p:p['phases'][0]['outputs'].__setitem__(0, 9)),
        ('changed-caller', lambda p:p['phases'][0]['inputs'].__setitem__(0, 99)),
        ('wrong-raw', lambda p:p['phases'][0].update(report='9\n')),
        ('missing-reopen', lambda p:p['phases'].pop()),
        ('missing-leaf-call', lambda p:p['phases'][0].update(function_calls=2)),
        ('missing-recovery', lambda p:p['recovery'].update(rebuilt=False))]:
        corrupt = copy.deepcopy(proofs['LiteralRecovery']); mutate(corrupt)
        try: check_proof(corrupt, 'LiteralRecovery')
        except (ValueError, AssertionError): negatives.append(label)
        else: raise ValueError('Accepted corrupt cleanup proof '+label)
    ledger = load(inputs['canonical_ledger'])
    holds = load(inputs['hold_receipt'])['unchanged_original_hold_keys']
    remaining = {r['key'] for r in ledger['remaining_original_rows']['requirements']}
    require(inputs['original_hold_keys'] == holds and len(set(holds)) == 59
            and set(holds) <= remaining, 'Original prerequisite hold identities changed')
    require(ledger['coverage']['requirements']['verified'] == 1019
            and ledger['requirements_still_needed'] == 82 and inputs['original_hold_count'] == 59,
            'Coverage or holds changed')
    return dict(status='cleanup-repair-verified;wider-I-O-incomplete', qualified_keys=[], new_requirement_credit=0,
        modes=modes, exact_core_increment=CORE, pending_core_paths=247, unchanged_prior_core_paths=246,
        original_runtime_assertions_preserved=24, CLI_census_extensions=['LiteralRecovery', 'LiteralClose'],
        original_close_crash='retained', failed_first_repair='retained', original_rebuild_recovery='passed-before-and-after',
        negative_controls=negatives, coverage=ledger['coverage'], requirements_still_needed=82,
        original_hold_count=59, positive_probe_failures=['QuotedString', 'Object', 'Literals', 'InOut'],
        remaining_work=['Correct quoted-string fixture semantics before testing wire loss;preserve initial failure.',
            'Review formal-input alias validation without bypassing actual runtime types.',
            'Retain whole-object/literal source conflicts and all original gates.',
            'Add documented array-element and output-property witnesses;finish wider visible cases and audit.'],
        screenshots=0, runtime_binary_copies=0, kernel_copies=0, scientific_approval=False,
        technical_core_approval=False, full_current_build_qualified=False, full_NASA_qualified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(Path(__file__).resolve().parents[3], args.stage.resolve())
    require(not args.output.exists(), 'Audit output exists')
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ['status', 'modes', 'negative_controls', 'requirements_still_needed']}))
