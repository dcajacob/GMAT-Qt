"""Replay identifier, precision, epoch and covariance unit-display evidence."""
from pathlib import Path
import hashlib
import json
import tarfile
import xml.etree.ElementTree as ET

load = lambda p: json.loads(Path(p).read_text())
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def need(value, message):
    if not value:
        raise ValueError(message)


EXPECTED = {name: {'NAIFId': 'identifier'} for name in ['Asteroid', 'Comet', 'Moon', 'Planet']}
EXPECTED.update(SpacecraftIDs={'NAIFId': 'identifier', 'NAIFIdReferenceFrame': 'identifier'},
                TrackingCodes={'SimTDRSSmarId': 'identifier', 'SimTDRSTrackerType': 'type code'},
                MessagePrecision={'Precision': 'digits'}, MOEEq={'Epoch': 'A1ModJulian'},
                MOEEc={'Epoch': 'A1ModJulian'})


def covered():
    return {(t, 'NAIFId') for t in ['Asteroid', 'Comet', 'Moon', 'Planet', 'Spacecraft']} | {
        ('Spacecraft', 'NAIFIdReferenceFrame'), ('TrackingFileSet', 'SimTDRSSmarId'),
        ('TrackingFileSet', 'SimTDRSTrackerType'), ('CoordinateSystem', 'Epoch'),
        ('Spacecraft', 'OrbitErrorCovariance'), ('MessageWindow', 'Dec')}


def observations(proof, cases, failures):
    expected = []
    for case in cases:
        need(case['units'] == EXPECTED[case['key']], 'Unit expectation differs from independent definition')
        expected.extend((case['key'], field, unit) for field, unit in sorted(case['units'].items()))
    need(proof['cases'] == [c['key'] for c in cases] and len(proof['observations']) == len(expected),
         'Incomplete terminal unit inventory')
    bad = 0
    for row, (key, field, unit) in zip(proof['observations'], expected):
        need((row['case'], row['field'], row['expected_unit']) == (key, field, unit), 'Unit observation identity changed')
        if row['route'] == 'body':
            passed = unit in row['actual_label']
        else:
            passed = row['actual_unit'] == unit and unit in row['visible_unit_or_caption']
        need(row['passed'] == passed, 'Incorrect terminal result flag')
        bad += not passed
    need(bad == failures == proof['mismatches'] and proof['status'] == ('failed' if bad else 'passed'),
         'Terminal failure/pass count changed')
    need(proof['source_native_history_file_isolation'] and proof['mission_executions'] == 0 and
         proof['new_requirement_credit'] == 0, 'Terminal unit inspection changed state')


def covariance(proof):
    expected = []
    for mode, labels in [('CartesianState', ['X (km)', 'Y (km)', 'Z (km)', 'VX (km/s)', 'VY (km/s)', 'VZ (km/s)']),
                         ('KeplerianState', ['SMA (km)', 'ECC', 'INC (deg)', 'RAAN (deg)', 'AOP (deg)', 'MA (deg)'])]:
        expected.extend(dict(mode=mode, axis=axis, index=i, label=label)
                        for i, label in enumerate(labels) for axis in ['row', 'column'])
        expected.append(dict(mode=mode, axis='product-rule'))
    need(len(proof['observations']) == 26, 'Covariance basis is incomplete')
    for actual, wanted in zip(proof['observations'], expected):
        need(all(actual.get(k) == v for k, v in wanted.items()), 'Covariance unit basis differs')
        if wanted['axis'] == 'product-rule':
            need('product of its row and column units' in actual['help'] and
                 'does not convert these values' in actual['help'], 'Covariance product/pending explanation missing')
    need(proof['status'] == 'passed' and proof['matrix_unchanged'] and
         proof['source_native_history_file_unchanged'] and proof['mission_executions'] == 0 and
         proof['new_requirement_credit'] == 0, 'Covariance display changed input state')


def definitions(stage):
    ids = load(stage / 'definition-source-sha256-v1.json')
    for n, h in ids.items():
        need(sha(stage / 'definitions' / n) == h, 'Independent definition changed')
    ns = {'d': 'http://docbook.org/ns/docbook'}
    fields = {}
    for n in ids:
        if not n.endswith('.xml'):
            continue
        for row in ET.parse(stage / 'definitions' / n).findall('.//d:row', ns):
            names = [''.join(e.itertext()).strip() for e in row.findall('./d:entry[1]//d:guilabel', ns)]
            for v in row.findall('.//d:varlistentry', ns):
                if ''.join(v.find('d:term', ns).itertext()).strip() == 'Units':
                    text = ' '.join(''.join(v.find('d:listitem', ns).itertext()).split())
                    for name in names:
                        fields[Path(n).stem, name] = text
    for doc, field in [('CelestialBody', 'NAIFId'), ('SpacecraftOrbitState', 'NAIFId'),
                       ('SpacecraftAttitude', 'NAIFIdReferenceFrame'), ('TrackingFileSet', 'SimTDRSSmarId'),
                       ('TrackingFileSet', 'SimTDRSTrackerType')]:
        need(fields['Resource_' + doc, field] == 'Integer identifier or type code; no physical quantity',
             'Identifier is being treated as a physical quantity')
    need(fields['Resource_QtInputConventions', 'Precision'] == 'Count of decimal digits', 'Precision meaning changed')
    need(fields['Resource_CoordinateSystem', 'Epoch'] == 'GMAT A1 modified Julian days', 'Epoch time scale changed')
    need('[SMA,ECC,INC,RAAN,AOP,MA]' in fields['Resource_SpacecraftNavigation', 'OrbitErrorCovariance'],
         'Covariance definition lost its full state basis')
    native = (stage / 'definitions/src/base/subscriber/MessageWindow.cpp').read_text()
    need('"Precision"' in native and 'Gmat::INTEGER_TYPE' in native, 'Current MessageWindow Precision identity missing')


def audit(stage):
    stage = Path(stage)
    definitions(stage)
    versions = {}
    for v in ['before-v1', 'before-v2', 'after-v1', 'after-v2']:
        ids = load(stage / ('source-sha256-' + v + '.json'))
        build = load(stage / ('build-' + v + '.json'))
        need(build['status'] == 'passed' and build['inputs_unchanged'] and build['source_sha256'] == ids and
             build['log_sha256'] == sha(stage / ('build-' + v + '.log')), 'Terminal build identity changed')
        raw = {}
        with tarfile.open(stage / ('sources-' + v + '.tar.gz')) as archive:
            need(set(archive.getnames()) == set(ids), 'Terminal source archive incomplete')
            for n, h in ids.items():
                data = archive.extractfile(n).read()
                need(hashlib.sha256(data).hexdigest() == h, 'Archived terminal source changed')
                if n in ['src/qtgui/ResourceProperties.cpp', 'src/qtgui/CelestialBodyPanel.cpp',
                         'src/qtgui/tests/CovarianceUnitDisplayTests.cpp', 'src/qtgui/tests/TerminalUnitCases.json']:
                    raw[n] = data
        versions[v] = (ids, build, raw)
    early, before, intermediate, after = [versions[v] for v in ['before-v1', 'before-v2', 'after-v1', 'after-v2']]
    need({n for n, h in early[0].items() if before[0][n] != h} == {'src/qtgui/tests/CovarianceUnitDisplayTests.cpp'},
         'Covariance interaction correction changed other source')
    prod = {'src/qtgui/ResourceProperties.cpp', 'src/qtgui/CelestialBodyPanel.cpp'}
    need({n for n, h in before[0].items() if intermediate[0][n] != h} == prod, 'Label repair changed unrelated source')
    need({n for n, h in intermediate[0].items() if after[0][n] != h} == {'src/qtgui/ResourceProperties.cpp'},
         'Precision type lookup changed unrelated source')
    need(intermediate[2]['src/qtgui/ResourceProperties.cpp'].replace(
         b'object.IsOfType("MessageWindow") && name=="Precision"',
         b'object.GetTypeName()=="MessageWindow" && name=="Precision"') == after[2]['src/qtgui/ResourceProperties.cpp'],
         'Precision repair exceeds exact type lookup')
    props = after[2]['src/qtgui/ResourceProperties.cpp']
    props = props.replace(b'   if (object.GetTypeName()=="MessageWindow" && name=="Precision") return "digits";\n', b'')
    start = props.index(b'         if (field.unit.isEmpty()) {\n            if ((object.IsOfType("Spacecraft")')
    end = props.index(b'         if (field.unit.isEmpty() && object.IsOfType("BatchEstimator")', start)
    need(props[:start] + props[end:] == before[2]['src/qtgui/ResourceProperties.cpp'],
         'Terminal metadata repair changes more than descriptions')
    need(after[2]['src/qtgui/CelestialBodyPanel.cpp'].replace(b'"NAIF ID (identifier)"', b'"NAIF ID"') ==
         before[2]['src/qtgui/CelestialBodyPanel.cpp'], 'Body label repair changed another behavior')
    need(load(stage / 'implementation-plan-v1.json')['production_files'] == {n: before[0][n] for n in prod},
         'Terminal preimage differs')
    for n, h in before[1]['binary_sha256'].items():
        if Path(n).name.startswith('lib'):
            need(after[1]['binary_sha256'][n] == h, 'Label repair changed native libraries')
    fixture = json.loads(after[2]['src/qtgui/tests/TerminalUnitCases.json'])
    need(load(stage / 'frozen-plan-v1.json')['fixture_sha256'] ==
         after[0]['src/qtgui/tests/TerminalUnitCases.json'], 'Frozen unit expectations changed')
    passes = failures = 0
    runs = [('headless', 'before-v1', 'all', {'IdentifierUnits': 8, 'MessagePrecisionUnit': 1, 'CoordinateEpochUnits': 0}, 'failed'),
            ('headless', 'before-v2', 'covariance', {}, 'passed'),
            ('headless', 'after-v1', 'repair', {'IdentifierUnits': 0, 'MessagePrecisionUnit': 1}, None),
            ('headless', 'after-v2', 'precision', {'MessagePrecisionUnit': 0}, None),
            ('visible', 'after-v2', 'all', {'IdentifierUnits': 0, 'MessagePrecisionUnit': 0, 'CoordinateEpochUnits': 0}, 'passed')]
    for mode, v, selection, groups, cov in runs:
        key = 'gui-' + mode + '-' + v + '-' + selection
        root = stage / key
        receipt = load(stage / (key + '-receipt.json'))
        batch = load(root / 'results.json')
        ids, build, _ = versions[v]
        failed = any(groups.values()) or cov == 'failed'
        need(receipt['inputs_unchanged'] and receipt['returncode'] == int(failed) and
             all(receipt['source_sha256'].get(n) == h for n, h in ids.items()), 'Runtime source identity changed')
        for n, h in build['binary_sha256'].items():
            if Path(n).name == 'GmatQt':
                continue
            need(any(value == h and Path(p).parent == Path(n).parent and
                     (p == n or Path(p).name.startswith(Path(n).name + '.'))
                     for p, value in receipt['image_sha256'].items()), 'Runtime image identity changed')
        names = {'QtGui.NativeTerminalUnits' + g for g in groups}
        if cov:
            names.add('QtGui.NativeCovarianceUnitDisplay')
        need({row['name'] for row in batch['results'] if row['status'] != 'not-run'} == names and
             batch['mode'] == mode and batch['end_reason'] == 'completed-selection', 'Terminal selection changed')
        for name in names:
            folder = root / name
            result = load(folder / 'result.json')
            is_cov = name.endswith('CovarianceUnitDisplay')
            bad = cov == 'failed' if is_cov else bool(groups[name.removeprefix('QtGui.NativeTerminalUnits')])
            need(result['status'] == ('failed' if bad else 'passed') and result['returncode'] == (8 if bad else 0)
                 and result['requested_visibility'] == mode, 'Case disposition changed')
            if mode == 'visible':
                need(result['trace']['native_exposure_events'] > 0, 'Terminal GUI was not visibly exposed')
            if is_cov and bad:
                need('FAIL Covariance unit control hidden' in (folder / 'test.log').read_text(), 'Original interaction failure missing')
                continue
            proof = load(folder / 'proof.json')
            need([json.loads(x.split('NASA_CHECK ', 1)[1]) for x in (folder / 'test.log').read_text().splitlines()
                  if 'NASA_CHECK ' in x] == [proof], 'Raw terminal proof changed')
            if is_cov:
                covariance(proof)
                passes += 26
            else:
                group = name.removeprefix('QtGui.NativeTerminalUnits')
                cases = [c for c in fixture['cases'] if c['group'] == group]
                observations(proof, cases, groups[group])
                need(proof['observations'] == load(folder / 'unit-observations.json'), 'Raw observations differ')
                passes += len(proof['observations']) - groups[group]
                failures += groups[group]
    need(passes == 74 and failures == 10 and len(covered()) == 11, 'Terminal totals changed')
    return dict(status='passed',passing_gui_observations=passes,preserved_unit_failure_observations=failures,
                preserved_interaction_failures=1,completed_saved_records=11,remaining_saved_numeric_records=0,
                production_changes='identifier/type-code descriptions and MessageWindow digit count only',
                missions_executed=0,new_requirement_credit=0,whole_row_qualified=False,
                still_required='Join current full input inventory and applicable GUI display evidence before E13 admission.')
