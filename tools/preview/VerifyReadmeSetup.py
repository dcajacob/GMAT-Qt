"""Check missing README setup paths against the published alpha.4 archives."""
import argparse
import ast
import asyncio
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import textwrap
import urllib.request
import zipfile


REPOSITORY = Path(__file__).resolve().parents[2]
RELEASE = 'https://github.com/dcajacob/GMAT-Qt/releases/download/v0.1.0-alpha.4/'
MCP_BUNDLE = 'https://github.com/dcajacob/GMAT-mcp/releases/download/v0.4.0rc1/gmat-mcp-0.4.0rc1-bundle.zip'
MCP_SHA256 = 'cec088ae1489e753cc9cdde95f9d0310caf9d5922a817900b72f38683ff91fc4'


def sha256(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def download(url, path):
    request = urllib.request.Request(url, headers={'User-Agent': 'GMAT-Qt-README-check'})
    with urllib.request.urlopen(request, timeout=180) as response, path.open('wb') as stream:
        shutil.copyfileobj(response, stream)


def run(command, folder, environment, log, timeout=180):
    with log.open('w', encoding='utf-8') as stream:
        process = subprocess.Popen(list(map(str, command)), cwd=folder, env=environment,
                                   stdout=stream, stderr=subprocess.STDOUT)
        try:
            code = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
            raise
    text = log.read_text(encoding='utf-8', errors='replace')
    if code:
        raise RuntimeError(f'{log.name} failed with {code}: {text[-3000:]}')
    return text


def clean_environment():
    environment = dict(os.environ)
    for key in list(environment):
        if key.startswith(('GMAT_', 'DYLD_')) or key in {
                'LD_LIBRARY_PATH', 'PYTHONPATH', 'PYTHONHOME',
                'QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH', 'OSG_LIBRARY_PATH'}:
            environment.pop(key, None)
    environment.update(PYTHONNOUSERSITE='1', PYTHONDONTWRITEBYTECODE='1')
    return environment


def review_saved(receipts):
    inventory = json.loads((receipts / 'SHA256.json').read_text())
    for name, expected in inventory.items():
        path = (receipts / name).resolve()
        assert path.is_relative_to(receipts.resolve()), name
        assert sha256(path) == expected, name
    summary = json.loads((receipts / 'summary.json').read_text())
    provenance = json.loads((receipts / 'provenance.json').read_text())
    qualification = json.loads((receipts.parent / 'release-receipt.json').read_text())
    assert summary['status'] == 'passed' and not summary['remaining_setup_failures']
    assert summary['release_source'] == qualification['source_commit']
    tested_readme = (receipts / 'tested-readme.rst').read_text()
    pattern = r'\.\. code-block:: python\n\n((?:   .*\n|\n)+)'
    tested_example = textwrap.dedent(re.search(pattern, tested_readme).group(1))
    current_example = textwrap.dedent(re.search(
        pattern, (REPOSITORY / 'README.rst').read_text()).group(1))
    assert tested_example == current_example
    assert hashlib.sha256(tested_example.encode()).hexdigest() == summary['executed_python_example_sha256']
    expected_readme_hashes = {provenance['tested_readme_sha256'],
                             provenance['tested_readme_crlf_sha256']}
    archives = {item['platform']: item for item in qualification['platforms']}
    assert {item['platform'] for item in summary['platforms']} == set(archives)
    for platform_result in summary['platforms']:
        name = platform_result['platform']
        final_path = receipts / platform_result['setup_receipt']
        direct_path = receipts / platform_result['direct_python_receipt']
        assert sha256(final_path) == platform_result['setup_receipt_sha256']
        assert sha256(direct_path) == platform_result['direct_python_receipt_sha256']
        final = json.loads(final_path.read_text())
        direct = json.loads(direct_path.read_text())
        assert final['status'] == 'passed' and final['platform'] == name
        assert final['archive_sha256'] == archives[name]['sha256']
        assert final['archive_contents_unchanged']
        assert final['readme_sha256'] in expected_readme_hashes
        assert direct['readme_sha256'] in expected_readme_hashes
        assert final['reused_setup_receipt_sha256'] == sha256(direct_path)
        assert direct['direct_python_example'] == 'passed'
        assert 'Sat' in (direct_path.parent / 'python-example.log').read_text().splitlines()
        native_path = final_path.parent / 'reused-api-verification.json'
        assert sha256(native_path) == archives[name]['receipt_sha256']
        native = json.loads(native_path.read_text())
        assert native['status'] == 'passed' and native['source_commit'] == summary['release_source']
        assert native['gui_without_python']['status'] == 'passed'
        mcp = final['mcp_setup']
        assert mcp['status'] == 'passed' and mcp['profile_unchanged_after_checks']
        assert mcp['uses_generated_profile_without_edits']
        assert mcp['default_gui_startup_and_discovery'] == 'passed'
        assert mcp['same_profile_desktop_attachment'] == 'passed'
        assert mcp['worker_core_station_navigation'] in ('passed', 'reused-passed')
        doctor_path = (direct_path.parent if final['doctor'] == 'reused-passed'
                       else final_path.parent) / 'doctor.json'
        doctor = json.loads(doctor_path.read_text())
        assert len(doctor['profiles']) == 1 and doctor['profiles'][0]['ok']
    print(json.dumps({'status': 'passed', 'platforms': sorted(archives),
                      'receipt_files_verified': len(inventory), 'native_tests_rerun': False}))


async def check_mcp(args):
    from mcp import Client, StdioServerParameters
    from gmat_mcp.config import load_config

    async def call(client, tool_name, **arguments):
        result = await client.call_tool(tool_name, arguments)
        if result.is_error or result.structured_content is None:
            raise RuntimeError(f'{tool_name} failed: {result}')
        return result.structured_content

    config_path = args.config.resolve()
    config_hash = sha256(config_path)
    config = load_config(config_path)
    assert list(config.profiles) == ['gmat-qt']
    cli = Path(sys.executable).parent / ('gmat-mcp.exe' if os.name == 'nt' else 'gmat-mcp')
    parameters = StdioServerParameters(command=str(cli),
        args=['--config', str(config_path), 'serve'], env=dict(os.environ))
    result = {'status': 'running', 'profile_sha256': config_hash,
              'uses_generated_profile_without_edits': True,
              'desktop_discovery_directory': str(config.server.desktop_discovery_dir)}
    process = None
    log = None
    try:
        async with Client(parameters, cache=None) as client:
            profiles = await call(client, 'list_profiles')
            assert [profile['id'] for profile in profiles['profiles']] == ['gmat-qt']
            prior_mcp = args.reuse_setup / 'mcp-setup.json' if args.reuse_setup else None
            if prior_mcp is not None and prior_mcp.is_file():
                prior_mcp = json.loads(prior_mcp.read_text())
                assert prior_mcp['worker_core_station_navigation'] in ('passed', 'reused-passed')
                result['worker_core_station_navigation'] = 'reused-passed'
            else:
                session = await call(client, 'create_session', profile_id='gmat-qt')
                assert session['backend'] == 'worker'
                assert Path(session['engine']['api']).resolve().is_relative_to(args.root.resolve())
                for kind, name in [('Spacecraft', 'ReadmeSat'), ('GroundStation', 'ReadmeStation'),
                                   ('BatchEstimator', 'ReadmeEstimator')]:
                    created = await call(client, 'create_object', session_id=session['session_id'],
                                         type=kind, name=name)
                    assert created['name'] == name
                await call(client, 'close_session', session_id=session['session_id'])
                result['worker_core_station_navigation'] = 'passed'
            before_launch = await call(client, 'list_desktops')
            existing_desktops = {desktop['desktop_id'] for desktop in before_launch['desktops']}

            root = args.root.resolve()
            executable = root / 'run-gmat'
            if not executable.is_file():
                executable = root / ('Contents/MacOS/GmatQt' if root.suffix == '.app'
                                      else 'bin/GmatQt.exe')
            # Only rendering and settings isolation supplement the README's launch.
            # Startup and desktop discovery use the application's default paths.
            environment = dict(os.environ, QT_QPA_PLATFORM='offscreen',
                QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM='1')
            if os.name == 'nt':
                environment['QT_QPA_FONTDIR'] = str(Path(os.environ['SystemRoot']) / 'Fonts')
            log = (args.output / 'desktop.log').open('w', encoding='utf-8')
            process = subprocess.Popen([str(executable), '--mcp-bridge', '--settings-dir',
                str(args.output / 'gui-settings')], cwd=root.parent, env=environment,
                stdout=log, stderr=subprocess.STDOUT)
            async with asyncio.timeout(45):
                while True:
                    desktops = await call(client, 'list_desktops')
                    matches = [desktop for desktop in desktops['desktops']
                               if desktop['desktop_id'] not in existing_desktops]
                    if matches:
                        break
                    if process.poll() is not None:
                        raise RuntimeError('README GUI command exited before desktop discovery')
                    await asyncio.sleep(.1)
            assert len(matches) == 1
            attached = await call(client, 'attach_desktop', profile_id='gmat-qt',
                                  desktop_id=matches[0]['desktop_id'])
            assert attached['backend'] == 'desktop'
            await call(client, 'list_objects', session_id=attached['session_id'])
            await call(client, 'detach_desktop', session_id=attached['session_id'])
            assert process.poll() is None
            result['default_gui_startup_and_discovery'] = 'passed'
            result['same_profile_desktop_attachment'] = 'passed'
        assert sha256(config_path) == config_hash
        result['profile_unchanged_after_checks'] = True
        result['status'] = 'passed'
    except Exception as error:
        result.update(status='failed', error=str(error))
        raise
    finally:
        if process is not None and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        if log is not None:
            log.close()
        (args.output / 'mcp-setup.json').write_text(json.dumps(result, indent=2) + '\n')


def check_setup(args):
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    workspace = Path(tempfile.mkdtemp(prefix='gmat-readme-install-'))
    report = {'status': 'running', 'platform': args.platform, 'native_os': platform.platform(),
              'workspace': str(workspace), 'readme_sha256': sha256(REPOSITORY / 'README.rst'),
              'native_missions_reused': True, 'full_regression_run': False}
    try:
        published = REPOSITORY / 'tools/preview/releases/0.1.0-alpha.4/release-receipt.json'
        qualification = json.loads(published.read_text())
        spec = next(item for item in qualification['platforms'] if item['platform'] == args.platform)
        archive = workspace / Path(spec['archive']).name
        download(RELEASE + archive.name, archive)
        assert sha256(archive) == spec['sha256']
        assert archive.stat().st_size == spec['bytes']
        report['archive_sha256'] = spec['sha256']
        prior_path = output / 'reused-api-verification.json'
        download(RELEASE + Path(spec['receipt']).name, prior_path)
        assert sha256(prior_path) == spec['receipt_sha256']
        prior = json.loads(prior_path.read_text())
        assert prior['status'] == 'passed' and prior['archive_sha256'] == spec['sha256']
        report['reused_qualification'] = {'sha256': spec['receipt_sha256'],
            'package_checks': prior['package_checks']['status'],
            'mcp_worker': prior['mcp']['worker']['status'],
            'mcp_desktop': prior['mcp']['desktop']['status'],
            'gui_without_python': prior['gui_without_python']['status']}
        extracted = workspace / 'installed'
        extracted.mkdir()
        if args.platform.startswith('macos'):
            subprocess.run(['ditto', '-x', '-k', str(archive), str(extracted)], check=True)
        elif args.platform.startswith('windows'):
            with zipfile.ZipFile(archive) as bundle:
                bundle.extractall(extracted)
        else:
            with tarfile.open(archive) as bundle:
                bundle.extractall(extracted, filter='data')
        payload = next(path for path in extracted.iterdir()
                       if path.is_dir() and path.name.startswith('GMAT-Qt-'))
        manifest = json.loads((payload / 'PACKAGE-MANIFEST.json').read_text())
        for name, digest in manifest.items():
            assert sha256(payload / name) == digest, name
        root = payload / 'GMAT-Qt.app' if args.platform.startswith('macos') else payload
        environment = clean_environment()
        if args.platform.startswith('linux'):
            environment['XDG_DATA_HOME'] = str(workspace / 'user-data')
            environment['XDG_CONFIG_HOME'] = str(workspace / 'user-config')
        python_command = ['py', '-3.12'] if args.platform.startswith('windows') else ['python3.12']
        selected = run([*python_command, '-c', 'import sys; print(sys.executable)'], workspace,
                       environment, output / 'python-selection.log').strip()
        report['python_executable'] = selected
        readme = (REPOSITORY / 'README.rst').read_text()
        code = textwrap.dedent(re.search(
            r'\.\. code-block:: python\n\n((?:   .*\n|\n)+)', readme).group(1))
        ast.parse(code)
        script = workspace / 'check_gmat.py'
        script.write_text(code)
        if args.reuse_setup:
            previous = json.loads((args.reuse_setup / 'result.json').read_text())
            assert previous['archive_sha256'] == spec['sha256']
            assert previous['readme_sha256'] == report['readme_sha256']
            assert previous['direct_python_example'] in ('passed', 'reused-passed')
            report['direct_python_example'] = 'reused-passed'
            report['reused_setup_receipt_sha256'] = sha256(args.reuse_setup / 'result.json')
        else:
            text = run([*python_command, script, root], workspace, environment,
                       output / 'python-example.log')
            assert 'Sat' in text.splitlines()
            report['direct_python_example'] = 'passed'

        bundle = workspace / 'gmat-mcp.zip'
        download(MCP_BUNDLE, bundle)
        assert sha256(bundle) == MCP_SHA256
        bundle_folder = workspace / 'mcp-bundle'
        with zipfile.ZipFile(bundle) as zipped:
            zipped.extractall(bundle_folder)
        installer = next(bundle_folder.iterdir()) / 'install.py'
        venv = workspace / 'mcp-env'
        run([*python_command, installer, '--destination', venv], workspace, environment,
            output / 'mcp-install.log', timeout=900)
        scripts = venv / ('Scripts' if os.name == 'nt' else 'bin')
        cli = scripts / ('gmat-mcp.exe' if os.name == 'nt' else 'gmat-mcp')
        config = workspace / 'profiles.toml'
        run([cli, 'init', '--root', root, '--python', selected, '--output', config],
            workspace, environment, output / 'profile-generation.log')
        if args.reuse_setup and (args.reuse_setup / 'doctor.json').is_file():
            doctor = json.loads((args.reuse_setup / 'doctor.json').read_text())
            report['doctor'] = 'reused-passed'
        else:
            doctor = json.loads(run([cli, '--config', config, 'doctor'], workspace, environment,
                                   output / 'doctor.json'))
            report['doctor'] = 'passed'
        assert len(doctor['profiles']) == 1 and doctor['profiles'][0]['ok']
        assert {'Spacecraft', 'GroundStation', 'BatchEstimator'} <= set(
            doctor['profiles'][0]['engine']['classes'])
        shutil.copy2(config, output / 'profiles.toml')
        driver = workspace / 'VerifyReadmeSetup.py'
        shutil.copy2(__file__, driver)
        interpreter = scripts / ('python.exe' if os.name == 'nt' else 'python')
        command = [interpreter, '-I', driver, '--mcp-check', '--root', root, '--config', config,
                   '--output', output]
        if args.reuse_setup:
            command += ['--reuse-setup', args.reuse_setup.resolve()]
        run(command, workspace, environment, output / 'mcp-setup.log')
        report['mcp_setup'] = json.loads((output / 'mcp-setup.json').read_text())
        assert report['mcp_setup']['status'] == 'passed'
        for name, digest in manifest.items():
            assert sha256(payload / name) == digest, name
        report['archive_contents_unchanged'] = True
        report['status'] = 'passed'
    except Exception as error:
        report.update(status='failed', error=str(error))
        raise
    finally:
        (output / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'platform': args.platform, 'status': report['status']}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--review-saved', type=Path)
    parser.add_argument('--mcp-check', action='store_true')
    parser.add_argument('--root', type=Path)
    parser.add_argument('--config', type=Path)
    parser.add_argument('--reuse-setup', type=Path)
    arguments = parser.parse_args()
    if arguments.review_saved:
        review_saved(arguments.review_saved)
    elif arguments.output is None:
        parser.error('--output is required when running setup checks')
    elif arguments.mcp_check:
        asyncio.run(check_mcp(arguments))
    else:
        check_setup(arguments)
