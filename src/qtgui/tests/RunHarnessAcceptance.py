"""Run explicitly reviewed backend acceptance contracts, with source-bound receipts.

The harness requirements are part of the original NASA denominator but do not
exercise GMAT windows or its numerical engine. No GUI pass is inferred here.
"""
import argparse
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys
import time
import xml.etree.ElementTree as ET

import AcceptanceCatalog
import NasaGuiCoverage as nasa


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def write(path, value):
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False, allow_nan=False) + '\n')


def passed_junit(path, name, returncode):
    if returncode != 0 or not path.is_file():
        return False
    try:
        root = ET.parse(path).getroot()
        cases = root.findall('.//testcase')
        return (len(cases) == 1 and cases[0].get('name') == name
                and cases[0].get('status') in (None, 'run', 'pass', 'passed')
                and not any(root.find('.//' + tag) is not None for tag in ('failure', 'error', 'skipped'))
                and not any(int(s.get(k, '0')) for s in root.iter('testsuite')
                            for k in ('failures', 'errors', 'skipped', 'disabled')))
    except (ET.ParseError, ValueError):
        return False


def preserve_text_inputs(repo, output, hashes, authorities):
    """Retain exact executed text inputs, never interpreter/runtime binaries."""
    root = output / 'sources'
    root.mkdir()
    copied, authority_inputs = {}, {}
    for source, expected in hashes.items():
        relative = Path(source)
        if relative.is_absolute() or '..' in relative.parts:
            raise ValueError('Unsafe source archive path: ' + source)
        original = repo / relative
        if sha(original) != expected:
            raise ValueError('Cannot preserve changed execution input: ' + source)
        if source in authorities:
            authority_inputs[source] = expected
            continue
        data = original.read_bytes()
        data.decode('utf-8')
        target = root / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        copied[source] = expected
    return dict(source_root=str(root), preserved_source_sha256=copied,
                authority_inputs_not_copied=authority_inputs,
                source_archive_created_after_execution=True,
                source_archive_limit='Text inputs retained only while matching before/after execution hashes. No execution inside this archive or GMAT source-to-binary qualification is claimed.')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--review', type=Path, default=Path('src/qtgui/tests/NasaHarnessTraceability.json'))
    parser.add_argument('--metadata', type=Path, default=Path('src/qtgui/tests/AcceptanceTestMetadata.json'))
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(argv)
    repo, output = args.repo.resolve(), args.output.resolve()
    build = (repo / args.build_dir).resolve()
    args.review = (repo / args.review).resolve().relative_to(repo)
    args.metadata = (repo / args.metadata).resolve().relative_to(repo)
    inventory = nasa.inventory(repo)
    review = nasa.load_review(repo, inventory, manifest=args.review)
    if not review['reviews'] or any(r['applicability'] != 'harness' for r in review['reviews']):
        raise ValueError('Only explicit harness scopes may use this runner')
    tests = sorted({e['test'] for r in review['reviews'] for e in r['evidence']})
    if not tests or any(not name.startswith('QtAcceptance.') for name in tests):
        raise ValueError('Harness review must name only acceptance support contracts')
    sources = set(inventory['sources']) | set(review.get('reference_sources', {}))
    for row in review['reviews']:
        sources.update(row.get('applicability_sources', {}))
        for claim in row['evidence']:
            sources.add(claim['source'])
            sources.update(claim.get('reference_sources', {}))
    sources.update([str(args.review), str(args.metadata),
                    'src/qtgui/tests/RunHarnessAcceptance.py', 'src/qtgui/tests/NasaGuiCoverage.py',
                    'src/qtgui/CMakeLists.txt'])
    before = {source: sha(repo / source) for source in sorted(sources)}
    output.mkdir(parents=True, exist_ok=False)
    receipt = dict(schema_version=1, started_utc=utc(), mode='harness-current-source',
                   source_hashes=before, binary_hashes={sys.executable: sha(sys.executable)},
                   platform=dict(id=platform.system()+'-'+platform.machine(),
                                 system=platform.system(), release=platform.release(),
                                 machine=platform.machine(), python=platform.python_version()),
                   results=[], full_current_GMAT_binary_qualified=False,
                   scope='Actual reviewed backend contract execution only; no GMAT GUI/engine invocation.')
    for name in tests:
        directory = output / name
        directory.mkdir()
        junit, log = directory / 'ctest.xml', directory / 'test.log'
        command = ['ctest', '--test-dir', str(build), '-C', 'Release', '-R', '^'+re.escape(name)+'$',
                   '--verbose', '--output-junit', str(junit)]
        started, tick = utc(), time.monotonic()
        env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
        try:
            process = subprocess.run(command, cwd=repo, env=env, capture_output=True, text=True, timeout=60)
            log.write_text(process.stdout + process.stderr)
            returncode = process.returncode
            status = 'passed' if passed_junit(junit, name, returncode) else 'failed'
        except subprocess.TimeoutExpired as error:
            def text(value):
                return value.decode(errors='replace') if isinstance(value, bytes) else value or ''
            log.write_text(text(error.stdout) + text(error.stderr))
            returncode, status = -9, 'timeout'
        result = dict(name=name, started_utc=started, status=status, returncode=returncode,
                      command=command, sources=sorted({e['source'] for r in review['reviews']
                                                     for e in r['evidence'] if e['test'] == name}),
                      log=str(log), log_sha256=sha(log), junit=str(junit),
                      junit_sha256=sha(junit) if junit.is_file() else None,
                      duration_seconds=time.monotonic()-tick,
                      trace={'native_exposure_events': 0, 'recorded_platforms': [], 'screenshots': []})
        receipt['results'].append(result)
        write(output / 'results.json', receipt)
    receipt.update(finished_utc=utc(), source_inputs_unchanged=all(sha(repo/p)==h for p,h in before.items()),
                   execution_counts={'executed_now': len(tests), 'reused_passes': 0})
    write(output / 'results.json', receipt)
    if not receipt['source_inputs_unchanged']:
        raise ValueError('Harness source changed during execution; no scope credit')
    report = nasa.evaluate(repo, inventory, review, receipt)
    report_path = output / 'nasa-requirements.json'
    write(report_path, report)
    metadata = json.loads((repo / args.metadata).read_text())
    catalogue = AcceptanceCatalog.catalogue(receipt, report, metadata)
    catalogue['input_sha256'] = {str(output/'results.json'): sha(output/'results.json'),
                                  str(report_path): sha(report_path),
                                  str(repo/args.metadata): sha(repo/args.metadata)}
    write(output / 'catalogue.json', catalogue)
    qualified = [r['key'] for kind in ('requirements','historical_cases')
                 for r in report[kind] if r['status']=='verified']
    qualification = dict(review=str(repo/args.review), review_sha256=sha(repo/args.review),
                         receipt=str(output/'results.json'), receipt_sha256=sha(output/'results.json'),
                         report=str(report_path), report_sha256=sha(report_path),
                         qualified_keys=qualified, source_inputs_unchanged=True,
                         full_current_build_qualified=False, full_NASA_qualified=False,
                         scope='Current source-bound harness contracts, not GMAT GUI or engine qualification.')
    qualification.update(preserve_text_inputs(repo, output, before, inventory['sources']))
    qualification['original_review'] = qualification['review']
    qualification['review'] = str(Path(qualification['source_root']) / args.review)
    write(output / 'contract-qualification.json', qualification)
    print(json.dumps({'qualified_source_scopes':len(qualified), 'executed_support_registrations':len(tests),
                      'GMAT_GUI_or_engine_cases_executed':0}))
    return 0 if (len(qualified)==len(review['reviews'])
                 and all(r['status']=='passed' for r in receipt['results'])) else 1


if __name__ == '__main__':
    sys.exit(main())
