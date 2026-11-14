"""Apply the B-1 oracle to source-bound, previously checked GUI OEM artifacts.

Consumes the existing sampling audit rather than rerunning its missions.
Never rewrites that audit, its receipts, assertions or coverage decisions.
Nonzero exit means a format violation, missing convention, or broken evidence.
This is an additional diagnostic; it is not a NASA requirement admission tool.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET

from AuditAcceptanceRecovery import payload
import AcceptanceSnapshot as snapshot
from CcsdsOemB1Oracle import ConformanceError, validate


STANDARD_SHA256 = 'fab306a994e5bce9cad8c64d77af172a5819af38faf459faab1b7e2f6163223c'
TEST_SOURCE = 'src/qtgui/tests/NasaEphemerisSamplingTests.cpp'


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def need(ok, message):
    if not ok:
        raise ValueError(message)


def audit(sampling_audit, standard):
    audit_bytes, standard_bytes = payload(sampling_audit), payload(standard)
    need(sha(standard_bytes) == STANDARD_SHA256, 'Wrong original B-1 authority PDF')
    prior = json.loads(audit_bytes)
    need(prior['versioned_collection'] is True, 'Prior audit is not source versioned')
    observations, seen = [], set()
    for cohort in prior['source_bound_evidence']['visible']:
        receipt_bytes = payload(cohort['receipt'])
        need(sha(receipt_bytes) == cohort['receipt_sha256'], 'Prior receipt changed')
        receipt = json.loads(receipt_bytes)
        need(receipt['snapshot_unchanged'] and receipt['snapshot_id'] == cohort['snapshot_id'],
             'Prior receipt/source identity differs')
        manifest_bytes = payload(receipt['snapshot_manifest'])
        manifest = json.loads(manifest_bytes)
        identity = dict(manifest)
        need(identity.pop('snapshot_id') == snapshot.fingerprint(identity)
             and manifest['snapshot_id'] == cohort['snapshot_id'], 'Snapshot identity differs')
        frozen = Path(manifest['root'])
        registered = {case['name']: case for case in manifest['cases']}
        for detail in cohort['details']:
            name = detail['test']
            need(name not in seen, 'Duplicate prior test: ' + name)
            seen.add(name)
            matches = [r for r in receipt['results'] if r['name'] == name]
            need(len(matches) == 1, 'Missing/duplicate prior case: ' + name)
            result = matches[0]
            need(result['status'] == 'passed' and result['returncode'] == 0,
                 'Prior GUI case did not pass: ' + name)
            need(result['snapshot_id'] == cohort['snapshot_id']
                 and snapshot.case_fingerprint(registered[name]) == result['case_fingerprint']
                 and registered[name]['command'] == result['registered_command'],
                 'Prior registered case changed: ' + name)
            need(result['requested_visibility'] == 'visible'
                 and result['execution_profile']['visible']
                 and result['execution_profile']['start_minimized']
                 and not result['execution_profile']['fixed_size'], 'Prior viewing policy differs')
            source_sha = sha(payload(frozen / TEST_SOURCE))
            need(source_sha == detail['source_sha256']
                 == manifest['artifact_hashes'][TEST_SOURCE]
                 == receipt['source_hashes'][TEST_SOURCE], 'Prior assertion source changed')
            binaries = {p: h for p, h in receipt['binary_hashes'].items()
                        if p in result['registered_command']}
            need(bool(binaries), 'Missing recorded native executable identity')
            for path, digest in binaries.items():
                need(sha(payload(path)) == digest, 'Prior executable changed: ' + path)
            log_bytes, junit_bytes = payload(result['log']), payload(result['junit'])
            need(result['log'] == detail['log']
                 and sha(log_bytes) == result['log_sha256'] == detail['log_sha256']
                 and sha(junit_bytes) == result['junit_sha256'], 'Prior log/JUnit changed')
            cases = list(ET.fromstring(junit_bytes).iter('testcase'))
            need(len(cases) == 1 and not any(cases[0].find(tag) is not None
                                           for tag in ('failure', 'error', 'skipped')),
                 'Prior JUnit is not one passing case')
            directory = Path(result['log']).parent
            raw = payload(directory / 'mission.oem')
            need(sha(raw) == detail['oem_sha256'], 'Previously audited OEM bytes changed')
            need(sha(payload(directory / 'segments.json')) == detail['segments_sha256'],
                 'Previously audited numerical record witness changed')
            try:
                finding = validate(raw)  # Deliberately no invented ICD for SatId.
            except ConformanceError as error:
                finding = dict(status='failed', clause=error.clause, line=error.line,
                               message=error.message, sha256=sha(raw))
            observations.append(dict(test=name, file=str(directory / 'mission.oem'),
                                     receipt=cohort['receipt'], receipt_sha256=sha(receipt_bytes),
                                     snapshot_id=cohort['snapshot_id'], source_sha256=source_sha,
                                     original_assertions_passed=True, finding=finding))
    expected = {'QtGui.NasaEphemerisSampling' + key
                for key in ('FixedAligned', 'FixedRemainder', 'RawSteps', 'ShortArc',
                            'UserInterval', 'AssignmentBoundary', 'ManeuverBoundary',
                            'DynamicsBoundary', 'ToggleGap')}
    need(seen == expected, 'Original nine-case sampling conjunction differs')
    counts = dict(Counter(item['finding']['status'] for item in observations))
    return dict(status='passed' if counts == {'passed': 9} else 'open-gaps',
                standard=dict(name='CCSDS 502.0-B-1, September 2004', path=str(standard),
                              sha256=STANDARD_SHA256, url='https://ccsds.org/Pubs/502x0b1s.pdf',
                              inspected_clauses=['Tables 4-1, 4-2, 4-3', '4.3.1-4.3.7']),
                prior_sampling_audit=str(sampling_audit), prior_sampling_audit_sha256=sha(audit_bytes),
                oracle_sha256=sha(payload(Path(__file__).with_name('CcsdsOemB1Oracle.py'))),
                counts=counts, observations=observations, native_missions_replayed=0,
                new_requirement_credit=[], product_changes=[], core_changes=[],
                limits=['Source-versioned retained output evidence; not current-build replay',
                        'No invented ICD agreement for synthetic SatId',
                        'Original numerical and metadata assertions remain unchanged',
                        'Format/support checks do not replace independent trajectory/frame oracles',
                        'No creation-clock accuracy or complete B-1/NASA qualification inferred'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sampling-audit', required=True, type=Path)
    parser.add_argument('--standard-pdf', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    report = audit(args.sampling_audit, args.standard_pdf)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in
                      ('status', 'counts', 'native_missions_replayed', 'new_requirement_credit')}))
    return 0 if report['status'] == 'passed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
