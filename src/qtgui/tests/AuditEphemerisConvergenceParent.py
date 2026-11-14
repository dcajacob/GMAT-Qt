"""Complete iterative-output conjunction from immutable execution evidence."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tarfile

KEY = 'FRR!E406'
FINGERPRINT = '0c63ed325e134f5265b5c425cbaafaa577c25540e799f7b3fbb069834ce37276'
KIND = 'converged-iterative-ephemeris-conjunction-v1'


def load(path): return json.loads(Path(path).read_text())
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def need(ok, message):
    if not ok: raise ValueError(message)


def audit_index(index, repo):
    def bound(path):
        need(str(path) in index['artifact_sha256'], 'Unbound evidence route: '+str(path))
        return Path(path)
    for path, digest in index['artifact_sha256'].items():
        need(sha(path) == digest, 'Changed execution evidence: '+path)
    old = load(bound(index['prior_scope_index']))
    need(old['requirement'] == KEY and old['fingerprint'] == FINGERPRINT, 'Wrong original obligation')
    need(set(old['formats']) == {'CCSDS-OEM','SPK','CK','Code-500','STK-TimePosVel'}, 'Incomplete writer family')
    need(len(old['saved_components']) == 12, 'Missing saved iterative-output components')
    need(set(old['solvers']) == {'DifferentialCorrector','Yukon','VF13ad','BatchEstimator',
         'BatchEstimatorInv','ExtendedKalmanFilter','Simulator'} and old['smoothers'] == ['Smoother'],
         'Historical provider inventory differs')
    prior = load(bound(index['prior_scope_verification']))
    need(prior['status'] == 'passed' and prior['frozen_CK_data_replays'] == 5 and
         prior['post_repair_targeted_passes'] == 6 and prior['original_failures_retained'],
         'Saved CK and background-recorder evidence incomplete')
    for component in old['saved_components']:
        bound(component['summary'])
        need(sha(component['summary']) == component['summary_sha256'], 'Changed saved component')
        for binding in component.get('provenance', []):
            bound(binding['path'])
            need(sha(binding['path']) == binding['sha256'], 'Changed source-bound provenance')
    for name in old['CK_evidence']:
        need(load(Path(index['prior_scope_index']).parent/name)['status'].startswith('passed'), 'CK constituent failed')

    branch = Path(index['branch_stage'])
    for name in ['frozen-readers-v1/AuditConvergedEphemerisEvidence.py','engineering-audit-v1.json',
                 'native-v2/proof.json','native-v3/proof.json','frozen-plan-v1.json',
                 'initial-guess-repair-v1.json','batch-dependent-v1/QtGui.NasaBatchOutputEdgeIterationLimit/proof.json']:
        bound(branch/name)
    code = [sys.executable, str(branch/'frozen-readers-v1/AuditConvergedEphemerisEvidence.py'), str(branch)]
    evidence = json.loads(subprocess.check_output(code, text=True))
    need(evidence == load(branch/'engineering-audit-v1.json') and evidence['selected_branch_contexts'] == 6 and
         evidence['dependent_batch_contexts'] == 3, 'Converged-only branch/batch evidence differs')

    matlab = Path(index['fmincon_stage'])
    for name in ['frozen-readers-v1/AuditFminconEphemeris.py','frozen-plan-v1.json',
                 'headless-v2/QtGui.FminconEphemeris/proof.json','independent-audit-before-v1.json',
                 'independent-audit-after-v2.json']:
        bound(matlab/name)
    code = [sys.executable,str(matlab/'frozen-readers-v1/AuditFminconEphemeris.py'),
            str(matlab/'headless-v2/QtGui.FminconEphemeris'),str(matlab/'frozen-plan-v1.json')]
    evidence = json.loads(subprocess.check_output(code, text=True))
    need(evidence == load(matlab/'independent-audit-after-v2.json') and evidence['status'] == 'passed' and
         evidence['native_missions'] == 3, 'Actual MATLAB output evidence incomplete')
    before = load(matlab/'independent-audit-before-v1.json')
    need(before['status'] == 'failed' and sum(not p['passed'] for p in before['phases']) == 2,
         'Original Fmincon terminal-status/initial-guess failures lost')

    vf = Path(index['physical_tolerance_stage'])
    for name in ['frozen-readers-v1/RunEphemerisConvergence.py','native-v3/proof.json',
                 'native-v2/independent-audit.json','vf13-tolerance-plan-v3.json']:
        bound(vf/name)
    code = ('import sys,json;from pathlib import Path;sys.path.insert(0,sys.argv[1]);'
            'from RunEphemerisConvergence import audit;'
            'print(json.dumps(audit(Path(sys.argv[2])/"native-v3",Path(sys.argv[2])/"vf13-tolerance-plan-v3.json")))')
    evidence = json.loads(subprocess.check_output([sys.executable,'-c',code,str(vf/'frozen-readers-v1'),str(vf)],text=True))
    need(evidence['status'] == 'passed' and evidence['passed'] == 1, 'Physical-tolerance public route incomplete')
    proof = load(vf/'native-v3/proof.json')['observations'][0]
    need(proof['solvers'][0]['physical_tolerances_enabled'] and proof['solvers'][0]['status'] in (1,5),
         'Tolerance request lost at actual native optimizer')
    need(load(vf/'native-v2/independent-audit.json')['status'] == 'failed', 'Original exact-status probe lost')

    with tarfile.open(bound(index['production_archive'])) as archive:
        need(set(archive.getnames()) == set(index['production_sha256']), 'Source family missing')
        sources = {}
        for name, digest in index['production_sha256'].items():
            raw = archive.extractfile(name).read();need(hashlib.sha256(raw).hexdigest() == digest, 'Changed source version')
            sources[name] = raw.decode()
    ephem = sources['src/base/subscriber/EphemerisFile.cpp']
    need('!generateInBackground &&' in ephem and '!EphemerisIterationScope::Accepted()' in ephem and
         'SolverBranchCommand::GetSubscriberRunState(state) == Gmat::SOLVING' in ephem,
         'User-output/shared-writer policy or background exemption differs')
    for name in ['Target','Optimize']:
        code = sources['src/base/command/'+name+'.cpp']
        need('EphemerisIterationScope ephemerisScope' in code and 'startMode != RUN_AND_SOLVE' in code and
             'IntegerSolverStatus' in code and 'Gmat::CONVERGED' in code, 'Missing live branch acceptance contract')
    need('status == Gmat::IN_TOLERANCE' in sources['src/base/command/Optimize.cpp'], 'Scripted-tolerance success rejected')
    estimator = sources['plugins/EstimationPlugin/src/base/command/RunEstimator.cpp']
    need('EphemerisIterationScope ephemerisScope' in estimator and 'solveMode != "Solve"' in estimator and
         '!command->theEstimator->IsOfType("BatchEstimator")' in estimator,
         'Batch convergence/noniterative-mode boundary differs')
    fmincon = sources['plugins/FminconOptimizerPlugin/src/base/solver/FminconOptimizer.cpp']
    start = fmincon.index('void FminconOptimizer::RunExternal()')
    end = fmincon.index('void FminconOptimizer::RunNominal()',start)
    need('Gmat::CONVERGED' in fmincon[start:end] and 'GmatGlobal::Instance()->SetSolverStatus' in fmincon[start:end],
         'Fmincon result is not published before accepted replay')
    context = sources['src/base/subscriber/EphemerisIterationScope.cpp']
    need('thread_local' in context and 'scopes.push_back(this)' in context and 'scopes.pop_back()' in context and
         'if (!scope->accepted(scope->owner)) return false;' in context,
         'Nested/error-unwind scope contract differs')
    need(index['providers'] == sorted(set(old['solvers'])|{'FminconOptimizer'}), 'Current runtime provider missing')
    protected = load(bound(index['protected_check']))
    need(protected['unchanged'] == 1333 and protected['original_assertions_changed'] == 0 and
         protected['authorized_display_title_delta'] == ['src/qtgui/tests/VisibleTestDisplay.py'],
         'Original assertions changed')
    return dict(status='passed',writer_formats=5,current_solver_provider_names=8,
                saved_components=12,saved_CK_replays=5,branch_contexts=6,batch_contexts=3,
                actual_matlab_contexts=3,physical_tolerance_contexts=1,
                original_failure_receipts_retained=True,original_assertions_changed=0,
                native_tests_run_by_replay=0,full_current_build_qualified=False)


def qualify(repo, path, inventory, admitted):
    q = load(path)
    need(q['contract_kind'] == KIND and q['qualified_keys'] == [KEY], 'Wrong ephemeris conjunction')
    for name, digest in q['artifact_sha256'].items():need(sha(name) == digest, 'Changed qualification artifact')
    known = {r['key']:r for r in inventory['requirements']}
    need(KEY not in admitted and known[KEY]['fingerprint'] == FINGERPRINT, 'Row duplicate or source changed')
    need(q['candidate_rows'] == [known[KEY]] and q['authority_sha256'] == inventory['sources'], 'Authority identity changed')
    result = audit_index(load(q['evidence_index']), repo)
    return q, [KEY], dict(status='passed',qualified_keys=[KEY],whole_row_conjunction=True,
                         source_fingerprints={KEY:FINGERPRINT},conjunction=result,
                         source_versioned_only=True,native_tests_run=0,
                         full_current_build_qualified=False,full_NASA_qualified=False)
