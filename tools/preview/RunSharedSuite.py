"""Build and run the frozen, targeted CTest selection on a native platform."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

import BuildPreviewPackage as builder


def sha(path):
    return hashlib.sha256(path.read_bytes().replace(b'\r\n', b'\n')).hexdigest()


def executable(build_dir, name, platform):
    filename = name + ('.exe' if platform.startswith('windows') else '')
    paths = list((build_dir / 'src/qtgui').rglob(filename))
    if len(paths) != 1:
        raise RuntimeError(f'Expected one executable for {name}, found {paths}')
    return paths[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', required=True,
                        choices=['linux-x86_64', 'windows-x64', 'macos-arm64', 'macos-x64'])
    parser.add_argument('--cases', default=os.environ.get('SHARED_CASES'),
                        help='Comma-separated subset for a targeted retry')
    parser.add_argument('--existing-build', type=Path)
    parser.add_argument('--source-ref', default='HEAD')
    parser.add_argument('--output', type=Path, default=builder.REPO / 'build/shared-suite')
    args = parser.parse_args()
    plan_path = Path(__file__).with_name('SharedSuite.json')
    plan = json.loads(plan_path.read_text())
    selected_names = set(args.cases.split(',')) if args.cases else {c['name'] for c in plan['cases']}
    cases = [c for c in plan['cases'] if c['name'] in selected_names]
    if len(cases) != len(selected_names):
        parser.error('The requested subset includes a case outside SharedSuite.json')
    targets = sorted({c['target'] for c in cases})
    args.output.mkdir(parents=True, exist_ok=True)
    source_paths = {builder.REPO / c['source'] for c in cases}
    source_paths.update((builder.REPO / 'src/qtgui/tests').glob('*.hpp'))
    source_paths.update(builder.REPO / 'src/qtgui/tests' / name
                        for name in ['RunTest.py', 'AcceptanceRuntime.py'])
    source_hashes = {p.relative_to(builder.REPO).as_posix(): sha(p) for p in sorted(source_paths)}
    source_commit = subprocess.check_output(
        ['git', 'rev-parse', args.source_ref], cwd=builder.REPO, text=True).strip()
    execution_plan = {'source_commit': source_commit, 'platform': args.platform,
                      'graphics_profile': 'offscreen', 'cases': cases,
                      'test_source_sha256': source_hashes, 'suite_sha256': sha(plan_path),
                      'assertions_and_tolerances': 'existing C++ test sources'}
    (args.output / 'execution-plan.json').write_text(json.dumps(execution_plan, indent=2) + '\n')

    environment = dict(os.environ, GMAT_QT_ACCEPTANCE_MODE='offscreen',
                       QT_QPA_PLATFORM='offscreen', LIBGL_ALWAYS_SOFTWARE='1')
    if args.platform.startswith('windows'):
        environment['QT_QPA_FONTDIR'] = str(Path(os.environ.get('SystemRoot', 'C:/Windows')) / 'Fonts')
    if args.existing_build:
        build_dir = args.existing_build.resolve()
        (args.output / 'native-executables.json').write_text(json.dumps({
            name: hashlib.sha256(executable(build_dir, name, args.platform).read_bytes()).hexdigest()
            for name in targets}, indent=2) + '\n')
    else:
        builder.LOGS.mkdir(parents=True, exist_ok=True)
        builder.DEPS.mkdir(parents=True, exist_ok=True)
        restored = builder.restore_native(args.platform)
        if restored and args.platform.startswith('windows'):
            qt, osg, prefix = (Path(restored[name]) for name in ['qt', 'osg', 'prefix'])
        else:
            qt, osg, prefix = builder.dependencies(args.platform)
        build_dir = builder.ROOT / 'gmat-build'
        if not restored:
            builder.build(args.platform, qt, osg, prefix,
                          extra_options=['-DGMAT_QT_BUILD_TESTS=ON'], targets=targets)
            binaries = [executable(build_dir, name, args.platform) for name in targets]
            metadata = list(build_dir.rglob('CTestTestfile.cmake'))
            builder.preserve_native(args.platform, qt, osg, prefix, extra_files=binaries + metadata)
        if args.platform.startswith('windows'):
            environment['PATH'] = os.pathsep.join([
                str(builder.REPO / 'application/bin'), str(qt / 'bin'),
                str(osg / 'bin'), str(prefix / 'bin'), environment.get('PATH', '')])
            environment['QT_PLUGIN_PATH'] = str(qt / 'plugins')
        elif args.platform.startswith('linux'):
            environment['LD_LIBRARY_PATH'] = os.pathsep.join([
                str(builder.REPO / 'application/bin'), str(qt / 'lib'),
                environment.get('LD_LIBRARY_PATH', '')])
            environment['QT_PLUGIN_PATH'] = str(qt / 'plugins')

    pattern = '^(' + '|'.join(re.escape(c['name']) for c in cases) + ')$'
    command = ['ctest', '--test-dir', str(build_dir), '-C', 'Release', '-R', pattern]
    description = subprocess.check_output(command + ['--show-only=json-v1'],
                                          env=environment, text=True)
    registered = json.loads(description)['tests']
    if {t['name'] for t in registered} != selected_names:
        raise RuntimeError('CTest selection differs from the frozen suite')
    (args.output / 'ctest-selection.json').write_text(description)
    junit = args.output / 'junit.xml'
    with (args.output / 'ctest.log').open('w', encoding='utf-8') as log:
        result = subprocess.run(command + ['--output-on-failure', '--output-junit', str(junit)],
                                env=environment, stdout=log, stderr=subprocess.STDOUT)
    rows = []
    for case in ET.parse(junit).iter('testcase'):
        status = 'failed' if case.find('failure') is not None else 'passed'
        if case.find('skipped') is not None:
            status = 'skipped'
        rows.append({'name': case.attrib['name'], 'status': status,
                     'seconds': float(case.attrib.get('time', 0))})
    if {c['name'] for c in rows} != selected_names:
        raise RuntimeError('CTest did not produce a result for every selected case')
    if any(sha(builder.REPO / p) != h for p, h in source_hashes.items()):
        raise RuntimeError('A test source changed during execution')
    receipt = {**execution_plan, 'status': 'passed' if result.returncode == 0 and
               all(c['status'] == 'passed' for c in rows) else 'failed',
               'results': rows, 'ctest_exit': result.returncode,
               'full_regression_run': False}
    (args.output / 'results.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'platform': args.platform, 'status': receipt['status'],
                      'cases': len(rows), 'failures': [c for c in rows if c['status'] != 'passed']}, indent=2))
    return 0 if receipt['status'] == 'passed' else 1


if __name__ == '__main__':
    sys.exit(main())
