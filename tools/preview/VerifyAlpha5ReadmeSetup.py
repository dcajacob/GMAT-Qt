"""Run only missing README witnesses on published alpha.5 packages."""
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import textwrap
import re
import zipfile

from BuildPythonRange import unpack
from VerifyReadmeSetup import clean_environment, download, run, sha256, MCP_BUNDLE, MCP_SHA256

REPOSITORY = Path(__file__).resolve().parents[2]
VERSION = '0.1.0-alpha.5'
VERSIONS = ('3.12', '3.13', '3.14', '3.15')
RELEASE = f'https://github.com/dcajacob/GMAT-Qt/releases/download/v{VERSION}/'


def readme_example():
    text = (REPOSITORY/'README.rst').read_text()
    block = re.search(r'\.\. code-block:: python\n\n((?:   .*\n|\n)+)', text)
    assert block, 'README Python example missing'
    code = textwrap.dedent(block.group(1))
    ast.parse(code)
    return code


def check(args):
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    workspace = Path(tempfile.mkdtemp(prefix='gmat-alpha5-readme-'))
    release = json.loads((REPOSITORY/'tools/preview/releases'/VERSION/'release-receipt.json').read_text())
    package = next(p for p in release['packages'] if p['platform'] == args.platform)
    saved_path = REPOSITORY/'tools/preview/releases'/VERSION/package['receipt']
    assert sha256(saved_path) == package['receipt_sha256']
    saved = json.loads(saved_path.read_text())
    assert saved['status'] == 'passed' and saved['archive_sha256'] == package['sha256']
    assert saved['gui_without_python']['status'] == 'passed'
    for version in VERSIONS:
        prior = saved['python_versions'][version]
        assert prior['status'] == 'passed' and prior['mcp']['one_profile']
        assert all(p['ok'] for p in prior['mcp']['doctor']['profiles'])
        assert prior['mcp']['worker']['status'] == prior['mcp']['desktop']['status'] == 'succeeded'
    archive = workspace/package['archive']
    if args.archive:
        shutil.copy2(args.archive, archive)
    else:
        download(RELEASE+archive.name, archive)
    assert sha256(archive) == package['sha256'] and archive.stat().st_size == package['bytes']
    payload = unpack(archive, workspace/'extracted')
    root = payload/'GMAT-Qt.app' if args.platform.startswith('macos') else payload
    manifest = json.loads((payload/'PACKAGE-MANIFEST.json').read_text())
    for name, expected in manifest.items():
        assert sha256(payload/name) == expected, name
    code = readme_example()
    script = workspace/'check_gmat.py'
    script.write_text(code)
    (output/'tested-readme.rst').write_bytes((REPOSITORY/'README.rst').read_bytes())
    (output/'check_gmat.py').write_text(code)
    environment = clean_environment()
    if sys.platform.startswith('linux'):
        environment.update(XDG_DATA_HOME=str(workspace/'user-data'), XDG_CONFIG_HOME=str(workspace/'user-config'))
    selected = {}
    for version in VERSIONS:
        folder = output/version
        folder.mkdir()
        python_command = ['py','-'+version] if os.name == 'nt' else ['python'+version]
        interpreter = run([*python_command,'-c','import sys; print(sys.executable)'], workspace,
                          environment, folder/'python-selection.log').strip()
        selected[version] = interpreter
        text = run([*python_command,script,root], workspace, environment, folder/'python-example.log')
        assert 'Sat' in text.splitlines()
        print(args.platform+' Python '+version+' README example passed',flush=True)
    if args.mcp_environment:
        server = args.mcp_environment.resolve()
    else:
        bundle = workspace/'gmat-mcp.zip'
        download(MCP_BUNDLE,bundle)
        assert sha256(bundle) == MCP_SHA256
        with zipfile.ZipFile(bundle) as source:
            source.extractall(workspace/'mcp-bundle')
        installer, = (workspace/'mcp-bundle').rglob('install.py')
        server = workspace/'mcp-env'
        # Installation for each minor version is already qualified. This one
        # environment supplies the CLI needed to test unmodified profiles.
        run([selected['3.15'],installer,'--destination',server],workspace,environment,
            output/'mcp-install.log',timeout=900)
    server_python = server/('Scripts/python.exe' if os.name == 'nt' else 'bin/python')
    cli = server/('Scripts/gmat-mcp.exe' if os.name == 'nt' else 'bin/gmat-mcp')
    driver = workspace/'VerifyReadmeSetup.py'
    shutil.copy2(Path(__file__).with_name('VerifyReadmeSetup.py'),driver)
    cases = []
    for version in VERSIONS:
        folder=output/version
        config=folder/'profiles.toml'
        run([cli,'init','--root',root,'--python',selected[version],'--output',config],
            workspace,environment,folder/'profile-generation.log')
        profile_hash=sha256(config)
        # Doctor, worker missions, live editing and numerical output already
        # passed for this exact archive/version. Only default discovery and
        # attachment with the untouched generated configuration are missing.
        run([server_python,'-I',driver,'--mcp-check','--reuse-worker-qualification',
             '--root',root,'--config',config,'--output',folder],workspace,environment,
             folder/'mcp-setup.log')
        result=json.loads((folder/'mcp-setup.json').read_text())
        assert result['status']=='passed' and result['profile_unchanged_after_checks']
        assert result['uses_generated_profile_without_edits']
        assert sha256(config)==profile_hash
        cases.append({'version':version,'status':'passed','python_executable':selected[version],
            'direct_python_example':'passed','default_profile_attachment':result,
            'reused_checks':['MCP install','doctor','worker propagation','desktop mission editing and results'],
            'reused_receipt_sha256':package['receipt_sha256']})
        print(args.platform+' Python '+version+' default-profile attachment passed',flush=True)
    for name, expected in manifest.items():
        assert sha256(payload/name)==expected,name
    result={'status':'passed','platform':args.platform,'release':VERSION,
        'archive_sha256':package['sha256'],'published_archive_url':RELEASE+archive.name,
        'readme_sha256':sha256(REPOSITORY/'README.rst'),
        'executed_python_example_sha256':hashlib.sha256(code.encode()).hexdigest(),
        'source_commit':subprocess.check_output(['git','rev-parse',args.source_ref],text=True).strip(),
        'saved_qualification_sha256':package['receipt_sha256'],
        'archive_contents_unchanged':True,'python_versions':cases,
        'new_checks':['Exact README Python example','Unmodified generated MCP profile and default desktop discovery'],
        'reused_checks':['Published download integrity','CPython-version MCP installation','doctor',
            'isolated propagation','live mission edit/run/results','Python-free GUI startup',
            'package mission/plots/reports/save-reopen','independent event and Hermite numerics'],
        'native_missions_rerun':False,'full_regression_run':False}
    (output/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'status':'passed','platform':args.platform,'python_versions':list(VERSIONS)}))


def review_saved(receipts):
    receipts=receipts.resolve()
    for name,digest in json.loads((receipts/'SHA256.json').read_text()).items():
        path=(receipts/name).resolve()
        assert path.is_relative_to(receipts)
        assert sha256(path)==digest,name
    summary=json.loads((receipts/'summary.json').read_text())
    assert summary['status']=='passed' and not summary['remaining_setup_failures']
    assert summary['platform_python_combinations']==16
    assert sha256(REPOSITORY/'README.rst')==summary['readme_sha256']
    example=hashlib.sha256(readme_example().encode()).hexdigest()
    assert example==summary['executed_python_example_sha256']
    packages=json.loads((receipts.parent/'release-receipt.json').read_text())['packages']
    packages={p['platform']:p for p in packages}
    published=json.loads((receipts/'published-assets.json').read_text())
    assets={a['name']:a for a in published['assets']}
    for record in summary['platforms']:
        package=packages[record['platform']]
        saved_path=receipts.parent/package['receipt']
        assert sha256(saved_path)==package['receipt_sha256']
        saved=json.loads(saved_path.read_text())
        path=receipts/record['receipt']
        assert sha256(path)==record['receipt_sha256']
        result=json.loads(path.read_text())
        assert result['status']=='passed' and result['archive_contents_unchanged']
        assert result['archive_sha256']==package['sha256']
        assert result['readme_sha256'] in {summary['readme_sha256'],summary['readme_crlf_sha256']}
        assert result['executed_python_example_sha256']==example
        assert result['saved_qualification_sha256']==package['receipt_sha256']
        asset=assets[package['archive']]
        assert asset['digest']=='sha256:'+package['sha256'] and asset['size']==package['bytes']
        assert [v['version'] for v in result['python_versions']]==list(VERSIONS)
        for case in result['python_versions']:
            folder=path.parent/case['version']
            assert case['status']=='passed' and case['direct_python_example']=='passed'
            assert 'Sat' in (folder/'python-example.log').read_text().splitlines()
            mcp=json.loads((folder/'mcp-setup.json').read_text())
            assert mcp==case['default_profile_attachment']
            assert mcp['status']=='passed' and mcp['uses_generated_profile_without_edits']
            assert mcp['profile_unchanged_after_checks'] and sha256(folder/'profiles.toml')==mcp['profile_sha256']
            assert mcp['default_gui_startup_and_discovery']==mcp['same_profile_desktop_attachment']=='passed'
            previous=saved['python_versions'][case['version']]['mcp']
            assert all(p['ok'] for p in previous['doctor']['profiles'])
            assert previous['worker']['status']==previous['desktop']['status']=='succeeded'
        assert saved['gui_without_python']['status']=='passed'
        assert not result['native_missions_rerun'] and not result['full_regression_run']
    print(json.dumps({'status':'passed','platforms':4,'python_versions':list(VERSIONS),
                      'combinations':16,'native_execution':False}))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform',choices=['linux-x86_64','windows-x64','macos-arm64','macos-x64'])
    parser.add_argument('--output',type=Path)
    parser.add_argument('--review-saved',type=Path)
    parser.add_argument('--archive',type=Path)
    parser.add_argument('--mcp-environment',type=Path)
    parser.add_argument('--source-ref',default='HEAD')
    args=parser.parse_args()
    if args.review_saved:
        review_saved(args.review_saved)
    elif not args.platform or not args.output:
        parser.error('--platform and --output are required for missing setup checks')
    else:
        check(args)
