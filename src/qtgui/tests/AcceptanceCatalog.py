"""Metadata, requirement queries and explicit summaries for acceptance receipts.

This describes supplied evidence. It never executes tests, changes fixture
tolerances or promotes a mapping into NASA qualification.
"""
import argparse
from collections import Counter, defaultdict
import datetime as dt
import hashlib
import json
import math
from pathlib import Path


METADATA_FIELDS = ('earliest_build_date', 'latest_build_date', 'requirements',
                   'author', 'truth_data', 'tolerances', 'comparator', 'extensions',
                   'comments', 'creation_date', 'platform_dependencies', 'test_type')
TEST_TYPES = {'Acceptance', 'System', 'Validation'}
STATUSES = {'passed', 'failed', 'timeout', 'interrupted', 'skipped', 'not-run'}


def need(value, message):
    if not value:
        raise ValueError(message)


def date(value):
    if value is None:
        return None
    need(isinstance(value, str), 'Date must be an ISO calendar date or null')
    parsed = dt.date.fromisoformat(value)
    need(parsed.isoformat() == value, 'Date must use YYYY-MM-DD')
    return parsed


def strings(values, label):
    need(isinstance(values, list) and all(isinstance(x, str) and x.strip() for x in values)
         and len(values) == len(set(values)), label + ' must be unique nonempty strings')


def finite_json(value):
    # JSON NaN/Infinity are accepted by Python's decoder, but are not valid
    # truth/extension data and must not silently survive an export/import.
    json.dumps(value, allow_nan=False)


def tolerance(value):
    need(isinstance(value, dict) and not (set(value) - {'absolute', 'relative', 'fields'}),
         'Unknown tolerance policy')
    for name in ('absolute', 'relative'):
        if name in value:
            number = value[name]
            need(isinstance(number, (int, float)) and not isinstance(number, bool)
                 and math.isfinite(number) and number >= 0, 'Invalid ' + name + ' tolerance')
    if 'fields' in value:
        need(isinstance(value['fields'], dict), 'Field tolerances must be an object')
        for field, policy in value['fields'].items():
            need(isinstance(field, str) and field and isinstance(policy, dict) and 'fields' not in policy,
                 'Invalid field tolerance')
            tolerance(policy)


def validate_metadata(document):
    need(document.get('schema_version') == 1 and isinstance(document.get('tests'), dict),
         'Unsupported acceptance metadata schema')
    for name, record in document['tests'].items():
        need(isinstance(name, str) and name.strip() and isinstance(record, dict),
             'Test metadata needs a nonempty name and object')
        need(not (set(record) - set(METADATA_FIELDS)), 'Unknown metadata field for ' + name)
        finite_json(record)
        earliest = date(record.get('earliest_build_date'))
        latest = date(record.get('latest_build_date'))
        date(record.get('creation_date'))
        need(earliest is None or latest is None or earliest <= latest,
             'Earliest supported build is after latest')
        for field in ('requirements', 'platform_dependencies'):
            if field in record:
                strings(record[field], field)
        for field in ('author', 'comments', 'comparator'):
            need(field not in record or record[field] is None or isinstance(record[field], str),
                 field + ' must be text or null')
        if 'test_type' in record:
            need(record['test_type'] in TEST_TYPES or record['test_type'] is None,
                 'Test type must be Acceptance, System or Validation')
        if 'extensions' in record:
            need(isinstance(record['extensions'], dict), 'Extensions must be an object')
        if 'tolerances' in record and record['tolerances'] is not None:
            tolerance(record['tolerances'])
        if 'truth_data' in record:
            need(isinstance(record['truth_data'], list), 'Truth data must be a list')
            for truth in record['truth_data']:
                need(isinstance(truth, dict) and truth.get('kind') in {'file', 'literal'},
                     'Truth data needs file or literal kind')
                if truth['kind'] == 'file':
                    digest = truth.get('sha256')
                    need(isinstance(truth.get('path'), str) and truth['path']
                         and isinstance(digest, str) and len(digest) == 64
                         and all(c in '0123456789abcdef' for c in digest),
                         'File truth needs a path and SHA256 identity')
                else:
                    need('value' in truth, 'Literal truth needs its value')
    return document


def metadata_eligibility(record, build_date=None, platform=None):
    """Unknown constraints remain unknown; dependency labels are exact labels."""
    build = date(build_date)
    first, last = date(record.get('earliest_build_date')), date(record.get('latest_build_date'))
    build_state = 'unknown'
    if build is not None and (first is not None or last is not None):
        build_state = 'eligible' if (first is None or build >= first) and (last is None or build <= last) else 'ineligible'
    dependencies = record.get('platform_dependencies')
    platform_state = 'unknown'
    if platform is not None and dependencies is not None:
        platform_state = 'eligible' if not dependencies or platform in dependencies else 'ineligible'
    return dict(build=build_state, platform=platform_state)


def platform_id(receipt, result):
    # xcb/offscreen/Wayland describe Qt backends, not the host operating system.
    value = result.get('platform', receipt.get('platform'))
    if isinstance(value, dict):
        value = value.get('id')
    return value if isinstance(value, str) and value else 'unknown'


def execution_kind(result):
    status = result.get('status', 'not-run')
    if result.get('reused_from'):
        need(status == 'passed', 'Only passing results may be reused')
        return 'reused'
    if status == 'not-run':
        return 'not-run'
    if result.get('command') and result.get('returncode') is not None:
        return 'executed-in-recorded-run'
    return 'execution-unproven'


def scope_state(row, tests):
    links = row['tests']
    if not links:
        return 'no-tests'
    statuses = [tests[name]['status'] for name in links]
    if any(s in {'failed', 'timeout'} for s in statuses):
        return 'failed'
    if 'interrupted' in statuses:
        return 'interrupted'
    if 'not-run' in statuses:
        return 'unexecuted'
    if 'skipped' in statuses:
        return 'skipped'
    # A metadata association does not supply a reviewed passing proof.
    evidence = row['evidence']
    if (row['reported_status'] == 'verified' and evidence
            and all(e.get('verified_in_run') for e in evidence)):
        return 'verified-in-supplied-report'
    return 'unverified-evidence'


def catalogue(receipt, nasa_report, metadata=None, build_date=None, platform=None):
    metadata = validate_metadata(metadata or {'schema_version': 1, 'tests': {}})
    results = receipt.get('results', [])
    need(isinstance(results, list), 'Receipt results must be a list')
    by_name = {}
    for result in results:
        name, status = result.get('name'), result.get('status')
        need(isinstance(name, str) and name and name not in by_name, 'Missing or duplicate test name')
        need(status in STATUSES, 'Unknown test status for ' + name)
        need(not (status == 'passed' and result.get('returncode') not in (None, 0)),
             'Passed result has nonzero returncode')
        by_name[name] = result
    scopes, links = {}, defaultdict(set)
    for kind in ('requirements', 'historical_cases'):
        for source in nasa_report.get(kind, []):
            key = source['key']
            need(key not in scopes, 'Duplicate requirement/historical source key')
            # Keep the query index small. Exact sources, numeric oracles and
            # full claims remain in the source report identified by its hash.
            evidence = [{k: e[k] for k in ('test', 'verified_in_run', 'result') if k in e}
                        for e in source.get('evidence', [])]
            names = sorted({e['test'] for e in evidence if e.get('test')})
            scopes[key] = dict(key=key, kind=kind, declared_id=source.get('declared_id'),
                               name=source.get('text', key), fingerprint=source.get('fingerprint'),
                               reported_status=source['status'], evidence=evidence, tests=names)
            for name in names:
                links[name].add(key)
    for name, record in metadata['tests'].items():
        for key in record.get('requirements', []):
            need(key in scopes, 'Metadata references an unknown requirement: ' + key)
            links[name].add(key)
            scopes[key]['tests'] = sorted(set(scopes[key]['tests']) | {name})
    names = sorted(set(by_name) | set(metadata['tests']) | set(links))
    tests = {}
    for name in names:
        result = by_name.get(name, {'name': name, 'status': 'not-run',
                                    'reason': 'No result in the supplied receipt'})
        record = metadata['tests'].get(name, {})
        tests[name] = dict(name=name, status=result['status'],
                           execution=execution_kind(result),
                           test_type=record.get('test_type') or 'unclassified',
                           platform=platform_id(receipt, result),
                           requirements=sorted(links[name]), metadata=record,
                           metadata_unspecified=[k for k in METADATA_FIELDS
                                                 if k not in record or record[k] is None],
                           eligibility=metadata_eligibility(record, build_date, platform),
                           source_log=result.get('log'), reused_from=result.get('reused_from'),
                           reason=result.get('reason'))
    for row in scopes.values():
        row['state'] = scope_state(row, tests)
    failures = [t for t in tests.values() if t['status'] in {'failed', 'timeout'}]
    groups = defaultdict(list)
    for test in failures:
        groups[(test['test_type'], test['platform'])].append(test['name'])
    failed_requirements = [r for r in scopes.values() if r['kind'] == 'requirements' and r['state'] == 'failed']
    summary = dict(tests_in_receipt=len(results), tests_in_catalogue=len(tests),
                   tests_run=sum(t['execution'] == 'executed-in-recorded-run' for t in tests.values()),
                   executed_status_counts=dict(Counter(t['status'] for t in tests.values()
                                                       if t['execution'] == 'executed-in-recorded-run')),
                   reused_passes=sum(t['execution'] == 'reused' for t in tests.values()),
                   execution_unproven=sum(t['execution'] == 'execution-unproven' for t in tests.values()),
                   tests_passed=sum(t['status'] == 'passed' for t in tests.values()),
                   tests_failed=len(failures), failed_test_names=sorted(t['name'] for t in failures),
                   failed_requirements=len(failed_requirements),
                   failed_requirement_keys=sorted(r['key'] for r in failed_requirements),
                   failed_requirement_names=[dict(key=r['key'], declared_id=r['declared_id'], name=r['name'])
                                             for r in sorted(failed_requirements, key=lambda r: r['key'])],
                   status_counts=dict(Counter(t['status'] for t in tests.values())),
                   scope_states={kind: dict(Counter(r['state'] for r in scopes.values() if r['kind'] == kind))
                                 for kind in ('requirements', 'historical_cases')})
    return dict(schema_version=1, tests=list(tests.values()), scopes=list(scopes.values()), summary=summary,
                failed_by_type_platform=[dict(test_type=kind, platform=host, tests=sorted(values))
                                         for (kind, host), values in sorted(groups.items())],
                requirements_without_tests=sorted(k for k, r in scopes.items() if not r['tests']),
                recorded_run=dict(started_utc=receipt.get('started_utc'), finished_utc=receipt.get('finished_utc'),
                                  mode=receipt.get('mode'), snapshot_id=receipt.get('snapshot_id')),
                scope='Summary/query of supplied evidence, not new execution or requalification. Metadata tolerances never override fixture assertions; unknown authors,dates,platforms and truth stay explicit.')


def query_requirements(catalog, keys):
    wanted = set(keys)
    known = {r['key'] for r in catalog['scopes']}
    need(wanted <= known, 'Unknown requested requirement: ' + ','.join(sorted(wanted - known)))
    return [t for t in catalog['tests'] if wanted.intersection(t['requirements'])]


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--receipt', type=Path, required=True)
    parser.add_argument('--nasa-report', type=Path, required=True)
    parser.add_argument('--metadata', type=Path)
    parser.add_argument('--build-date')
    parser.add_argument('--platform', help='Dependency filter label, not a measurement of old execution platform')
    parser.add_argument('--query', choices=('all', 'failed', 'no-tests', 'requirements', 'summary'), default='all')
    parser.add_argument('--requirement', action='append', default=[])
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(argv)
    read = lambda path: json.loads(path.read_text())
    result = catalogue(read(args.receipt), read(args.nasa_report),
                       read(args.metadata) if args.metadata else None, args.build_date, args.platform)
    result['input_sha256'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                              for path in (args.receipt, args.nasa_report, args.metadata) if path}
    if args.query == 'requirements':
        need(bool(args.requirement), 'Requirement query needs at least one source key')
        output = query_requirements(result, args.requirement)
    elif args.query == 'failed':
        output = result['failed_by_type_platform']
    elif args.query == 'no-tests':
        output = result['requirements_without_tests']
    elif args.query == 'summary':
        output = result['summary']
    else:
        output = result
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, indent=2, ensure_ascii=False, allow_nan=False) + '\n')
    return result


if __name__ == '__main__':
    main()
