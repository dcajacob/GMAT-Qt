"""Qualify the extracted alpha archive with CPython and the released GMAT MCP client."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile

import BuildPreviewPackage as builder

MCP_VERSION = '0.4.0rc1'
MCP_SHA256 = 'cec088ae1489e753cc9cdde95f9d0310caf9d5922a817900b72f38683ff91fc4'


def clean_environment():
    environment = dict(os.environ)
    for key in list(environment):
        if key.startswith(('DYLD_', 'GMAT_')) or key in {
            'PYTHONPATH', 'PYTHONHOME', 'LD_LIBRARY_PATH', 'QT_PLUGIN_PATH',
            'QT_QPA_PLATFORM_PLUGIN_PATH', 'OSG_LIBRARY_PATH'}:
            environment.pop(key, None)
    environment['PYTHONNOUSERSITE'] = '1'
    environment['PYTHONDONTWRITEBYTECODE'] = '1'
    if os.name == 'nt':
        system = Path(os.environ.get('SystemRoot', 'C:/Windows'))
        environment['PATH'] = os.pathsep.join(map(str, [system/'System32', system]))
    else:
        environment['PATH'] = '/usr/bin:/bin'
        if sys.platform.startswith('linux'):
            # Hosted CPython may need its own shared runtime directory.
            # GMAT paths are supplied only from the extracted installation.
            environment['LD_LIBRARY_PATH'] = str(Path(sys.base_prefix)/'lib')
    return environment


def write_no_python_check(target, root, workspace, logs):
    binary = root/'Contents/MacOS' if target.startswith('macos') else root/'bin'
    launcher = root/'run-gmat' if target.startswith('linux') else binary/(
        'GmatQt.exe' if target.startswith('windows') else 'GmatQt')
    startup = root.parent/'gmat_startup_qt.txt' if target.startswith('macos') else binary/'gmat_startup_qt.txt'
    python_root = Path(sys.base_prefix)
    hidden = python_root.with_name(python_root.name + '-gmat-hidden-' + str(os.getpid()))
    screenshot = logs/'without-python.png'
    report = logs/'without-python.json'
    environment = ['--unset=PYTHONHOME', '--unset=PYTHONPATH', '--unset=LD_LIBRARY_PATH',
                   '--unset=DYLD_LIBRARY_PATH', '--unset=DYLD_FALLBACK_LIBRARY_PATH',
                   '--unset=QT_PLUGIN_PATH', '--unset=QT_QPA_PLATFORM_PLUGIN_PATH',
                   '--unset=OSG_LIBRARY_PATH', 'QT_QPA_PLATFORM=offscreen']
    if target.startswith('windows'):
        system = Path(os.environ.get('SystemRoot', 'C:/Windows'))
        environment += ['PATH='+os.pathsep.join(map(str, [binary, system/'System32', system])),
                        'QT_QPA_FONTDIR='+str(system/'Fonts')]
    elif target.startswith('linux'):
        environment += ['PATH=/usr/bin:/bin', 'LD_DEBUG=libs']
    else:
        environment += ['PATH=/usr/bin:/bin', 'DYLD_PRINT_LIBRARIES=1',
                        'QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM=1']
    arguments = [str(launcher), '--startup', str(startup), '--settings-dir',
                 str(workspace/'without-python-settings'), '--mcp-discovery-dir',
                 str(workspace/'without-python-discovery'), '--screenshot', str(screenshot)]
    bracket = lambda value: '[==[' + str(value) + ']==]'
    command = ' '.join(bracket(value) for value in [*environment, *arguments])
    text = f'''cmake_minimum_required(VERSION 3.21)
file(RENAME {bracket(python_root)} {bracket(hidden)} RESULT moved)
if(NOT moved STREQUAL "0")
  message(FATAL_ERROR "Could not temporarily hide the selected CPython installation: ${{moved}}")
endif()
execute_process(COMMAND "${{CMAKE_COMMAND}}" -E env {command}
  WORKING_DIRECTORY {bracket(workspace)} RESULT_VARIABLE result TIMEOUT 60
  OUTPUT_FILE {bracket(logs/'without-python.stdout')}
  ERROR_FILE {bracket(logs/'without-python.stderr')})
file(RENAME {bracket(hidden)} {bracket(python_root)} RESULT restored)
if(NOT restored STREQUAL "0")
  message(FATAL_ERROR "CPython restoration failed: ${{restored}}")
endif()
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "The packaged GUI failed with CPython hidden: ${{result}}")
endif()
if(NOT EXISTS {bracket(screenshot)})
  message(FATAL_ERROR "The GUI did not produce its startup screenshot")
endif()
file(READ {bracket(logs/'without-python.stderr')} loader_log)
if(loader_log MATCHES "[Ll]ibpython[0-9]|Python\\.framework/")
  message(FATAL_ERROR "The GUI loaded or searched for a Python runtime")
endif()
file(GLOB bridge_records {bracket(workspace/'without-python-discovery/*.json')})
if(bridge_records)
  message(FATAL_ERROR "The GUI enabled its MCP bridge without opt-in")
endif()
file(WRITE {bracket(report)} "{{\\"status\\":\\"passed\\",\\"python_installation_hidden\\":true,\\"bridge_opt_in_preserved\\":true}}\\n")
'''
    (logs/'without-python.cmake').write_text(text)
    (logs/'without-python-plan.json').write_text(json.dumps({
        'python_installation': str(python_root), 'hidden_path': str(hidden),
        'gui': str(launcher), 'environment': environment, 'arguments': arguments}, indent=2)+'\n')


def qualify(target, archive):
    original_root, original_logs = builder.ROOT, builder.LOGS
    logs = original_logs/'archive-qualification'
    logs.mkdir(parents=True, exist_ok=True)
    workspace = Path(tempfile.mkdtemp(prefix='gmat-alpha3-'))
    extracted = workspace/'extracted'
    extracted.mkdir()
    if target.startswith('macos'):
        subprocess.run(['ditto', '-x', '-k', str(archive), str(extracted)], check=True)
    elif target.startswith('linux'):
        with tarfile.open(archive) as bundle:
            bundle.extractall(extracted, filter='data')
    else:
        with zipfile.ZipFile(archive) as bundle:
            bundle.extractall(extracted)
    payload = next(p for p in extracted.iterdir() if p.is_dir() and p.name.startswith('GMAT-Qt-'))
    manifest = json.loads((payload/'PACKAGE-MANIFEST.json').read_text())
    for name, digest in manifest.items():
        path = payload/name
        assert path.resolve().is_relative_to(payload.resolve()), name
        assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, name
    root = payload/'GMAT-Qt.app' if target.startswith('macos') else payload
    binary = root/'Contents/MacOS' if target.startswith('macos') else root/'bin'
    api = binary/'gmatpy/_py312'
    for module in ['gmat', 'station', 'navigation']:
        assert (api/(module+'_py.py')).is_file()
        assert any(api.glob('_'+module+'_py.*'))
    metadata_path = root/'Contents/Resources/PYTHON-API.json' if target.startswith('macos') else root/'PYTHON-API.json'
    metadata = json.loads(metadata_path.read_text())
    assert metadata['python_version'] == '3.12' and metadata['architecture'] == target
    assert not any(p.is_file() and (p.name.lower().startswith('libpython3') or
                    p.name.lower() in {'python', 'python3.dll', 'python312.dll'}) for p in payload.rglob('*'))
    runtime = original_logs/'linux-runtime.json'
    if runtime.exists():
        shutil.copy2(runtime, logs/'linux-runtime.json')
    environment = clean_environment()
    for key in ['LD_LIBRARY_PATH', 'DYLD_LIBRARY_PATH', 'PYTHONPATH']:
        for entry in environment.get(key, '').split(os.pathsep):
            if entry:
                assert not Path(entry).resolve().is_relative_to(builder.REPO.resolve()), (key, entry)
    bundle = workspace/'gmat-mcp.zip'
    digest = builder.download(
        f'https://github.com/dcajacob/GMAT-mcp/releases/download/v{MCP_VERSION}/gmat-mcp-{MCP_VERSION}-bundle.zip', bundle)
    assert digest == MCP_SHA256
    with zipfile.ZipFile(bundle) as source:
        source.extractall(workspace/'mcp-bundle')
    mcp_bundle = next((workspace/'mcp-bundle').iterdir())
    venv = workspace/'mcp-environment'
    with (logs/'mcp-install.log').open('w') as log:
        subprocess.run([sys.executable, str(mcp_bundle/'install.py'), '--destination', str(venv)],
                       env=environment, cwd=workspace, stdout=log, stderr=subprocess.STDOUT, check=True)
    interpreter = venv/('Scripts/python.exe' if os.name == 'nt' else 'bin/python')
    driver = workspace/'McpArchiveCheck.py'
    shutil.copy2(Path(__file__).with_name('McpArchiveCheck.py'), driver)
    hidden = []
    try:
        for source in [builder.REPO/'application/bin', builder.REPO/'application/plugins', original_root/'gmat-build']:
            if source.exists():
                destination = source.with_name(source.name+'-qualification-hidden')
                source.rename(destination)
                hidden.append((source, destination))
        builder.ROOT, builder.LOGS = workspace/'package-probe', logs
        builder.ROOT.mkdir()
        executable = payload/'run-gmat' if target.startswith('linux') else binary/(
            'GmatQt.exe' if target.startswith('windows') else 'GmatQt')
        # This invokes the existing package mission and save/reopen assertions
        # against the extracted archive while the build outputs are unavailable.
        package_result = builder.probe(target, payload, executable, model_reader=False)
        with (logs/'mcp-protocol.log').open('w') as log:
            subprocess.run([str(interpreter), '-I', str(driver), '--root', str(root),
                '--worker', sys.executable, '--output', str(logs/'mcp'),
                '--workspace', str(workspace/'mcp-work')], env=environment,
                cwd=workspace, stdout=log, stderr=subprocess.STDOUT, check=True)
    finally:
        builder.ROOT, builder.LOGS = original_root, original_logs
        for source, destination in reversed(hidden):
            destination.rename(source)
    write_no_python_check(target, root, workspace, logs)
    receipt = {'status': 'awaiting-without-python-check', 'platform': target,
               'source_commit': metadata['source_commit'], 'archive': str(archive),
               'archive_sha256': hashlib.sha256(archive.read_bytes()).hexdigest(),
               'extracted_root': str(root), 'manifest_files_verified': len(manifest),
               'python': metadata, 'mcp_release': MCP_VERSION, 'mcp_bundle_sha256': MCP_SHA256,
               'package_checks': package_result,
               'mcp': json.loads((logs/'mcp/mcp-verification.json').read_text()),
               'qualification_library_environment': {key: environment.get(key, '')
                   for key in ['LD_LIBRARY_PATH', 'DYLD_LIBRARY_PATH', 'PYTHONPATH']},
               'original_build_outputs_hidden_during_checks': True,
               'full_regression_run': False}
    (logs/'verification.json').write_text(json.dumps(receipt, indent=2)+'\n')


def finish(target):
    logs = builder.LOGS/'archive-qualification'
    receipt = json.loads((logs/'verification.json').read_text())
    no_python = json.loads((logs/'without-python.json').read_text())
    assert no_python['status'] == 'passed'
    receipt['gui_without_python'] = no_python
    receipt['status'] = 'passed'
    (logs/'verification.json').write_text(json.dumps(receipt, indent=2)+'\n')
    builder.copy_file(logs/'verification.json', builder.ROOT/'dist'/(target+'-api-verification.json'))
    print(json.dumps({'platform': target, 'status': 'passed', 'python': '3.12', 'mcp_profile_count': 1}))
