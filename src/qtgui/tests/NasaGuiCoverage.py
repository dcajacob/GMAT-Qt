"""Source-bound NASA requirement instances and historical GUI test cases.

A mapping is not a passing requirement. Credit requires reviewed source text,
an unchanged assertion source, a passing executable, and its explicit proof
record. Duplicate/missing/retired IDs stay in the inventory for review.
"""
import argparse
from collections import Counter
import csv
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import xml.etree.ElementTree as ET
from zipfile import ZipFile

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
MATRIX = 'doc/SystemDocs/Test/Requirements-To-Test Matrix, 2012-05-11 (GMAT Build 2012-05-04).csv'
WORKBOOK = 'doc/SystemDocs/Requirements/GMAT Requirements.xlsx'
MANIFEST = 'src/qtgui/tests/NasaGuiTraceability.json'
NS = {'m': 'http://schemas.openxmlformats.org/spreadsheetml/2006/main'}
REL = '{http://schemas.openxmlformats.org/officeDocument/2006/relationships}id'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class _SourceReadCache:
    """Share repeated reads within one validation, never across validations.

    A subsequent report rehashes every referenced file. No timestamp, size or
    previous-report result is used as evidence of unchanged content.
    """
    def __init__(self, repo):
        self.repo, self.digests, self.texts = repo, {}, {}

    def matches(self, source, expected):
        if source not in self.digests:
            path = self.repo / source
            self.digests[source] = sha(path) if path.is_file() else None
        actual = self.digests[source]
        return actual is not None and actual == expected

    def text(self, source):
        if source not in self.texts:
            self.texts[source] = (self.repo / source).read_text()
        return self.texts[source]


def fingerprint(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, ensure_ascii=False).encode()).hexdigest()


def workbook_rows(path):
    with ZipFile(path) as archive:
        strings = []
        if 'xl/sharedStrings.xml' in archive.namelist():
            strings = [''.join(x.text or '' for x in item.findall('.//m:t', NS))
                       for item in ET.fromstring(archive.read('xl/sharedStrings.xml')).findall('m:si', NS)]
        relations = {x.get('Id'): x.get('Target') for x in ET.fromstring(archive.read('xl/_rels/workbook.xml.rels'))}
        sheets = ET.fromstring(archive.read('xl/workbook.xml')).findall('m:sheets/m:sheet', NS)
        for sheet in sheets:
            target = relations[sheet.get(REL)]
            target = target.lstrip('/') if target.startswith('/') else str(PurePosixPath('xl') / target)
            for row in ET.fromstring(archive.read(target)).findall('m:sheetData/m:row', NS):
                cells = {}
                for cell in row.findall('m:c', NS):
                    value = cell.find('m:v', NS)
                    text = value.text or '' if value is not None else ''
                    if cell.get('t') == 's':
                        text = strings[int(text)]
                    elif cell.get('t') == 'inlineStr':
                        text = ''.join(x.text or '' for x in cell.findall('.//m:t', NS))
                    cells[re.sub(r'[0-9]', '', cell.get('r'))] = text
                yield sheet.get('name'), int(row.get('r')), cells


def inventory(repo=REPO):
    requirements, groups = [], {}
    for sheet, row, cells in workbook_rows(repo / WORKBOOK):
        if cells.get('C'):
            groups[sheet] = cells['C']
        text, identifier = cells.get('E', ''), cells.get('D', '')
        if not text.strip() or row < (5 if sheet in ('FRR', 'FRC') else 12):
            continue
        if identifier.strip() in ('ID', '#.#.#.#'):
            continue
        source = {'sheet': sheet, 'row': row, 'declared_id': identifier,
                  'text': text, 'group': groups.get(sheet, '')}
        requirements.append({'key': f'{sheet}!E{row}', **source,
                             'fingerprint': fingerprint(source), 'applicability': 'unreviewed'})
    counts = Counter(x['declared_id'] for x in requirements if x['declared_id'])
    for item in requirements:
        item['id_ambiguous'] = not item['declared_id'] or counts[item['declared_id']] > 1
        item['identifier_issues'] = []
        if not re.fullmatch(r'(?:FRR|FRC|FRAC|EIR|ER|CRR|TSR)-[0-9]+(?:\.[0-9]+)*', item['declared_id']):
            item['identifier_issues'].append('missing, placeholder or nonstandard identifier; retain exact source spelling')
        if counts[item['declared_id']] > 1:
            item['identifier_issues'].append('duplicate identifier; review each source instance separately')
    with (repo / MATRIX).open(newline='', encoding='utf-8-sig') as stream:
        historical = [{'key': f'H2012:R{row}', 'row': row, 'source_fields': fields,
                       'fingerprint': fingerprint(fields), 'applicability': 'unreviewed'}
                      for row, fields in enumerate(csv.DictReader(stream), 2)]
    modified = None
    with ZipFile(repo / WORKBOOK) as archive:
        if 'docProps/core.xml' in archive.namelist():
            value = ET.fromstring(archive.read('docProps/core.xml')).find('{http://purl.org/dc/terms/}modified')
            modified = value.text if value is not None else None
    return {'schema_version': 1, 'sources': {WORKBOOK: sha(repo / WORKBOOK), MATRIX: sha(repo / MATRIX)},
            'source_metadata': {'requirements_workbook_modified': modified,
                                'authority_limit': 'Shipped historical workbook; current NASA baseline completeness requires reconciliation.'},
            'requirements': requirements, 'historical_cases': historical}


def _valid_terminal_marker(marker):
    if not isinstance(marker, str):
        return False
    if re.match(r'^PASS(?:[: ])', marker):
        return True
    if marker == ('Qt resource round trips: changed-assignment snapshots, removed owned fields, '
                  'dotted continuations, grouped Array resize, exact source/Undo/Redo, preserved '
                  'implicit power epoch and unused body defaults, mixed power/empty shadows, '
                  'invalid correction, implicit mission, grouped deletion, referenced/built-in '
                  'rejection, create/delete/Undo/Redo and Unicode save/reopen matched independent script reports'):
        return True
    # Source-bound native rendering assertions predate NASA_CHECK. This exact
    # whole-case terminal line is never a generic suffix or substring match.
    if marker == ('Native texture, depth, rotation, resize, model material, Sun illumination, '
                  'star catalog/camera/replay/occlusion, fallback and lifecycle checks passed'):
        return True
    # This preserved inspection case predates NASA_CHECK. Accept only its
    # exact completion line; source anchors, hashes, JUnit and unique runtime
    # occurrence are still required by legacy_assertions_verified.
    if marker == ('Qt inspections: previews, pending edits, command/mission states, four frames, '
                  'physics selection, skipped branches, script events, search/copy/export, '
                  'stale/failure/stop recovery passed'):
        return True
    # Existing solver matrices prefix their terminal PASS with the exercised
    # solver context. Retain the whole original line: exact log matching below
    # must still reject changed contexts, duplicate records and quoted output.
    if re.match(r'^(?:Differential corrector|Yukon optimization|Nested differential correctors|Yukon): PASS(?:[: ])', marker):
        return True
    # These two exact retained ephemeris cases predate NASA_CHECK. Their
    # complete format-specific terminal records are still bound to the
    # assertion source, unique log occurrence, JUnit and original receipt.
    ephemeris_completion = (
        ': PASS actual MDI Ephemeris/Orbit/Ground/XY ordered Toggle selection, '
        'empty filtering/Cancel/pending/retained Apply, exact source/Undo/Redo/Unicode Save/Save As/reopen; '
        'initially disabled ephemeris activation, final Off through 420 s, every ephemeris state analytically circular '
        'and full script-reference rows/reports equal, metadata/finalization/Output access, '
        'invalid-subscriber rollback/correction and clean repeat-run; '
        'viewer two-arc suppression/replay/camera/close-reopen; two enabled output arcs (0-120/240-360).')
    if marker in {kind + ephemeris_completion for kind in ('CCSDS-OEM', 'STK-TimePosVel')}:
        return True
    # Existing visible companions emitted structured route receipts before
    # NASA_CHECK existed. Reuse their exact terminal record and source anchors;
    # do not synthesize a NASA proof or rewrite the preserved execution log.
    if not marker.startswith('GUI_ROUTE_CHECK '):
        return False
    try:
        record = json.loads(marker[len('GUI_ROUTE_CHECK '):])
    except ValueError:
        return False
    return (isinstance(record, dict) and record.get('outcome') == 'passed'
            and isinstance(record.get('kind'), str) and bool(record['kind'])
            and all(isinstance(record.get(key), list) and bool(record[key])
                    and all(isinstance(value, str) and value for value in record[key])
                    for key in ('controls', 'checks')))


def _valid_legacy_assertions(specification, source_text):
    if not isinstance(specification, dict):
        return False
    markers = specification.get('terminal_markers')
    anchors = specification.get('assertion_anchors')
    return (isinstance(markers, list) and bool(markers) and
            isinstance(anchors, list) and bool(anchors) and
            all(_valid_terminal_marker(x) for x in markers) and
            len(set(markers)) == len(markers) and
            all(isinstance(x, str) and 'require(' in x and x in source_text for x in anchors))


def load_review(repo, source_inventory, manifest=None):
    sources = _SourceReadCache(repo)
    path = repo / (manifest if manifest is not None else MANIFEST)
    review = json.loads(path.read_text()) if path.is_file() else {'sources': source_inventory['sources'], 'reviews': []}
    if review.get('schema_version', 1) != 1 or review['sources'] != source_inventory['sources']:
        raise ValueError('NASA source inventory changed: review and rebind the traceability manifest.')
    for source, digest in review.get('reference_sources', {}).items():
        if not sources.matches(source, digest):
            raise ValueError('NASA reference source changed: ' + source)
    entries = {item['key']: item for item in source_inventory['requirements'] + source_inventory['historical_cases']}
    seen = set()
    for record in review['reviews']:
        key = record['key']
        if key in seen or key not in entries:
            raise ValueError('Duplicate or unknown NASA source key: ' + key)
        seen.add(key)
        if record['fingerprint'] != entries[key]['fingerprint']:
            raise ValueError('NASA source row changed: ' + key)
        if record['applicability'] not in ('gui', 'harness', 'non-gui', 'retired', 'external', 'conflict') or not record.get('rationale'):
            raise ValueError('An applicability decision needs a reviewed rationale: ' + key)
        for source, digest in record.get('applicability_sources', {}).items():
            if not sources.matches(source, digest):
                raise ValueError('NASA applicability source changed: ' + source)
        for evidence in record.get('evidence', []):
            for reference, digest in evidence.get('reference_sources', {}).items():
                if not sources.matches(reference, digest):
                    raise ValueError('NASA claim reference source changed: ' + reference)
            source = evidence['source']
            if not sources.matches(source, evidence['source_sha256']):
                raise ValueError('NASA assertion source changed: ' + evidence['source'])
            if evidence['anchor'] not in sources.text(source):
                raise ValueError('NASA assertion anchor missing: ' + key)
            if not evidence.get('facets') or not evidence.get('scope'):
                raise ValueError('NASA evidence needs explicit facets and scope: ' + key)
            if 'legacy_assertions' in evidence:
                if (evidence.get('expected') or
                        not _valid_legacy_assertions(evidence['legacy_assertions'], sources.text(source))):
                    raise ValueError('Legacy evidence needs exact terminal PASS markers and reviewed source assertions: ' + key)
    return review


def proof_records(result):
    if result.get('status') != 'passed' or not result.get('log'):
        return []
    log = Path(result['log'])
    if not log.is_file():
        return []
    proofs = []
    for line in log.read_text(errors='replace').splitlines():
        match = re.match(r'^\s*(?:\d+:\s*)?NASA_CHECK (\{.*\})\s*$', line)
        if match:
            try:
                record = json.loads(match[1])
            except ValueError:
                continue
            if record.get('outcome') == 'passed' and not record.get('diagnostic_only', False):
                if isinstance(record.get('checkpoint'), str) and isinstance(record.get('facets'), list) and all(isinstance(x, str) for x in record['facets']):
                    proofs.append(record)
    return proofs


def legacy_assertions_verified(repo, result, claim):
    """Recognize reviewed pre-NASA_CHECK tests without altering their receipts.

    This credits source-bound assertions reached before exact terminal markers,
    not synthetic numerical payloads, saved report values or visible exposure.
    Snapshot integrity and production-version qualification remain separate.
    """
    if (result.get('status') != 'passed' or result.get('returncode') != 0 or
            result.get('name') != claim.get('test')):
        return False
    try:
        source = Path(repo) / claim['source']
        if (claim['source'] not in result.get('sources', []) or
                sha(source) != claim['source_sha256'] or claim.get('expected')):
            return False
        legacy = claim['legacy_assertions']
        if not _valid_legacy_assertions(legacy, source.read_text()):
            return False
        markers = legacy['terminal_markers']
        for key in ('log', 'junit'):
            if sha(result[key]) != result.get(key + '_sha256'):
                return False
        document = ET.parse(result['junit']).getroot()
        cases = document.findall('.//testcase')
        if (len(cases) != 1 or cases[0].get('name') != claim['test'] or
                cases[0].get('status') not in (None, 'run', 'passed', 'pass')):
            return False
        if any(int(suite.get(key, '0')) != 0 for suite in document.iter('testsuite')
               for key in ('failures', 'errors', 'skipped', 'disabled')):
            return False
        if any(document.find('.//' + tag) is not None for tag in ('failure', 'error', 'skipped')):
            return False
        lines = [re.sub(r'^\s*\d+:\s?', '', line).strip()
                 for line in Path(result['log']).read_text(errors='replace').splitlines()]
        return all(lines.count(marker) == 1 for marker in markers)
    except (OSError, KeyError, TypeError, ValueError, ET.ParseError):
        return False


def evaluate(repo, source_inventory, review, receipt=None):
    sources = _SourceReadCache(repo)
    receipt = receipt or {}
    results = {x['name']: x for x in receipt.get('results', [])}
    reviews = {x['key']: x for x in review['reviews']}
    references_match = all(sources.matches(source, digest)
                           for source, digest in review.get('reference_sources', {}).items())
    run_references_match = all(receipt.get('source_hashes', {}).get(source) == digest
                               for source, digest in review.get('reference_sources', {}).items())
    proof_cache, source_cache, legacy_cache = {}, {}, {}
    output = {key: source_inventory[key] for key in ('schema_version', 'sources', 'source_metadata')}
    output['execution'] = {'started_utc': receipt.get('started_utc'), 'mode': receipt.get('mode'),
                           'source_hashes_fingerprint': fingerprint(receipt.get('source_hashes', {})),
                           'binary_hashes_fingerprint': fingerprint(receipt.get('binary_hashes', {}))}
    for collection in ('requirements', 'historical_cases'):
        output[collection] = []
        for item in source_inventory[collection]:
            record = reviews.get(item['key'])
            row = {**item, 'status': 'unreviewed', 'evidence': []}
            if record:
                row.update(applicability=record['applicability'], rationale=record['rationale'], status='reviewed-unverified')
                if record.get('applicability_sources'):
                    row['applicability_sources'] = record['applicability_sources']
                for claim in record.get('evidence', []):
                    result = results.get(claim['test'], {})
                    source_key = (claim['source'], claim['source_sha256'])
                    if source_key not in source_cache:
                        source_cache[source_key] = (receipt.get('source_hashes', {}).get(claim['source']) == claim['source_sha256']
                                                    and sources.matches(claim['source'], claim['source_sha256']))
                    claim_references_match = all(
                        sources.matches(source, digest)
                        and receipt.get('source_hashes', {}).get(source) == digest
                        for source, digest in claim.get('reference_sources', {}).items())
                    source_matches = references_match and run_references_match and claim_references_match and source_cache[source_key]
                    if 'legacy_assertions' in claim:
                        cache_key = (claim['test'], claim['source'], claim['source_sha256'],
                                     fingerprint(claim['legacy_assertions']), fingerprint(claim.get('expected')))
                        if cache_key not in legacy_cache:
                            legacy_cache[cache_key] = legacy_assertions_verified(repo, result, claim)
                        matches = source_matches and legacy_cache[cache_key]
                    else:
                        if claim['test'] not in proof_cache:
                            proof_cache[claim['test']] = proof_records(result)
                        proofs = proof_cache[claim['test']] if source_matches and claim['source'] in result.get('sources', []) else []
                        matches = [p for p in proofs if p.get('checkpoint') == claim['checkpoint']
                                   and set(claim['facets']) <= set(p.get('facets', []))
                                   and all(p.get(k) == v for k, v in claim.get('expected', {}).items())]
                    row['evidence'].append({**claim, 'verified_in_run': bool(matches),
                                            'result': result.get('status', 'not-run'), 'log': result.get('log'),
                                            'native_exposure_events': result.get('trace', {}).get('native_exposure_events', 0)})
                if record['applicability'] in ('gui', 'harness') and row['evidence'] and all(e['verified_in_run'] for e in row['evidence']):
                    row['status'] = 'verified'
                elif row['evidence']:
                    row['status'] = 'mapped-unverified'
            output[collection].append(row)
    output['counts'] = {collection: dict(Counter(x['status'] for x in output[collection]))
                        for collection in ('requirements', 'historical_cases')}
    output['source_conflicts'] = review.get('source_conflicts', [])
    output['reference_sources_unchanged'] = references_match
    output['reference_sources_match_run'] = run_references_match
    output['limits'] = ['Historical operation rows and requirement instances are distinct denominators.',
                        'Verified means the reviewed scope matched explicit passing proofs in the supplied run receipt; it is not whole-GUI certification.',
                        'The shipped requirements workbook was modified in 2012; it is not proof of a current complete NASA requirements baseline.',
                        'Applicability reviews do not themselves verify behavior or remove rows.',
                        'Setting-property evidence does not establish scientific model accuracy.',
                        'Complete widget/input/platform/stress coverage requires separate evidence.']
    return output


def write_report(output, report):
    output.mkdir(parents=True, exist_ok=True)
    (output / 'nasa-requirements.json').write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n')
    lines = ['# NASA GUI traceability', '', 'Requirements and historical operations are counted separately. No old Passed field is current evidence.', '']
    lines += ['Workbook modification metadata: ' + str(report['source_metadata']['requirements_workbook_modified']) +
              '. Current NASA baseline completeness remains unqualified.', '']
    for collection in ('requirements', 'historical_cases'):
        lines += [f"- {collection}: {len(report[collection])}; {report['counts'][collection]}"]
    lines += ['', '## Reviewed requirements and cases', '']
    for item in report['requirements'] + report['historical_cases']:
        if item['status'] != 'unreviewed':
            lines.append(f"- **{item['key']} {item.get('declared_id', '')}**: {item['status']}; {item['rationale']}")
    lines += ['', '## Source conflicts', '']
    for conflict in report['source_conflicts']:
        lines.append(f"- **{conflict['id']}**: {conflict['detail']} Sources: " + ', '.join(conflict['sources']))
    lines += ['', '## Limits', '', *['- ' + line for line in report['limits']], '']
    (output / 'nasa-traceability.md').write_text('\n'.join(lines))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--results', type=Path)
    args = parser.parse_args()
    source_inventory = inventory()
    review = load_review(REPO, source_inventory)
    receipt = json.loads(args.results.read_text()) if args.results else None
    report = evaluate(REPO, source_inventory, review, receipt)
    write_report(args.output, report)
    print(json.dumps(report['counts'], indent=2))


if __name__ == '__main__':
    main()
