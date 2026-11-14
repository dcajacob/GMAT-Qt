"""Reject missing, stale and overbroad NASA traceability evidence."""
import copy
import csv
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import xml.etree.ElementTree as ET
from zipfile import ZipFile

import NasaGuiCoverage as nasa


class CoverageContracts(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='NASA coverage ')
        self.addCleanup(self.temp.cleanup)
        self.repo = Path(self.temp.name)
        matrix = self.repo / nasa.MATRIX
        matrix.parent.mkdir(parents=True)
        with matrix.open('w', newline='') as stream:
            writer = csv.DictWriter(stream, fieldnames=['Object', 'Operation', 'Requirement(s)', 'Results'])
            writer.writeheader()
            writer.writerow({'Object': 'Spacecraft', 'Operation': 'Apply', 'Requirement(s)': '', 'Results': 'Passed'})
        book = self.repo / nasa.WORKBOOK
        book.parent.mkdir(parents=True)
        ns = nasa.NS['m']
        q = lambda tag: '{' + ns + '}' + tag
        document = ET.Element(q('worksheet'))
        data = ET.SubElement(document, q('sheetData'))
        for row, identifier, text in [(5, 'FRR-1.1', 'Set the value.'), (6, 'FRR-1.1', 'Another distinct requirement.'),
                                      (7, 'DEFERRED', 'Keep deferred source.'), (8, '', 'Keep unnumbered source.')]:
            element = ET.SubElement(data, q('row'), r=str(row))
            for column, value in [('C', 'Properties' if row == 5 else ''), ('D', identifier), ('E', text)]:
                cell = ET.SubElement(element, q('c'), r=f'{column}{row}', t='inlineStr')
                ET.SubElement(ET.SubElement(cell, q('is')), q('t')).text = value
        with ZipFile(book, 'w') as archive:
            archive.writestr('xl/workbook.xml', f'<workbook xmlns="{ns}" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"><sheets><sheet name="FRR" sheetId="1" r:id="rId1"/></sheets></workbook>')
            archive.writestr('xl/_rels/workbook.xml.rels', '<Relationships><Relationship Id="rId1" Target="worksheets/sheet1.xml"/></Relationships>')
            archive.writestr('xl/worksheets/sheet1.xml', ET.tostring(document))
        self.inventory = nasa.inventory(self.repo)
        self.source = 'src/qtgui/tests/Example.cpp'
        path = self.repo / self.source
        path.parent.mkdir(parents=True)
        path.write_text('require(actual == expected); proof("check-1");')
        self.claim = {'test': 'QtGui.Example', 'source': self.source, 'source_sha256': nasa.sha(path),
                      'anchor': 'proof("check-1")', 'checkpoint': 'check-1', 'scope': 'Set the value.',
                      'facets': ['nominal', 'invalid'], 'expected': {'field': 'Value'}}
        item = self.inventory['requirements'][0]
        self.review = {'schema_version': 1, 'sources': copy.deepcopy(self.inventory['sources']), 'reviews': [
            {'key': item['key'], 'fingerprint': item['fingerprint'], 'applicability': 'gui',
             'rationale': 'The widget sets this value.', 'evidence': [self.claim]}]}
        self.manifest = self.repo / nasa.MANIFEST
        self.save_review()
        self.log = self.repo / 'test.log'
        self.log.write_text('1: NASA_CHECK ' + json.dumps({'checkpoint': 'check-1', 'outcome': 'passed',
                                                         'field': 'Value', 'facets': ['nominal', 'invalid']}) + '\n')
        self.receipt = {'source_hashes': {self.source: self.claim['source_sha256']}, 'results': [
            {'name': 'QtGui.Example', 'status': 'passed', 'sources': [self.source], 'log': str(self.log)}]}

    def save_review(self):
        self.manifest.write_text(json.dumps(self.review))

    def status(self, receipt=None):
        return nasa.evaluate(self.repo, self.inventory, self.review, receipt)['requirements'][0]['status']

    def test_applicability_review_is_bound_to_current_source(self):
        row = self.review['reviews'][0]
        row.update(applicability='non-gui', evidence=[],
                   applicability_sources={self.source: nasa.sha(self.repo / self.source)})
        self.save_review()
        self.assertEqual(nasa.load_review(self.repo, self.inventory)['reviews'][0]['applicability'], 'non-gui')
        (self.repo / self.source).write_text('Changed API interface contract')
        with self.assertRaisesRegex(ValueError, 'NASA applicability source changed'):
            nasa.load_review(self.repo, self.inventory)

    def test_non_gui_source_review_keeps_denominator_without_crediting_pass(self):
        self.review['reviews'][0].update(applicability='non-gui', evidence=[],
            applicability_sources={self.source: nasa.sha(self.repo / self.source)})
        report = nasa.evaluate(self.repo, self.inventory, self.review, self.receipt)
        self.assertEqual(len(report['requirements']), len(self.inventory['requirements']))
        self.assertEqual(report['requirements'][0]['status'], 'reviewed-unverified')
        self.assertEqual(report['requirements'][0]['applicability_sources'], {self.source: nasa.sha(self.repo / self.source)})
        self.assertNotIn('verified', report['counts']['requirements'])

    def test_explicit_harness_scope_needs_the_same_exact_passing_proof(self):
        self.review['reviews'][0]['applicability'] = 'harness'
        alternate = self.repo / 'harness-review.json'
        alternate.write_text(json.dumps(self.review))
        reviewed = nasa.load_review(self.repo, self.inventory, manifest='harness-review.json')
        self.assertEqual(reviewed['reviews'][0]['applicability'], 'harness')
        report = nasa.evaluate(self.repo, self.inventory, reviewed, self.receipt)
        self.assertEqual(report['requirements'][0]['status'], 'verified')
        self.receipt['results'][0]['status'] = 'failed'
        self.assertEqual(nasa.evaluate(self.repo, self.inventory, reviewed, self.receipt)
                         ['requirements'][0]['status'], 'mapped-unverified')
        self.receipt['results'][0]['status'] = 'passed'
        self.log.write_text('No structured proof emitted')
        self.assertEqual(nasa.evaluate(self.repo, self.inventory, reviewed, self.receipt)
                         ['requirements'][0]['status'], 'mapped-unverified')

    def test_harness_review_and_metadata_survive_source_preservation(self):
        import RunAcceptance
        extra = ['src/qtgui/tests/NasaHarnessTraceability.json',
                 'src/qtgui/tests/AcceptanceTestMetadata.json']
        for source in extra:
            (self.repo/source).write_text('{}')
        inputs = RunAcceptance.input_sources(self.repo, [], {'mechanisms': []}, (self.inventory, self.review))
        self.assertTrue(set(extra) <= inputs)

    def test_applicability_sources_are_preserved_without_executable_claim(self):
        import RunAcceptance
        self.review['reviews'][0].update(applicability='non-gui', evidence=[],
            applicability_sources={self.source: nasa.sha(self.repo / self.source)})
        inputs = RunAcceptance.input_sources(self.repo, [], {'mechanisms': []}, (self.inventory, self.review))
        self.assertIn(self.source, inputs)

    def test_duplicate_deferred_and_unnumbered_instances_survive(self):
        rows = self.inventory['requirements']
        self.assertEqual(len(rows), 4)
        self.assertEqual(len({x['key'] for x in rows}), 4)
        self.assertTrue(rows[0]['id_ambiguous'])
        self.assertTrue(rows[1]['id_ambiguous'])
        self.assertEqual(rows[2]['declared_id'], 'DEFERRED')
        self.assertEqual(rows[3]['declared_id'], '')
        self.assertEqual(rows[1]['group'], 'Properties')

    def test_old_pass_and_a_review_are_not_execution(self):
        result = nasa.evaluate(self.repo, self.inventory, self.review)
        self.assertEqual(result['historical_cases'][0]['status'], 'unreviewed')
        self.assertEqual(self.status(), 'mapped-unverified')

    def test_complete_proof_is_scoped_to_exact_source_instance(self):
        result = nasa.evaluate(self.repo, self.inventory, self.review, self.receipt)
        self.assertEqual(result['requirements'][0]['status'], 'verified')
        self.assertEqual(result['requirements'][1]['status'], 'unreviewed')

    def test_failed_skipped_or_missing_test_cannot_credit_proof(self):
        for status in ['failed', 'timeout', 'skipped', 'not-run']:
            receipt = copy.deepcopy(self.receipt)
            receipt['results'][0]['status'] = status
            self.assertNotEqual(self.status(receipt), 'verified')
        self.assertNotEqual(self.status({'results': []}), 'verified')

    def test_missing_marker_or_facet_or_wrong_field_is_not_verified(self):
        for proof in [{'checkpoint': 'wrong', 'field': 'Value', 'facets': ['nominal', 'invalid']},
                      {'checkpoint': 'check-1', 'field': 'Value', 'facets': ['nominal']},
                      {'checkpoint': 'check-1', 'field': 'Other', 'facets': ['nominal', 'invalid']}]:
            self.log.write_text('NASA_CHECK ' + json.dumps({'outcome': 'passed', **proof}) + '\n')
            self.assertNotEqual(self.status(self.receipt), 'verified')
        self.log.write_text('NASA_CHECK {truncated\n')
        self.assertNotEqual(self.status(self.receipt), 'verified')
        self.log.write_text('NASA_CHECK {"checkpoint":"check-1","outcome":"passed","facets":null}\n')
        self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_diagnostic_and_conflict_markers_cannot_credit_nasa_conformance(self):
        for outcome, diagnostic in [('passed', True), ('conflict', True), ('conflict', False)]:
            self.log.write_text('NASA_CHECK ' + json.dumps({
                'checkpoint': 'check-1', 'outcome': outcome, 'diagnostic_only': diagnostic,
                'field': 'Value', 'facets': ['nominal', 'invalid']}) + '\n')
            self.assertEqual(nasa.proof_records(self.receipt['results'][0]), [])
            self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_receipt_source_mismatch_does_not_credit_a_passing_log(self):
        self.receipt['source_hashes'][self.source] = 'stale'
        self.assertNotEqual(self.status(self.receipt), 'verified')
        self.receipt['source_hashes'][self.source] = self.claim['source_sha256']
        self.receipt['results'][0]['sources'] = ['another.cpp']
        self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_source_changed_after_review_never_credits_earlier_marker(self):
        (self.repo / self.source).write_text('changed after source review')
        self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_recipe_or_reference_drift_after_review_invalidates_proof(self):
        reference = self.repo / 'recipe.json'
        reference.write_text('{"setting":1}')
        self.review['reference_sources'] = {'recipe.json': nasa.sha(reference)}
        self.receipt['source_hashes']['recipe.json'] = nasa.sha(reference)
        self.assertEqual(self.status(self.receipt), 'verified')
        reference.write_text('{"setting":2}')
        self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_current_recipe_cannot_credit_a_run_with_missing_or_different_recipe(self):
        reference = self.repo / 'recipe.json'
        reference.write_text('{"setting":1}')
        self.review['reference_sources'] = {'recipe.json': nasa.sha(reference)}
        self.assertNotEqual(self.status(self.receipt), 'verified')
        self.receipt['source_hashes']['recipe.json'] = 'different-run-input'
        self.assertNotEqual(self.status(self.receipt), 'verified')
        self.receipt['source_hashes']['recipe.json'] = nasa.sha(reference)
        self.assertEqual(self.status(self.receipt), 'verified')

    def test_source_manifest_and_anchor_drift_fail_closed(self):
        nasa.load_review(self.repo, self.inventory)
        self.review['reviews'][0]['evidence'][0]['anchor'] = 'missing assertion'
        self.save_review()
        with self.assertRaisesRegex(ValueError, 'anchor missing'):
            nasa.load_review(self.repo, self.inventory)
        self.claim['anchor'] = 'proof("check-1")'
        (self.repo / self.source).write_text('changed source')
        self.save_review()
        with self.assertRaisesRegex(ValueError, 'assertion source changed'):
            nasa.load_review(self.repo, self.inventory)

    def test_claim_reference_is_required_only_for_its_own_scope(self):
        reference = self.repo / 'body-document.xml'
        reference.write_text('<body>Mu greater than zero</body>')
        self.claim['reference_sources'] = {'body-document.xml': nasa.sha(reference)}
        other = copy.deepcopy(self.review['reviews'][0])
        other.update(key=self.inventory['requirements'][1]['key'],
                     fingerprint=self.inventory['requirements'][1]['fingerprint'])
        other['evidence'][0].pop('reference_sources')
        self.review['reviews'].append(other)
        result = nasa.evaluate(self.repo, self.inventory, self.review, self.receipt)
        self.assertEqual(result['requirements'][0]['status'], 'mapped-unverified')
        self.assertEqual(result['requirements'][1]['status'], 'verified')
        self.receipt['source_hashes']['body-document.xml'] = nasa.sha(reference)
        self.assertEqual(self.status(self.receipt), 'verified')
        self.receipt['source_hashes']['body-document.xml'] = 'stale'
        self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_claim_document_drift_fails_manifest_validation(self):
        reference = self.repo / 'body-document.xml'
        reference.write_text('<body>original</body>')
        self.claim['reference_sources'] = {'body-document.xml': nasa.sha(reference)}
        self.receipt['source_hashes']['body-document.xml'] = nasa.sha(reference)
        self.save_review()
        nasa.load_review(self.repo, self.inventory)
        reference.write_text('<body>changed</body>')
        self.assertNotEqual(self.status(self.receipt), 'verified')
        with self.assertRaisesRegex(ValueError, 'claim reference source changed'):
            nasa.load_review(self.repo, self.inventory)

    def test_repeated_claims_hash_each_reference_once_per_validation(self):
        reference = self.repo / 'shared-reference.xml'
        reference.write_text('before')
        self.claim['reference_sources'] = {'shared-reference.xml': nasa.sha(reference)}
        self.receipt['source_hashes']['shared-reference.xml'] = nasa.sha(reference)
        other = copy.deepcopy(self.review['reviews'][0])
        other.update(key=self.inventory['requirements'][1]['key'],
                     fingerprint=self.inventory['requirements'][1]['fingerprint'])
        self.review['reviews'].append(other)
        self.save_review()
        with patch.object(nasa, 'sha', wraps=nasa.sha) as hashed:
            nasa.load_review(self.repo, self.inventory)
            self.assertEqual(sum(call.args[0] == reference for call in hashed.call_args_list), 1)
        with patch.object(nasa, 'sha', wraps=nasa.sha) as hashed:
            result = nasa.evaluate(self.repo, self.inventory, self.review, self.receipt)
            self.assertEqual(sum(call.args[0] == reference for call in hashed.call_args_list), 1)
            self.assertEqual([row['status'] for row in result['requirements'][:2]], ['verified', 'verified'])

    def test_reference_is_rehashed_between_calls_even_with_same_size_and_time(self):
        reference = self.repo / 'shared-reference.xml'
        reference.write_text('before')
        self.claim['reference_sources'] = {'shared-reference.xml': nasa.sha(reference)}
        self.receipt['source_hashes']['shared-reference.xml'] = nasa.sha(reference)
        self.save_review()
        nasa.load_review(self.repo, self.inventory)
        self.assertEqual(self.status(self.receipt), 'verified')
        original = reference.stat()
        reference.write_text('after!')
        os.utime(reference, ns=(original.st_atime_ns, original.st_mtime_ns))
        self.assertEqual(reference.stat().st_size, original.st_size)
        self.assertEqual(reference.stat().st_mtime_ns, original.st_mtime_ns)
        self.assertNotEqual(self.status(self.receipt), 'verified')
        with self.assertRaisesRegex(ValueError, 'claim reference source changed'):
            nasa.load_review(self.repo, self.inventory)

    def test_row_fingerprint_and_duplicate_reviews_fail_closed(self):
        self.review['reviews'][0]['fingerprint'] = 'stale'
        self.save_review()
        with self.assertRaisesRegex(ValueError, 'row changed'):
            nasa.load_review(self.repo, self.inventory)
        self.review['reviews'][0]['fingerprint'] = self.inventory['requirements'][0]['fingerprint']
        self.review['reviews'].append(copy.deepcopy(self.review['reviews'][0]))
        self.save_review()
        with self.assertRaisesRegex(ValueError, 'Duplicate'):
            nasa.load_review(self.repo, self.inventory)

    def test_source_workbook_change_requires_manifest_review(self):
        self.review['sources'][nasa.WORKBOOK] = 'changed workbook'
        self.save_review()
        with self.assertRaisesRegex(ValueError, 'inventory changed'):
            nasa.load_review(self.repo, self.inventory)

    def test_non_gui_review_never_becomes_a_gui_pass(self):
        self.review['reviews'][0]['applicability'] = 'non-gui'
        self.assertNotEqual(self.status(self.receipt), 'verified')


    def legacy_fixture(self):
        self.claim.pop('expected')
        self.claim['legacy_assertions'] = {
            'terminal_markers': ['PASS: first checked stage', 'PASS: final checked stage'],
            'assertion_anchors': ['require(actual == expected);']}
        self.log.write_text('7: PASS: first checked stage\n7: PASS: final checked stage\n')
        self.junit = self.repo / 'ctest.xml'
        self.junit.write_text('<testsuite tests="1" failures="0"><testcase name="QtGui.Example" status="run"/></testsuite>')
        result = self.receipt['results'][0]
        result.update(returncode=0, junit=str(self.junit))
        self.rehash_legacy()
        return result

    def rehash_legacy(self):
        result = self.receipt['results'][0]
        for key in ('log', 'junit'):
            result[key + '_sha256'] = nasa.sha(result[key])

    def test_legacy_assertions_reuse_pass_without_synthetic_payload(self):
        self.legacy_fixture()
        self.save_review()
        nasa.load_review(self.repo, self.inventory)
        self.assertEqual(nasa.proof_records(self.receipt['results'][0]), [])
        original = self.log.read_bytes()
        self.assertEqual(self.status(self.receipt), 'verified')
        self.assertEqual(self.log.read_bytes(), original)

    def test_exact_native_renderer_terminal_rejects_changed_quoted_or_duplicate_lines(self):
        self.legacy_fixture()
        marker = ('Native texture, depth, rotation, resize, model material, Sun illumination, '
                  'star catalog/camera/replay/occlusion, fallback and lifecycle checks passed')
        self.claim['legacy_assertions']['terminal_markers'] = [marker]
        self.log.write_text('7: '+marker+'\n')
        self.rehash_legacy(); self.save_review()
        nasa.load_review(self.repo, self.inventory)
        self.assertEqual(self.status(self.receipt), 'verified')
        for text in [marker+' changed\n', 'echo "'+marker+'"\n', marker+'\n'+marker+'\n', '']:
            self.log.write_text(text); self.rehash_legacy()
            self.assertNotEqual(self.status(self.receipt), 'verified')
        self.log.write_text(marker+'\n'); self.rehash_legacy()
        self.claim['legacy_assertions']['terminal_markers'] = [marker+' changed']
        self.save_review()
        with self.assertRaisesRegex(ValueError, 'Legacy evidence needs exact terminal'):
            nasa.load_review(self.repo, self.inventory)

    def test_legacy_claim_without_checkpoint_can_reuse_a_log_with_structured_proofs(self):
        self.legacy_fixture();self.claim.pop('checkpoint')
        structured='7: NASA_CHECK '+json.dumps(dict(checkpoint='another-check',outcome='passed',facets=['nominal']))+'\n'
        self.log.write_text(self.log.read_text()+structured);self.rehash_legacy();self.save_review()
        nasa.load_review(self.repo,self.inventory)
        self.assertEqual(len(nasa.proof_records(self.receipt['results'][0])),1)
        self.assertEqual(self.status(self.receipt),'verified')
        self.log.write_text(structured);self.rehash_legacy()
        self.assertNotEqual(self.status(self.receipt),'verified')

    def test_legacy_missing_duplicate_or_quoted_marker_fails(self):
        self.legacy_fixture()
        for text in ['PASS: final checked stage\n',
                     'PASS: first checked stage\nPASS: final checked stage\nPASS: final checked stage\n',
                     'echo "PASS: first checked stage"\nPASS: final checked stage\n',
                     'PASS: first checked stage\nPASS: final checked stage changed\n']:
            self.log.write_text(text)
            self.rehash_legacy()
            self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_exact_resource_roundtrip_completion_requires_unchanged_terminal(self):
        self.legacy_fixture()
        marker = ('Qt resource round trips: changed-assignment snapshots, removed owned fields, '
                  'dotted continuations, grouped Array resize, exact source/Undo/Redo, preserved '
                  'implicit power epoch and unused body defaults, mixed power/empty shadows, '
                  'invalid correction, implicit mission, grouped deletion, referenced/built-in '
                  'rejection, create/delete/Undo/Redo and Unicode save/reopen matched independent script reports')
        self.claim['legacy_assertions']['terminal_markers'] = [marker]
        self.log.write_text('7: ' + marker + '\n')
        self.rehash_legacy(); self.save_review()
        nasa.load_review(self.repo, self.inventory)
        self.assertEqual(self.status(self.receipt), 'verified')
        for text in [marker + '\n' + marker + '\n', 'echo "' + marker + '"\n',
                     marker + ' changed\n', marker.replace('matched', 'did not match') + '\n']:
            self.log.write_text(text); self.rehash_legacy()
            self.assertNotEqual(self.status(self.receipt), 'verified')
        self.log.write_text(marker + '\n'); self.rehash_legacy()
        self.receipt['results'][0]['status'] = 'failed'
        self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_resource_roundtrip_marker_rejects_other_completion_text(self):
        for marker in ['Qt resource round trips: all passed', 'resource round trips matched reports',
                       'Qt resource round trips: deletion matched independent script reports']:
            self.assertFalse(nasa._valid_terminal_marker(marker))

    def test_exact_inspection_completion_reuses_unchanged_legacy_record(self):
        self.legacy_fixture()
        marker = ('Qt inspections: previews, pending edits, command/mission states, four frames, '
                  'physics selection, skipped branches, script events, search/copy/export, '
                  'stale/failure/stop recovery passed')
        self.claim['legacy_assertions']['terminal_markers'] = [marker]
        self.log.write_text('7: ' + marker + '\n')
        self.rehash_legacy(); self.save_review()
        nasa.load_review(self.repo, self.inventory)
        self.assertEqual(self.status(self.receipt), 'verified')
        for text in [marker + '\n' + marker + '\n', 'echo "' + marker + '"\n',
                     marker.replace('passed', 'failed') + '\n', marker + ' changed\n']:
            self.log.write_text(text); self.rehash_legacy()
            self.assertNotEqual(self.status(self.receipt), 'verified')
        self.log.write_text(marker + '\n'); self.rehash_legacy()
        self.receipt['results'][0]['status'] = 'failed'
        self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_inspection_completion_does_not_accept_arbitrary_passed_text(self):
        for marker in ['Qt inspections: fields passed', 'Other checks passed',
                       'Qt inspections: previews, pending edits passed',
                       'quoted Qt inspections: complete passed']:
            self.assertFalse(nasa._valid_terminal_marker(marker))

    def test_exact_ephemeris_context_requires_both_complete_unique_records(self):
        self.legacy_fixture()
        completion = (': PASS actual MDI Ephemeris/Orbit/Ground/XY ordered Toggle selection, '
                      'empty filtering/Cancel/pending/retained Apply, exact source/Undo/Redo/Unicode Save/Save As/reopen; '
                      'initially disabled ephemeris activation, final Off through 420 s, every ephemeris state analytically circular '
                      'and full script-reference rows/reports equal, metadata/finalization/Output access, '
                      'invalid-subscriber rollback/correction and clean repeat-run; '
                      'viewer two-arc suppression/replay/camera/close-reopen; two enabled output arcs (0-120/240-360).')
        markers = [kind + completion for kind in ['CCSDS-OEM', 'STK-TimePosVel']]
        self.claim['legacy_assertions']['terminal_markers'] = markers
        self.log.write_text('7: ' + markers[0] + '\n7: ' + markers[1] + '\n')
        self.rehash_legacy(); self.save_review()
        nasa.load_review(self.repo, self.inventory)
        self.assertEqual(self.status(self.receipt), 'verified')
        for lines in [[markers[0]], [markers[1]], markers + [markers[0]],
                      [markers[0].replace('0-120/240-360', '0-120/240-420'), markers[1]],
                      ['echo "' + markers[0] + '"', markers[1]],
                      ['SPK' + completion, markers[1]]]:
            self.log.write_text('\n'.join(lines) + '\n'); self.rehash_legacy()
            self.assertNotEqual(self.status(self.receipt), 'verified')
        self.log.write_text('\n'.join(markers) + '\n'); self.rehash_legacy()
        for status in ['failed', 'skipped', 'timeout', 'not-run']:
            self.receipt['results'][0]['status'] = status
            self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_ephemeris_completion_rejects_generic_or_unqualified_formats(self):
        for marker in ['CCSDS-OEM: PASS', 'STK-TimePosVel: PASS actual MDI',
                       'SPK: PASS actual MDI Ephemeris/Orbit/Ground/XY ordered Toggle selection',
                       'Code-500: PASS internal Toggle gap', 'Other: PASS circular states']:
            self.assertFalse(nasa._valid_terminal_marker(marker))

    def test_exact_solver_context_marker_reuses_unchanged_terminal_record(self):
        self.legacy_fixture()
        for context in ['Differential corrector', 'Yukon optimization',
                        'Nested differential correctors', 'Yukon']:
            marker=context+': PASS: checked solver matrix'
            self.claim['legacy_assertions']['terminal_markers']=[marker]
            self.log.write_text('7: '+marker+'\n')
            self.rehash_legacy();self.save_review()
            nasa.load_review(self.repo,self.inventory)
            self.assertEqual(self.status(self.receipt),'verified')
            for text in [marker+'\n'+marker+'\n',
                         'echo "'+marker+'"\n',
                         marker.replace(context,'Other context',1)+'\n',
                         marker.replace('PASS:','FAIL:',1)+'\n']:
                self.log.write_text(text);self.rehash_legacy()
                self.assertNotEqual(self.status(self.receipt),'verified')

    def test_solver_context_marker_does_not_accept_arbitrary_pass_substrings(self):
        for marker in ['FAIL: Yukon: PASS: checked', 'Other context: PASS: checked',
                       'Yukon: PASSED checked', 'quoted Yukon: PASS: checked',
                       'Yukon: FAIL: checked']:
            self.assertFalse(nasa._valid_terminal_marker(marker))

    def test_exact_structured_route_receipt_reuses_source_without_synthetic_proof(self):
        self.legacy_fixture()
        route = dict(kind='existing-playback', outcome='passed',
                     controls=['play', 'stop'], checks=['timer', 'retained-data'])
        marker = 'GUI_ROUTE_CHECK ' + json.dumps(route, separators=(',', ':'))
        self.claim['legacy_assertions']['terminal_markers'] = [marker]
        self.log.write_text('7: ' + marker + '\n')
        self.rehash_legacy()
        self.save_review()
        nasa.load_review(self.repo, self.inventory)
        self.assertEqual(nasa.proof_records(self.receipt['results'][0]), [])
        self.assertEqual(self.status(self.receipt), 'verified')
        for text in ['', marker+'\n'+marker+'\n', marker.replace('retained-data', 'unchecked')+'\n']:
            self.log.write_text(text)
            self.rehash_legacy()
            self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_structured_route_marker_requires_success_kind_controls_and_checks(self):
        good = dict(kind='existing-playback', outcome='passed', controls=['play'], checks=['timer'])
        for record in [{}, [], {**good, 'outcome':'failed'}, {**good, 'kind':''},
                       {**good, 'controls':[]}, {**good, 'checks':[]}, {**good, 'checks':[9]}]:
            self.assertFalse(nasa._valid_terminal_marker('GUI_ROUTE_CHECK '+json.dumps(record)))
        self.assertFalse(nasa._valid_terminal_marker('GUI_ROUTE_CHECK {'))

    def test_legacy_tampered_or_missing_artifact_hash_fails(self):
        result = self.legacy_fixture()
        self.log.write_text(self.log.read_text() + 'changed\n')
        self.assertNotEqual(self.status(self.receipt), 'verified')
        self.rehash_legacy()
        self.junit.write_text(self.junit.read_text() + ' ')
        self.assertNotEqual(self.status(self.receipt), 'verified')
        self.rehash_legacy()
        result.pop('log_sha256')
        self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_legacy_junit_wrong_case_failure_skip_or_duplicate_fails(self):
        self.legacy_fixture()
        for xml in ['<testsuite><testcase name="QtGui.Other"/></testsuite>',
                    '<testsuite><testcase name="QtGui.Example"><failure/></testcase></testsuite>',
                    '<testsuite><testcase name="QtGui.Example" status="fail"/></testsuite>',
                    '<testsuite><testcase name="QtGui.Example"><skipped/></testcase></testsuite>',
                    '<testsuite failures="1"><testcase name="QtGui.Example"/></testsuite>',
                    '<testsuite><testcase name="QtGui.Example"/><testcase name="QtGui.Example"/></testsuite>',
                    '<broken']:
            self.junit.write_text(xml)
            self.rehash_legacy()
            self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_legacy_wrong_result_status_code_name_or_source_fails(self):
        self.legacy_fixture()
        for key, value in [('status', 'failed'), ('status', 'skipped'), ('status', 'not-run'),
                           ('returncode', 1), ('name', 'QtGui.Other'), ('sources', [])]:
            receipt = copy.deepcopy(self.receipt)
            receipt['results'][0][key] = value
            self.assertNotEqual(self.status(receipt), 'verified')
        self.receipt['source_hashes'][self.source] = 'stale'
        self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_legacy_unreviewed_assertion_or_numeric_payload_fails(self):
        self.legacy_fixture()
        original = copy.deepcopy(self.claim['legacy_assertions'])
        for specification in [None, [], {}, {'terminal_markers': [[]], 'assertion_anchors': ['require(actual == expected);']},
                              {'terminal_markers': ['PASS: final checked stage'], 'assertion_anchors': ['require(missing);']}]:
            self.claim['legacy_assertions'] = specification
            self.save_review()
            with self.assertRaisesRegex(ValueError, 'Legacy evidence'):
                nasa.load_review(self.repo, self.inventory)
            self.assertNotEqual(self.status(self.receipt), 'verified')
        self.claim['legacy_assertions'] = original
        self.claim['expected'] = {'actual': 42}
        self.save_review()
        with self.assertRaisesRegex(ValueError, 'Legacy evidence'):
            nasa.load_review(self.repo, self.inventory)
        self.assertNotEqual(self.status(self.receipt), 'verified')

    def test_legacy_cache_does_not_credit_unexecuted_source_or_payload(self):
        self.legacy_fixture()
        other = copy.deepcopy(self.review['reviews'][0])
        other.update(key=self.inventory['requirements'][1]['key'], fingerprint=self.inventory['requirements'][1]['fingerprint'])
        alternative = 'src/qtgui/tests/Unexecuted.cpp'
        (self.repo / alternative).write_bytes((self.repo / self.source).read_bytes())
        self.receipt['source_hashes'][alternative] = self.claim['source_sha256']
        other['evidence'][0]['source'] = alternative
        self.review['reviews'].append(other)
        rows = nasa.evaluate(self.repo, self.inventory, self.review, self.receipt)['requirements']
        self.assertEqual(rows[0]['status'], 'verified')
        self.assertNotEqual(rows[1]['status'], 'verified')
        other['evidence'][0]['source'] = self.source
        other['evidence'][0]['expected'] = {'actual': 42}
        rows = nasa.evaluate(self.repo, self.inventory, self.review, self.receipt)['requirements']
        self.assertNotEqual(rows[1]['status'], 'verified')

    def test_legacy_reference_drift_still_blocks_pass(self):
        self.legacy_fixture()
        reference = self.repo / 'legacy-reference.xml'
        reference.write_text('reviewed expected behavior')
        self.claim['reference_sources'] = {'legacy-reference.xml': nasa.sha(reference)}
        self.assertNotEqual(self.status(self.receipt), 'verified')
        self.receipt['source_hashes']['legacy-reference.xml'] = nasa.sha(reference)
        self.assertEqual(self.status(self.receipt), 'verified')
        reference.write_text('different behavior')
        self.assertNotEqual(self.status(self.receipt), 'verified')


if __name__ == '__main__':
    unittest.main()
