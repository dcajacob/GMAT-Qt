"""Replay the frozen STM base-alignment/GUI evidence without native execution."""
from pathlib import Path
import hashlib
import json
import math
import re
import tarfile
import tempfile

from RunStmBaseAlignment import audit as matrix_audit

load = lambda p: json.loads(Path(p).read_text())
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
TYPES = ['OrbitSTM', 'OrbitSTMA', 'OrbitSTMB', 'OrbitSTMC', 'OrbitSTMD']


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sources(stage, version):
    ids = load(stage / ('source-sha256-' + version + '.json'))
    with tarfile.open(stage / ('sources-' + version + '.tar.gz')) as bundle:
        need(set(ids) == set(bundle.getnames()), 'Executed source inventory differs')
        for n, h in ids.items():
            need(hashlib.sha256(bundle.extractfile(n).read()).hexdigest() == h, 'Executed source bytes differ: ' + n)
    return ids


def gui_audit(root, mode):
    receipt = load(root / 'results.json')
    rows = [v for v in receipt['results'] if v['status'] != 'not-run']
    need(receipt['mode'] == mode and len(rows) == 1 and rows[0]['name'] == 'QtGui.StmSelectedFrame' and
         rows[0]['status'] == 'passed' and rows[0]['returncode'] == 0, 'Wrong GUI selection or result')
    row = rows[0]
    command = row['registered_command']
    need(any(str(v).endswith('/RunNativeTest.py') for v in command) and 'QT_QPA_PLATFORM=xcb' in command,
         'GUI proof did not use the native launcher')
    if mode == 'visible':
        display = receipt.get('visible_display', {})
        need(receipt['display'] and receipt['display_route'] == 'nested' and
             display.get('route') == 'nested Xephyr window on existing desktop' and
             display.get('test_display') != display.get('host_display') and display.get('server_pid') and
             display.get('host_window'), 'Visible proof did not use the watchable desktop')
    folder = root / 'QtGui.StmSelectedFrame'
    proof = load(folder / 'proof.json')
    logged = [json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})', (folder / 'test.log').read_text())]
    need(proof in logged and proof['outcome'] == 'passed', 'GUI proof differs from terminal native stdout')
    need(proof['native_missions'] == 1 and proof['native_propagations'] == 0 and proof['owner_unchanged'] and
         proof['save_reopen'] and proof['runtime_frame'] == 'ICRF' and proof['runtime_keep'] == 42,
         'GUI execution scope or continuation differs')
    need([v['type'] for v in proof['transactions']] == TYPES and [v['type'] for v in proof['matrices']] == TYPES,
         'GUI parameter inventory incomplete')
    for transaction in proof['transactions']:
        need(transaction['reference'] == 'Sat.ReviewFrame.' + transaction['type'] and
             all(transaction[v] for v in ['cancel', 'pending', 'apply', 'undo', 'redo']), 'GUI transaction incomplete')
    objects = row['trace']['named_objects']
    for name in ['reportPropertyObject', 'reportPropertyType', 'reportPropertyDependency']:
        need('input' in objects[name]['events'], 'Actual GUI combo input witness missing: ' + name)
    matrices, count = [], 0
    for k, matrix in enumerate(proof['matrices']):
        size = 6 if k == 0 else 3
        r, col = (0, 0) if k == 0 else (3*((k-1)//2), 3*((k-1)%2))
        raw = [float(v) for v in (folder / (matrix['type'] + '.txt')).read_text().split()]
        need(raw == matrix['values'] and len(raw) == size*size and matrix['readonly_viewer'], 'GUI raw matrix/viewer evidence differs')
        need('shown' in objects['report:R' + str(k)]['events'], 'Corresponding GUI report viewer was not shown')
        need(all(math.isfinite(v) and abs(v - float(i//size + r == i%size + col)) <= 1e-12
                 for i, v in enumerate(raw)), 'GUI fresh-identity/partition oracle failed')
        matrices.append(raw); count += len(raw)
    for k in range(1, 5):
        r, c = 3*((k-1)//2), 3*((k-1)%2)
        need(all(matrices[k][3*i+j] == matrices[0][6*(i+r)+j+c] for i in range(3) for j in range(3)),
             'GUI full/partition consistency failed')
    initial, final = (folder/'initial.script').read_text(), (folder/'mission.script').read_text()
    for k, name in enumerate(TYPES):
        initial = initial.replace(' R' + str(k) + ' Sat.' + name + ';', ' R' + str(k) + ' Sat.ReviewFrame.' + name + ';')
    need(initial == final, 'GUI accepted source changed unrelated statements')
    return dict(status='passed', profile=mode, parameter_transactions=5, matrix_entries=count,
                native_missions=1, native_propagations=0, readonly_viewers=5, native_tests_run_by_replay=0)


def audit(repo, stage):
    repo, stage = Path(repo), Path(stage)
    plan = load(stage/'frozen-plan-v1.json')
    need(plan['expected_reads'] == 75 and plan['expected_contexts'] == 15 and plan['expected_values'] == 1080 and
         plan['matrix_tolerance'] == 1e-9 and plan['new_requirement_credit'] == 0, 'Frozen obligation/bounds changed')
    versions = {v: sources(stage, v) for v in ['before-v1', 'after-v1', 'gui-v1', 'gui-v2', 'gui-v3']}
    first, final = versions['before-v1'], versions['after-v1']
    need(first.keys() == final.keys() and {n for n, h in first.items() if final[n] != h} ==
         {'src/base/parameter/OrbitStmFrame.hpp'}, 'Unrelated production or frozen native test input changed')
    for v, ids in versions.items():
        build = load(stage / ('build-' + v + '.json'))
        need(build['inputs_unchanged'] and build['source_sha256'] == ids and
             build['log_sha256'] == sha(stage / ('build-' + v + '.log')), 'Build input/log guard differs')
        need(build['returncode'] == (1 if v == 'gui-v1' else 0) and
             build['status'] == ('failed' if v == 'gui-v1' else 'passed'), 'Original compile disposition changed')
    for v in ['gui-v1', 'gui-v2', 'gui-v3']:
        need(all(versions[v][n] == h for n, h in final.items() if n != 'src/qtgui/CMakeLists.txt'),
             'Production or native mathematical inputs changed after passing reads')
    fixture_name = 'src/qtgui/tests/StmBaseAlignmentCases.json'
    with tempfile.TemporaryDirectory(prefix='gmat-stm-alignment-replay-') as temporary:
        fixture = Path(temporary)/'cases.json'
        with tarfile.open(stage/'sources-before-v1.tar.gz') as bundle:
            fixture.write_bytes(bundle.extractfile(fixture_name).read())
        need(sha(fixture) == plan['fixture_sha256'], 'Frozen native fixture identity differs')
        spec = load(fixture)
        table = stage/'icrf-table-v1.txt'
        need(sha(table) == spec['alignment_table_sha256'], 'Executed alignment table bytes differ')
        table_lines = table.read_text().splitlines()
        for node in spec['literal_nodes']:
            need(node['literal'] in table_lines and [node['epoch']] + node['vector'] ==
                 [float(v) for v in node['literal'].split(',')], 'Literal alignment table-node oracle changed')
        prior = Path(spec['source_matrix_archive'])
        need(sha(prior) == spec['source_matrix_archive_sha256'], 'Source-versioned inner matrix changed')
        for c in spec['cases']:
            key = 'unattended-backward' if c['key'] == 'NativeICRFBackward' else 'unattended-forward'
            need(c['native_stm'] == load(prior)[key]['native_stm'], 'Old native matrix was altered or relabelled')
            for endpoint in ['initial', 'final']:
                node = next(v for v in spec['literal_nodes'] if v['epoch'] == c[endpoint + '_mjd'])
                need(c[endpoint + '_rotation_vector'] == node['vector'], 'Wrong frozen endpoint alignment')
        results = []
        for v, passed, failed in [('before-v1', 27, 48), ('after-v1', 75, 0)]:
            root = stage/v
            result = matrix_audit(root, fixture)
            need(result == load(root/'independent-audit.json') and not result['issues'] and
                 result['passed'] == passed and result['failed'] == failed and result['reads'] == 75,
                 'Native matrix replay changed original outcome')
            run, images = load(root/'run.json'), load(root/'loaded-images.json')
            build = load(stage/('build-'+v+'.json'))
            need(run['native_terminal'] and run['returncode'] == (1 if failed else 0) and
                 run['fixture_sha256'] == sha(fixture) and run['runtime_binary_copies'] == 0, 'Wrong native process or fixture')
            for n, h in build['binary_sha256'].items():
                need(images[str(Path(n).resolve())] == h, 'Wrong executed native image')
            results.append(result)
    failed_gui = load(stage/'gui-headless-v1-receipt.json')
    need(failed_gui['returncode'] == 1 and failed_gui['inputs_unchanged'] and
         'Supply STM GUI startup on native widgets' in (stage/'gui-headless-v1.log').read_text(),
         'Original offscreen-launcher rejection lost')
    with tarfile.open(stage/'gui-launcher-owned-sources-v1.tar.gz') as bundle:
        for n, h in failed_gui['source_sha256'].items():
            need(hashlib.sha256(bundle.extractfile(n).read()).hexdigest() == h, 'Rejected launcher source changed')
    gui = []
    final_build = load(stage/'build-after-v1.json')
    for folder, mode in [('gui-headless-v2', 'headless'), ('gui-visible-v1', 'visible')]:
        root = stage/folder
        guard = load(stage/(folder+'-receipt.json'))
        need(guard['returncode'] == 0 and guard['inputs_unchanged'] and not guard['full_regression'], 'GUI source/image guard failed')
        with tarfile.open(stage/'gui-executed-owned-sources-v1.tar.gz') as bundle:
            need(set(bundle.getnames()) == set(guard['source_sha256']), 'GUI executed source inventory differs')
            for n, h in guard['source_sha256'].items():
                need(hashlib.sha256(bundle.extractfile(n).read()).hexdigest() == h, 'GUI executed source bytes differ')
        for n, h in final_build['binary_sha256'].items():
            if n.endswith('.so'):
                need(guard['image_sha256'][str(Path(n).resolve())] == h, 'Passing native reads rebound to different GUI kernel')
        gui.append(gui_audit(root, mode))
    protected = load(stage/'protected-input-check-v1.json')
    ids = load(protected['manifest'])
    need(protected['status'] == 'passed' and protected['original_inputs_unchanged'] == len(ids) == 1334 and
         not protected['changed'] and sha(protected['manifest']) == protected['manifest_sha256'], 'Protected input guard failed')
    previous = stage.parent/'complete18-20261111-stm-frame-reference-v1/stage-receipt-v1.json'
    need(sha(previous) == 'edbdd99c47b253ac6fb843dc9edfc44287b00786367b83975801f7b37d76caaf', 'Previous engineering receipt changed')
    correction = load(stage/'previous-platform-correction-v1.json')
    need(correction['previous_engineering_receipt_sha256'] == sha(previous) and
         correction['native_X11_or_visible_credit_from_those_old_runs'] == 0 and
         not correction['original_record_changed'] and not correction['coverage_changed'], 'Prior platform correction lost')
    with tarfile.open(previous.parent/'sources-after-v5.tar.gz') as bundle:
        runtime = bundle.extractfile('src/qtgui/tests/AcceptanceRuntime.py').read()
        registration = bundle.extractfile('src/qtgui/CMakeLists.txt').read()
        need(hashlib.sha256(runtime).hexdigest() == correction['original_frozen_runtime_sha256'] and
             hashlib.sha256(registration).hexdigest() == correction['original_frozen_registration_source_sha256'] and
             b"env['QT_QPA_PLATFORM'] = 'offscreen'" in runtime and
             b'add_qt_test(QtGui.NasaStmParameter${stm_parameter_direction}' in registration,
             'Original offscreen route not established by archived inputs')
    for n, h in correction['original_run_logs'].items():
        need(sha(n) == h and '/RunTest.py' in Path(n).read_text(), 'Original GUI launcher log changed')
    return dict(status='passed', native_reads=75, native_frame_contexts=15, matrix_entries=1080,
                original_native_failures_retained=48, native_max_entry_error=max(v['max_error'] for v in results[1]['checks']),
                gui_profiles=gui, gui_matrix_entries=sum(v['matrix_entries'] for v in gui),
                original_gui_launcher_failure_retained=True, original_gui_compile_failure_retained=True,
                previous_platform_narrative_corrected=True, original_inputs_unchanged=1334,
                native_tests_run_by_replay=0, new_requirement_credit=0, requirements_verified=1106,
                requirements_total=1158, agreed_complete=5, agreed_total=18,
                whole_row_qualified=False, full_current_build_qualified=False, full_regression=False)
