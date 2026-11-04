"""Replay the unchanged Topocentric reopen contract without exact-row credit."""
import argparse
from collections import Counter
import copy
import difflib
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tarfile
import tempfile
from xml.etree import ElementTree

STAGE = 'build/nasa-coverage/coverage95-20261104-function-topocentric-reopen'
PRIOR = 'build/nasa-coverage/coverage95-20261104-function-clock-propagation'
LEDGER = 'build/nasa-coverage/coverage95-20261103-matlab-c-interface/coverage-snapshot-v1.json'
LEDGER_SHA = '4d7da4f31ba802900eb7a9eec2fd085dff86cd1ba365b1c23ed5959962bc34e6'

def check(ok, message):
    if not ok:
        raise ValueError(message)

def load(path):
    return json.loads(Path(path).read_text())

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def source(archive, name):
    with tarfile.open(archive) as tar:
        return tar.extractfile(name).read()

def module(archive, name):
    ns = {'__name__': 'frozen_contract', '__file__': name}
    exec(compile(source(archive, name), name, 'exec'), ns)
    return ns

def normalize(root, name):
    match = re.search(r"ReadShared\.FunctionPath = '([^']+/Owned type scope Ω)/ReadShared.gmf';",
                      (root/'caller.script').read_text())
    check(match is not None, 'Actual fixture path absent')
    return (root/name).read_text().replace(match.group(1), '${OWNED_FIXTURE_DIR}')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--stage', type=Path)
    parser.add_argument('--manifest', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    repo = args.repo.resolve()
    stage = args.stage.resolve() if args.stage else repo/STAGE
    prior = repo/PRIOR
    counts = Counter()
    if args.manifest:
        seal = load(args.manifest)
        check(sha(__file__) == seal['auditor_sha256'], 'Auditor identity differs')
        for name, digest in seal['regular_artifact_sha256'].items():
            check(not (repo/name).is_symlink() and sha(repo/name) == digest,
                  'Immutable artifact changed: '+name)
        for name, target in seal['symbolic_links'].items():
            check((repo/name).is_symlink() and str((repo/name).readlink()) == target,
                  'Temporal alias route changed')
        with tarfile.open(repo/seal['final_source_archive']) as tar:
            check(set(tar.getnames()) == set(seal['final_source_sha256']), 'Final source scope changed')
            for name, digest in seal['final_source_sha256'].items():
                check(hashlib.sha256(tar.extractfile(name).read()).hexdigest() == digest,
                      'Final reviewed source changed')
        counts['immutable_artifact_bindings'] = len(seal['regular_artifact_sha256'])
        counts['temporal_alias_routes'] = len(seal['symbolic_links'])
    check(sha(repo/LEDGER) == LEDGER_SHA, 'Canonical ledger changed')
    preparation = load(stage/'preparation-v1.json')
    check(sha(prior/'evidence-manifest-v1.json') == preparation['prior_manifest_sha256'] and
          sha(prior/'evidence-audit-v1.json') == preparation['prior_audit_sha256'],
          'Prior seal/audit changed')
    old_seal = load(prior/'evidence-manifest-v1.json')
    old_archive = repo/old_seal['final_source_archive']
    with tempfile.TemporaryDirectory(prefix='gmat-topocentric-prior-audit-') as folder:
        root = Path(folder)
        script = root/'prior.py'
        script.write_bytes(source(old_archive, 'src/qtgui/tests/AuditFunctionClockPropagation.py'))
        output = root/'audit.json'
        result = subprocess.run([sys.executable, str(script), '--repo', str(repo),
            '--stage', str(prior), '--manifest', str(prior/'evidence-manifest-v1.json'),
            '--output', str(output)], cwd=root, capture_output=True, text=True)
        check(result.returncode == 0, 'Frozen prior replay failed: '+result.stderr[-1500:])
        check(output.read_bytes() == (stage/'prior-audit-replay-v1.json').read_bytes(),
              'Frozen prior replay changed')
    old = module(old_archive, 'src/qtgui/tests/AuditFunctionOwnedTypes.py')
    custody = module(old_archive, 'src/qtgui/tests/AuditFunctionTextEphemeris.py')
    receipt = load(stage/'stage-receipt-v1.json')
    previous = load(prior/'stage-receipt-v2.json')
    for key in ['canonical_verified','original_total','target','still_needed','historical_verified',
                'historical_total','original_hold_keys','new_requirement_credit','whole_row_qualified',
                'scientific_approval','technical_approval','full_current_build_qualified','full_NASA_qualified']:
        check(receipt[key] == previous[key], 'Original target/scope/gate changed: '+key)
    check(receipt['collective_owned_types'] == 36 and receipt['remaining_owned_types'] == [] and
          len(set(receipt['original_hold_keys'])) == 59, 'Full owned type/hold census changed')
    old_rows = load(prior/'remaining-requirement-audit-v2.json')
    rows = load(stage/'remaining-requirement-audit-v1.json')
    check(len(rows['rows']) == 108 and all(rows[k] == v for k,v in old_rows.items()),
          'Original exact rows/fingerprints/gaps/conjunctions changed')
    a = load(prior/'clock-owned-type-matrix-v2.json')
    b = load(stage/'topocentric-owned-type-matrix-v1.json')
    check(len(b['rows']) == 566 and len(a['rows']) == 566, 'Full factory universe changed')
    for x,y in zip(a['rows'], b['rows']):
        check(all(y[k] == v for k,v in x.items()), 'Original role/eligibility/Global obligation changed')
    driver = 'src/qtgui/tests/NasaFunctionOwnedTypeTests.cpp'
    catalog = 'src/qtgui/tests/NasaFunctionOwnedTypeCases.json'
    native_refs = ['src/qtgui/tests/FunctionOwnedInputFixtures.hpp',
                   'src/qtgui/tests/FunctionScopeUi.hpp', driver, catalog,
                   *['src/qtgui/tests/'+n+'.cpp' for n in ['NasaOriginRelativeStateTests','NasaOriginFrameTests','NasaInertialAxesTests','NasaTopoRuntimeTests','NasaFunctionOwnedScopeTests']]]
    baseline = load(stage/'executed-source-sha256-v1.json')
    for version in range(1, receipt['source_versions']+1):
        identities = custody['archive_sources'](stage, version)
        build = load(stage/f'build-receipt-v{version}.json')
        check(build['status'] == 'passed' and build['returncode'] == 0 and
              build['inputs_unchanged'] and build['source_sha256'] == identities,
              'Build/source custody changed')
        check(all(identities[n] == baseline[n] for n in native_refs),
              'Original native program/fixtures/assertions changed')
        counts['source_capsules'] += 1
    for run,version in receipt['gui_execution_versions'].items():
        custody['custody'](load(stage/(run+'-loaded-images.json')),
            load(stage/f'build-receipt-v{version}.json'), stage/f'build-receipt-v{version}.json', counts)
    failed_root = stage/'initial-topocentric-headless-v1'/'QtGui.NasaFunctionOwnedTypeTopocentric'
    failed = load(failed_root/'result.json')
    check(failed['status'] == 'failed' and failed['returncode'] != 0 and
          failed['failure_summary'] == ['FAIL Owned Unicode reopen changed source/prototypes'] and
          load(failed_root/'reopened-diagnostic.json') == dict(loaded=True,rebuilt=True,
              source_unchanged=True,prototypes_unchanged=False), 'Initial counterexample changed')
    check(load(failed_root/'prototypes-before.json') != load(failed_root/'reopened-prototypes.json'),
          'Initial exact prototype failure absent')
    counts['retained_current_counterexamples'] += 1
    # Admit only the newly proved finite type; all original contract conditions
    # and strict numeric bounds remain the frozen predecessor's own code.
    old['OPEN_TYPES'] -= {'Topocentric'}
    specs = json.loads(source(stage/'executed-sources-v1.tar.gz', catalog))
    proofs = []
    for selected in receipt['selected']:
        root = old['invocation'](stage,selected['run'],selected['test'],selected['mode'],counts)
        proof = load(root/'proof.json')
        spec = next(c for c in specs['cases'] if c['key'] == selected['case'])
        check(spec['family'] == 'Axes' and load(root/'executed-catalog.json') == specs and
              sha(root/'proof.json') == selected['proof_sha256'], 'Original selected case/proof changed')
        check((spec['position_bound_km'],spec['velocity_bound_km_s'],spec['stop_time_bound_seconds']) ==
              (1e-7,1e-10,1e-6), 'Original numerical bounds changed')
        old['contract'](proof,spec,counts)
        check(load(root/'reopened-prototypes.json') == load(root/'prototypes-before.json'),
              'Exact fresh Build prototype equality changed')
        check(load(root/'configured-state-reopened.json') ==
              load(root/'configured-state-before.json'), 'Fresh Build state/orientation diagnostics differ')
        for phase in range(3):
            check(all(load(root/f'{phase}-invariants.json').values()), 'Original isolation invariant changed')
            check([float(x) for x in (root/f'{phase}-values.txt').read_text().split()] ==
                  proof['phases'][phase]['raw_values'], 'Actual raw report bytes changed')
        if selected['case'] == 'Topocentric':
            for name in ['caller.script','initial.gmf','edited.gmf','nested.gmf']:
                check(normalize(root,name) == normalize(failed_root,name),
                      'Counterexample script changed beyond exact temporary path')
                counts['unchanged_counterexample_scripts'] += 1
            proofs.append((proof,spec))
            counts['new_topocentric_missions'] += len(proof['phases'])
            counts['new_topocentric_live_callbacks'] += sum(len(p['live_samples']) for p in proof['phases'])
            counts['new_topocentric_components'] += sum(len(p['live_samples'])*6 for p in proof['phases'])
    for selected in receipt['regressions']:
        old['invocation'](stage,selected['run'],selected['test'],selected['mode'],counts)
    support = stage/receipt['support_run']
    execution = load(support/'execution.json')
    version = receipt['support_source_version']
    custody['custody'](execution,load(stage/f'build-receipt-v{version}.json'),
                       stage/f'build-receipt-v{version}.json',counts)
    tests = ElementTree.parse(support/'ctest.xml').findall('.//testcase')
    check(execution['returncode'] == 0 and len(tests) == 1 and
          tests[0].get('name') == 'QtGui.OriginRelativeStateContract' and
          all(tests[0].find(k) is None for k in ['error','failure','skipped']), 'Native origin-state contract failed')
    marker = re.search(r'NASA_CHECK (\{[^\n]+\})', tests[0].findtext('system-out',''))
    support_proof = json.loads(marker.group(1)) if marker else {}
    check(support_proof.get('checkpoint') == 'origin-relative-state-contract' and
          support_proof.get('outcome') == 'passed' and support_proof.get('checks') == 96,
          'Original origin-state assertions absent')
    counts['native_origin_state_checks'] = support_proof['checks']
    for damage in [lambda p:p['phases'][0]['live_samples'].pop(),
                   lambda p:p['phases'][0]['live_samples'][1].update(shared_pointer='0'),
                   lambda p:p['phases'][0]['live_samples'][1]['resource'].update(automatic=False),
                   lambda p:p['phases'][0]['live_samples'][1]['resource'].update({'global':False}),
                   lambda p:p['phases'][0]['live_samples'][0]['actual'].__setitem__(0,0),
                   lambda p:p['phases'][0]['raw_values'].__setitem__(0,0),
                   lambda p:p.update(whole_row_qualified=True)]:
        proof,spec = proofs[0]
        bad = copy.deepcopy(proof)
        damage(bad)
        rejected = False
        try:
            old['contract'](bad,spec,Counter())
        except (ValueError,KeyError,IndexError):
            rejected = True
        check(rejected, 'Damaged scoping proof escaped')
        counts['damaged_proofs_rejected'] += 1
    core = load(stage/'core-increment-v1.json')
    patch = []
    for name,record in sorted(core['paths'].items()):
        before = stage/'core-preimages-v1'/name
        after = stage/'core-postimages-v1'/name
        check(sha(before) == baseline[name] == record['before_sha256'] and
              sha(after) == load(stage/'executed-source-sha256-v2.json')[name] == record['after_sha256'],
              'Exact core pre/postimage changed')
        patch += difflib.unified_diff(before.read_text().splitlines(True),after.read_text().splitlines(True),
                                     fromfile='a/'+name,tofile='b/'+name)
    check(''.join(patch) == (stage/'core-increment-v1.patch').read_text(), 'Exact core patch changed')
    inventory = load(stage/'core-inventory-v1/inventory.json')
    old_inventory = load(prior/'core-inventory-v2/inventory.json')
    old_files = {f['path']:f['working_sha256'] for f in old_inventory['files']}
    new_files = {f['path']:f['working_sha256'] for f in inventory['files']}
    check(len(new_files) == len(old_files) == 279 and set(new_files) == set(old_files) and
          {n for n in new_files if new_files[n] != old_files[n]} == set(core['paths']),
          'Pending core inventory changed outside exact increment')
    processes = load(stage/'recorded-process-audit-v1.json')
    check(not processes['matching_native_processes'] and not processes['matching_visible_servers'] and
          len(processes['recorded_native_identities']) == counts['mapped_native_processes'],
          'Recorded terminal process census differs')
    result = dict(status='passed-bounded-topocentric-reopen-audit',counts=dict(counts),
        canonical_verified=1050,original_total=1158,target=1101,still_needed=51,
        open_exact_rows=108,original_holds=59,collective_owned_types=36,total_owned_types=36,
        remaining_owned_types=[],pending_core_paths=279,new_requirement_credit=0,
        whole_row_qualified=False,scientific_technical_current_build_NASA_qualified=False)
    check(not args.output.exists(), 'Never overwrite an audit')
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))

if __name__ == '__main__':
    main()
