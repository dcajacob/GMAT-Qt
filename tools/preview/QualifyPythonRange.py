"""Qualify every bundled Python minor version against a freshly extracted archive."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

import BuildPreviewPackage as builder
from BuildPythonRange import VERSIONS, unpack
from QualifyPythonPackage import clean_environment, write_no_python_check, MCP_SHA256


def run(command, log, environment):
    with log.open('w', encoding='utf-8') as stream:
        result = subprocess.run(list(map(str, command)), cwd=log.parent,
            env=environment, stdout=stream, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError(f'{log.name} failed with exit {result.returncode}')


def qualify(target, archive, output, base_receipt):
    output.mkdir(parents=True, exist_ok=True)
    workspace = Path(tempfile.mkdtemp(prefix='gmat-python-range-'))
    payload = unpack(archive, workspace/'extracted')
    root = payload/'GMAT-Qt.app' if target.startswith('macos') else payload
    manifest = json.loads((payload/'PACKAGE-MANIFEST.json').read_text())
    for name, digest in manifest.items():
        assert hashlib.sha256((payload/name).read_bytes()).hexdigest() == digest, name
    binary = root/'Contents/MacOS' if target.startswith('macos') else root/'bin'
    resource = root/'Contents/Resources' if target.startswith('macos') else root
    metadata = json.loads((resource/'PYTHON-API.json').read_text())
    assert metadata['python_versions'] == list(VERSIONS)
    prior = json.loads(base_receipt.read_text())
    assert prior['status'] == 'passed' and prior['platform'] == target
    assert prior['archive_sha256'] == metadata['base_archive']['sha256']
    environment = clean_environment()
    mcp_bundle = builder.ROOT/'gmat-mcp.zip'
    assert builder.download('https://github.com/dcajacob/GMAT-mcp/releases/download/v0.4.0rc1/gmat-mcp-0.4.0rc1-bundle.zip', mcp_bundle) == MCP_SHA256
    import zipfile
    with zipfile.ZipFile(mcp_bundle) as bundle:
        bundle.extractall(workspace/'mcp')
    installer, = (workspace/'mcp').rglob('install.py')
    driver = workspace/'McpArchiveCheck.py'
    shutil.copy2(Path(__file__).with_name('McpArchiveCheck.py'), driver)
    direct = workspace/'direct-api.py'
    direct.write_text('''import json, os, sys
from pathlib import Path
root = Path(sys.argv[1])
binary = root / ('Contents/MacOS' if root.suffix == '.app' else 'bin')
sys.path.insert(0, str(binary))
handles = []
if sys.platform == 'win32':
    handles = [os.add_dll_directory(str(p)) for p in [binary, root/'plugins']]
import gmatpy as g
os.chdir(binary)
g.Setup('gmat_startup_file.txt')
assert g.Construct('Spacecraft', 'ApiSat').GetName() == 'ApiSat'
assert g.Construct('GroundStation', 'ApiStation').GetName() == 'ApiStation'
assert g.Construct('BatchEstimator', 'ApiEstimator').GetName() == 'ApiEstimator'
# Exercise native-to-Python exception translation as well as all three modules.
try:
    g.Construct('Spacecraft', 'ApiInvalid').SetField('NoSuchField', 'Invalid')
except Exception:
    pass
else:
    raise AssertionError('Unknown class must be rejected')
print(json.dumps({'status':'passed', 'python':sys.version, 'implementation':sys.implementation.name}))
''')
    results = {}
    for version in VERSIONS:
        interpreter = Path(os.environ['GMAT_PYTHON_'+version.replace('.', '')])
        folder = output/version
        folder.mkdir()
        run([interpreter, '-I', direct, root], folder/'direct-api.log', environment)
        server = workspace/('mcp-environment-'+version)
        run([interpreter, installer, '--destination', server], folder/'mcp-install.log', environment)
        server_python = server/('Scripts/python.exe' if os.name == 'nt' else 'bin/python')
        # The released server can use each selected CPython worker through the
        # same generated profile used for live desktop attachment.
        run([server_python, '-I', driver, '--root', root, '--worker', interpreter,
             '--output', folder/'mcp', '--workspace', workspace/('mcp-'+version)],
             folder/'mcp.log', environment)
        result = json.loads((folder/'mcp/mcp-verification.json').read_text())
        assert result['status'] == 'passed'
        results[version] = {'status':'passed', 'mcp':result}
    write_no_python_check(target, root, workspace, output)
    result = {'status':'awaiting-without-python-check', 'platform':target,
        'archive':str(archive), 'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),
        'source_commit':metadata['bindings_source_commit'], 'native_source_commit':metadata['source_commit'],
        'extracted_root':str(root), 'python_versions':results, 'python':metadata,
        'manifest_files_verified':len(manifest), 'base_evidence':prior,
        'base_evidence_sha256':hashlib.sha256(base_receipt.read_bytes()).hexdigest(),
        'full_regression_run':False}
    (output/'verification.json').write_text(json.dumps(result,indent=2)+'\n')


def finish(output):
    receipt = json.loads((output/'verification.json').read_text())
    receipt['gui_without_python'] = json.loads((output/'without-python.json').read_text())
    assert receipt['gui_without_python']['status'] == 'passed'
    root = Path(receipt['extracted_root'])
    payload = root.parent if receipt['platform'].startswith('macos') else root
    for name, digest in json.loads((payload/'PACKAGE-MANIFEST.json').read_text()).items():
        assert hashlib.sha256((payload/name).read_bytes()).hexdigest() == digest, name
    receipt.update(status='passed', archive_files_unchanged_after_checks=True)
    (output/'verification.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({'status':'passed','platform':receipt['platform'],'python_versions':list(receipt['python_versions'])}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform')
    parser.add_argument('--archive', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--base-receipt', type=Path)
    parser.add_argument('--finish', action='store_true')
    args = parser.parse_args()
    if args.finish:
        finish(args.output.resolve())
    else:
        qualify(args.platform, args.archive.resolve(), args.output.resolve(), args.base_receipt.resolve())
