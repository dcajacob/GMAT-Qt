"""Fail-closed proof-recovery contracts; synthetic inputs never earn coverage."""
import copy
import gzip
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

import AcceptanceSnapshot as snapshot
import AuditAcceptanceRecovery as recovery
import BuildAcceptanceCampaign as campaign
from HarnessRunnerTests import HarnessReceiptContracts
from VisualRecoveryTests import VisualSourceContracts
from GraphicsRecoveryTests import GraphicsSourceContracts
from RemainingRecoveryTests import RemainingReferenceContracts


class RecoveryContracts(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='gmat-recovery-contract-')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / 'test.cpp'
        self.source.write_text('require(actual == 2);\n// PASS: synthetic unit contract\n')
        self.reference = self.root / 'reference.txt'
        self.reference.write_text('Unit test expected value: 2')
        self.log = self.root / 'test.log'
        self.log.write_text('1: PASS: synthetic unit contract\n')
        self.junit = self.root / 'test.xml'
        self.junit.write_text('<testsuites><testsuite tests="1" failures="0"><testcase name="QtGui.UnitProof"/></testsuite></testsuites>')
        self.claim = dict(test='QtGui.UnitProof', source='test.cpp',
                          source_sha256=recovery.digest(self.source.read_bytes()),
                          reference_sources={'reference.txt': recovery.digest(self.reference.read_bytes())},
                          anchor='require(actual == 2);', scope='Synthetic scalar assertion',
                          facets=['scalar'], legacy_assertions=dict(
                              terminal_markers=['PASS: synthetic unit contract'],
                              assertion_anchors=['require(actual == 2);']))
        self.case = dict(name='QtGui.UnitProof', command=[str(self.root / 'probe')],
                         sources=['test.cpp'], properties=[])
        self.manifest = dict(root=str(self.root), cases=[self.case], artifact_hashes={
            'test.cpp': self.claim['source_sha256'],
            'reference.txt': self.claim['reference_sources']['reference.txt'],
            'probe': '1' * 64})
        self.manifest['snapshot_id'] = snapshot.fingerprint(self.manifest)
        self.receipt = dict(snapshot_id=self.manifest['snapshot_id'],
                            source_hashes={'test.cpp': self.claim['source_sha256'],
                                           **self.claim['reference_sources']},
                            binary_hashes={str(self.root / 'probe'): '1' * 64})
        self.result = dict(name=self.case['name'], sources=['test.cpp'],
                           snapshot_id=self.manifest['snapshot_id'], status='passed', returncode=0,
                           case_fingerprint=snapshot.case_fingerprint(self.case),
                           registered_command=self.case['command'],
                           log=str(self.log), log_sha256=recovery.digest(self.log.read_bytes()),
                           junit=str(self.junit), junit_sha256=recovery.digest(self.junit.read_bytes()))

    def verify(self):
        return recovery.verify_claim(self.root, self.manifest, self.receipt,
                                     self.result, self.claim)

    def test_exact_complete_retained_contract(self):
        self.assertEqual(self.verify()['test'], 'QtGui.UnitProof')

    def test_failed_skipped_and_nonzero_return_never_credit(self):
        for state in ('failed', 'skipped', 'not-run', 'timeout'):
            with self.subTest(state=state):
                self.result['status'] = state
                with self.assertRaises(ValueError):
                    self.verify()
        self.result.update(status='passed', returncode=1)
        with self.assertRaises(ValueError):
            self.verify()

    def test_source_drift_and_unrecorded_reference_reject(self):
        self.source.write_text('require(actual != 2);')
        with self.assertRaisesRegex(ValueError, 'Source/reference changed'):
            self.verify()
        self.source.write_text('require(actual == 2);\n// PASS: synthetic unit contract\n')
        del self.receipt['source_hashes']['reference.txt']
        with self.assertRaisesRegex(ValueError, 'not recorded'):
            self.verify()

    def test_case_command_and_snapshot_mismatch_reject(self):
        for field, value in [('registered_command', ['different']),
                             ('case_fingerprint', 'different'), ('snapshot_id', 'different')]:
            original = copy.deepcopy(self.result[field])
            self.result[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                self.verify()
            self.result[field] = original

    def test_missing_or_mismatched_executable_identity_reject(self):
        self.receipt['binary_hashes'] = {}
        with self.assertRaisesRegex(ValueError, 'No recorded executable'):
            self.verify()
        self.receipt['binary_hashes'] = {str(self.root / 'probe'): '2' * 64}
        with self.assertRaisesRegex(ValueError, 'executable identity disagrees'):
            self.verify()

    def test_missing_duplicate_and_quoted_terminal_reject(self):
        for text in ('', '1: PASS: synthetic unit contract\n1: PASS: synthetic unit contract\n',
                     '1: "PASS: synthetic unit contract"\n'):
            self.log.write_text(text)
            self.result['log_sha256'] = recovery.digest(self.log.read_bytes())
            with self.subTest(text=text), self.assertRaisesRegex(ValueError, 'Legacy assertions'):
                self.verify()

    def test_junit_empty_wrong_name_skipped_and_failure_reject(self):
        documents = [
            '<testsuites/>',
            '<testsuites><testsuite><testcase name="other"/></testsuite></testsuites>',
            '<testsuites><testsuite skipped="1"><testcase name="QtGui.UnitProof"><skipped/></testcase></testsuite></testsuites>',
            '<testsuites><testsuite><testcase name="QtGui.UnitProof"><failure/></testcase></testsuite></testsuites>',
            '<broken>']
        for text in documents:
            self.junit.write_text(text)
            self.result['junit_sha256'] = recovery.digest(self.junit.read_bytes())
            with self.subTest(text=text), self.assertRaises(ValueError):
                self.verify()

    def test_changed_log_and_junit_bytes_reject_before_parsing(self):
        for target in (self.log, self.junit):
            original = target.read_bytes()
            target.write_bytes(original + b' changed')
            with self.subTest(target=target.name), self.assertRaisesRegex(ValueError, 'Changed test'):
                self.verify()
            target.write_bytes(original)

    def test_lossless_compressed_logs_reuse_without_rewriting_originals(self):
        for target in (self.log, self.junit):
            Path(str(target) + '.gz').write_bytes(gzip.compress(target.read_bytes(), mtime=0))
            target.unlink()
        self.assertEqual(self.verify()['test'], 'QtGui.UnitProof')
        self.assertFalse(self.log.exists())
        self.assertFalse(self.junit.exists())

    def test_structured_proof_needs_exact_nondiagnostic_record(self):
        self.claim.pop('legacy_assertions')
        self.claim.update(checkpoint='value', expected={'value': 2})
        proof = dict(checkpoint='value', outcome='passed', facets=['scalar'], value=2)
        def log(records):
            self.log.write_text(''.join('1: NASA_CHECK ' + json.dumps(r) + '\n' for r in records))
            self.result['log_sha256'] = recovery.digest(self.log.read_bytes())
        log([proof])
        self.assertEqual(self.verify()['test'], 'QtGui.UnitProof')
        for records in ([{**proof, 'value': 3}], [{**proof, 'diagnostic_only': True}],
                        [{**proof, 'outcome': 'conflict'}], [proof, proof]):
            log(records)
            with self.subTest(records=records), self.assertRaises(ValueError):
                self.verify()

    def test_manifest_json_identity_and_compressed_identity(self):
        path = self.root / 'metadata.json'
        path.write_text('{"unit":2}')
        expected = recovery.digest(path.read_bytes())
        self.assertEqual(recovery.checked_json(path, expected), {'unit': 2})
        Path(str(path) + '.gz').write_bytes(gzip.compress(path.read_bytes(), mtime=0))
        path.unlink()
        self.assertEqual(recovery.checked_json(path, expected), {'unit': 2})
        with self.assertRaises(ValueError):
            recovery.checked_json(path, '0' * 64)

    def test_omitted_command_input_binding_cannot_rewrite_receipt(self):
        self.case['command'].append(str(self.reference))
        self.result['registered_command'] = list(self.case['command'])
        original = {**self.receipt, 'results': [self.result]}
        original['source_hashes'] = {'test.cpp': self.claim['source_sha256']}
        bindings = {'reference.txt': {'sha256': self.claim['reference_sources']['reference.txt'],
                                      'test': 'QtGui.UnitProof', 'provenance': 'unit fixture'}}
        wrapper = copy.deepcopy(original)
        wrapper.update(original_receipt='unit.json', original_receipt_sha256='2' * 64,
                       review_only_snapshot_input_bindings=bindings)
        wrapper['source_hashes']['reference.txt'] = bindings['reference.txt']['sha256']
        self.assertEqual(recovery.supplemented_receipt(original, wrapper, bindings,
                         self.root, self.manifest)['results'], original['results'])
        # A review can attest an input already recorded at the identical hash.
        # It adds no execution fact and must not change that recorded hash.
        repeated = copy.deepcopy(original)
        repeated['source_hashes']['reference.txt'] = bindings['reference.txt']['sha256']
        self.assertEqual(recovery.supplemented_receipt(repeated, wrapper, bindings,
                         self.root, self.manifest)['source_hashes'], repeated['source_hashes'])
        repeated['source_hashes']['reference.txt'] = '3' * 64
        with self.assertRaisesRegex(ValueError, 'already recorded'):
            recovery.supplemented_receipt(repeated, wrapper, bindings, self.root, self.manifest)
        changed = copy.deepcopy(wrapper)
        changed['results'][0]['status'] = 'failed'
        with self.assertRaisesRegex(ValueError, 'rewrites execution'):
            recovery.supplemented_receipt(original, changed, bindings, self.root, self.manifest)
        self.result['registered_command'] = [str(self.root / 'probe')]
        wrapper['results'] = copy.deepcopy(original['results'])
        with self.assertRaisesRegex(ValueError, 'executed command input'):
            recovery.supplemented_receipt(original, wrapper, bindings, self.root, self.manifest)

    def test_union_excludes_pending_duplicate_and_partial_rows(self):
        decisions = [dict(key='A', decision='accept_versioned_scope'),
                     dict(key='B', decision='missing_obligation')]
        plan = dict(version='unit', required_cohorts=['fast', 'visible'], decisions=decisions)
        report = dict(version='unit', decisions=copy.deepcopy(decisions),
                      accepted_source_versioned_scope_keys={'requirements': ['A'], 'historical_cases': []},
                      cohorts=[dict(cohort=c, rows=[dict(key=k, checks=[{'unit': True}]) for k in ['A', 'B']])
                               for c in ['fast', 'visible']],
                      new_test_executions=0, full_current_build_qualified=False)
        campaign.validate_recovery_report(report, plan)
        mutations = []
        changed = copy.deepcopy(report)
        changed['accepted_source_versioned_scope_keys']['requirements'].append('B')
        mutations.append(changed)
        changed = copy.deepcopy(report)
        changed['accepted_source_versioned_scope_keys']['requirements'].append('A')
        mutations.append(changed)
        changed = copy.deepcopy(report)
        changed['cohorts'][0]['rows'].pop()
        mutations.append(changed)
        changed = copy.deepcopy(report)
        changed['cohorts'][0]['rows'][0]['checks'] = []
        mutations.append(changed)
        changed = copy.deepcopy(report)
        changed['cohorts'].pop()
        mutations.append(changed)
        changed = copy.deepcopy(report)
        changed['new_test_executions'] = 2
        mutations.append(changed)
        changed = copy.deepcopy(report)
        changed['cohorts'][0]['rows'][0]['checks'] = [dict(verified=False,rejection='wrong source')]
        mutations.append(changed)
        for changed in mutations:
            with self.subTest(changed=changed), self.assertRaises(ValueError):
                campaign.validate_recovery_report(changed, plan)

    def composed_fixture(self):
        mapping=dict(sources={},reviews=[dict(key='A',fingerprint='literal-fingerprint',evidence=[self.claim])])
        mapping_path=self.root/'mapping.json';mapping_path.write_text(json.dumps(mapping))
        manifest_path=self.root/'manifest.json';manifest_path.write_text(json.dumps(self.manifest))
        old=dict(source_bound_evidence={})
        self.result['trace']={'native_exposure_events':1}
        for name,mode in [('old-visible','visible'),('old-fast','headless')]:
            receipt=dict(self.receipt,results=[self.result],mode=mode,snapshot_unchanged=True,
                         finished_utc='2026-01-01T00:00:00Z',snapshot_manifest=str(manifest_path))
            path=self.root/(name+'.json');path.write_text(json.dumps(receipt))
            old['source_bound_evidence'][name]=dict(receipt=str(path),receipt_sha256=recovery.digest(path.read_bytes()))
        version=dict(mapping_manifest=str(mapping_path),mapping_manifest_sha256=recovery.digest(mapping_path.read_bytes()))
        plan=dict(required_cohorts=['visible','unattended'],decisions=[dict(key='A',rationale='Unit scalar literal2',remaining_limits='Synthetic only')],
                  claim_bindings={'visible':{'QtGui.UnitProof':'old-visible'},'unattended':{'QtGui.UnitProof':'old-fast'}})
        return plan,version,old,{'sources':{}},{'A':{'fingerprint':'literal-fingerprint'}}

    def composed(self, fixture):
        with mock.patch.object(recovery.nasa,'inventory',return_value={'sources':{}}):
            return recovery.composed_observations(self.root,*fixture)

    def test_explicit_composed_original_execution_preserves_all_claims_and_modes(self):
        observations=self.composed(self.composed_fixture())
        self.assertEqual({c['cohort'] for c in observations},{'visible','unattended'})
        self.assertTrue(all(c['rows'][0]['checks'][0]['test']=='QtGui.UnitProof' for c in observations))
        self.assertEqual(observations[0]['rows'][0]['checks'][0]['original_cohort'],'old-visible')

    def test_composed_cannot_omit_a_claim_add_a_test_or_use_unbound_receipts(self):
        for mutation in ('omit','extra','outside'):
            fixture=self.composed_fixture();bindings=fixture[0]['claim_bindings']['visible']
            if mutation=='omit':bindings.clear()
            elif mutation=='extra':bindings['unrelated']='old-visible'
            else:bindings['QtGui.UnitProof']='outside'
            with self.subTest(mutation=mutation),self.assertRaises(ValueError):self.composed(fixture)

    def test_composed_wrong_mode_and_source_hash_still_reject(self):
        fixture=self.composed_fixture();fixture[0]['claim_bindings']['visible']['QtGui.UnitProof']='old-fast'
        with self.assertRaisesRegex(ValueError,'Execution mode'):self.composed(fixture)
        fixture=self.composed_fixture();self.source.write_text('changed')
        with self.assertRaisesRegex(ValueError,'Source/reference changed'):self.composed(fixture)

    def test_composed_visible_needs_exposure_and_all_original_legacy_assertions(self):
        fixture=self.composed_fixture()
        old=fixture[2]['source_bound_evidence']['old-visible'];path=Path(old['receipt']);receipt=json.loads(path.read_text())
        receipt['results'][0]['trace']['native_exposure_events']=0;path.write_text(json.dumps(receipt));old['receipt_sha256']=recovery.digest(path.read_bytes())
        with self.assertRaisesRegex(ValueError,'native exposure'):self.composed(fixture)
        fixture=self.composed_fixture();self.log.write_text('no terminal')
        with self.assertRaisesRegex(ValueError,'Changed test log'):self.composed(fixture)

    def test_composed_mapping_without_authorities_needs_optin_and_exact_frozen_inventory(self):
        fixture=self.composed_fixture();path=Path(fixture[1]['mapping_manifest']);mapping=json.loads(path.read_text())
        del mapping['sources'];path.write_text(json.dumps(mapping));fixture[1]['mapping_manifest_sha256']=recovery.digest(path.read_bytes())
        with self.assertRaisesRegex(ValueError,'explicit frozen-inventory'):self.composed(fixture)
        fixture[0]['mapping_authority_from_frozen_inventories']=True
        self.assertEqual(len(self.composed(fixture)),2)
        with mock.patch.object(recovery.nasa,'inventory',return_value={'sources':{'changed':'hash'}}):
            with self.assertRaisesRegex(ValueError,'NASA source baseline differs'):
                recovery.composed_observations(self.root,*fixture)

    def test_composed_original_raw_receipt_alias_needs_original_audit_hash(self):
        fixture=self.composed_fixture();old=fixture[2]
        visible=old['source_bound_evidence'].pop('old-visible');path=self.root/'visible'/'results.json';path.parent.mkdir();path.write_bytes(Path(visible['receipt']).read_bytes())
        fixture[0]['raw_receipts']={'raw:visible':str(path)}
        fixture[0]['claim_bindings']['visible']['QtGui.UnitProof']='raw:visible'
        with self.assertRaisesRegex(ValueError,'not hashed by the original'):self.composed(fixture)
        old['raw_artifact_sha256']={str(path):recovery.digest(path.read_bytes())}
        self.assertEqual(len(self.composed(fixture)),2)
        old['raw_artifact_sha256'][str(path)]='0'*64
        with self.assertRaisesRegex(ValueError,'Retained identity mismatch'):self.composed(fixture)

    def test_composed_same_test_multiple_claims_are_all_required(self):
        fixture=self.composed_fixture();path=Path(fixture[1]['mapping_manifest']);mapping=json.loads(path.read_text())
        extra=copy.deepcopy(self.claim);extra['anchor']='require(unexecuted == 3);'
        mapping['reviews'][0]['evidence'].append(extra);path.write_text(json.dumps(mapping));fixture[1]['mapping_manifest_sha256']=recovery.digest(path.read_bytes())
        with self.assertRaisesRegex(ValueError,'Missing source assertion'):self.composed(fixture)

    def test_composed_original_index_requires_identity_and_exact_row_claim(self):
        fixture=self.composed_fixture();old=fixture[2];path=self.root/'index.json'
        entry=dict(key='A',claim=self.claim,receipt=old['source_bound_evidence']['old-visible'])
        path.write_text(json.dumps([entry]));old.update(combined_claim_evidence=str(path),combined_claim_evidence_sha256=recovery.digest(path.read_bytes()))
        fixture[0]['receipt_index']='combined_claim_evidence';fixture[0]['claim_bindings']['visible']['QtGui.UnitProof']='index:0'
        self.assertEqual(len(self.composed(fixture)),2)
        entry['key']='B';path.write_text(json.dumps([entry]))
        with self.assertRaisesRegex(ValueError,'Retained identity mismatch'):self.composed(fixture)
        old['combined_claim_evidence_sha256']=recovery.digest(path.read_bytes())
        with self.assertRaisesRegex(ValueError,'complete original row claim'):self.composed(fixture)

    def reference_composition_fixture(self):
        fixture=list(self.composed_fixture())
        plan,version,old,baseline,candidates=fixture
        version['label']='newer'
        path=Path(version['mapping_manifest']);base=json.loads(path.read_text())
        original=copy.deepcopy(base)
        base['reviews'][0]['evidence'][0]['reference_sources']['reference.txt']='0'*64
        path.write_text(json.dumps(base));version['mapping_manifest_sha256']=recovery.digest(path.read_bytes())
        origin_path=self.root/'original-mapping.json';origin_path.write_text(json.dumps(original))
        audit_path=self.root/'original-audit.json';audit_path.write_text(json.dumps(old))
        report=self.root/'original-report.json';report.write_text('{}')
        receipt=old['source_bound_evidence']['old-visible']
        origin=dict(label='older',mapping_manifest=str(origin_path),mapping_manifest_sha256=recovery.digest(origin_path.read_bytes()),
                    audit=str(audit_path),audit_sha256=recovery.digest(audit_path.read_bytes()),report=str(report),
                    report_sha256=recovery.digest(report.read_bytes()),**receipt)
        reviewed=self.root/'reviewed-composition.json';reviewed.write_text(json.dumps(original))
        plan['reviewed_composition']=dict(mapping=str(reviewed),mapping_sha256=recovery.digest(reviewed.read_bytes()),
                                           origins={'A':['older']},collections=['older'])
        for mode,name in [('visible','old-visible'),('unattended','old-fast')]:
            plan['claim_bindings'][mode]['QtGui.UnitProof']='version:older:'+name
        return fixture+[{'versions':[version,origin]}]

    def test_reviewed_composition_restores_actual_old_references_with_all_original_assertions(self):
        fixture=self.reference_composition_fixture();observed=self.composed(fixture)
        self.assertEqual(len(observed),2)
        self.assertTrue(all('version:older:' in c['rows'][0]['checks'][0]['original_cohort'] for c in observed))
        self.assertEqual(observed[0]['mapping'],fixture[0]['reviewed_composition']['mapping'])

    def test_reviewed_composition_rejects_omitted_or_rewritten_original_clauses(self):
        for change in ('missing-clause','changed-source','changed-scope','dropped-reference'):
            fixture=self.reference_composition_fixture();spec=fixture[0]['reviewed_composition'];path=Path(spec['mapping']);mapping=json.loads(path.read_text())
            if change=='missing-clause':mapping['reviews'][0]['evidence'].clear()
            elif change=='changed-source':mapping['reviews'][0]['evidence'][0]['source_sha256']='0'*64
            elif change=='changed-scope':mapping['reviews'][0]['evidence'][0]['scope']='Narrower scalar scope'
            else:mapping['reviews'][0]['evidence'][0]['reference_sources'].clear()
            path.write_text(json.dumps(mapping));spec['mapping_sha256']=recovery.digest(path.read_bytes())
            with self.subTest(change=change),self.assertRaises(ValueError):self.composed(fixture)

    def test_reviewed_composition_rejects_unbound_collection_or_wrong_receipt_mode(self):
        fixture=self.reference_composition_fixture();fixture[0]['reviewed_composition']['collections']=[]
        with self.assertRaisesRegex(ValueError,'collection list'):self.composed(fixture)
        fixture=self.reference_composition_fixture();fixture[0]['claim_bindings']['visible']['QtGui.UnitProof']='version:older:old-fast'
        with self.assertRaisesRegex(ValueError,'Execution mode'):self.composed(fixture)

    def test_composed_missing_obligation_retains_rejection_without_passing_proof(self):
        fixture=self.composed_fixture();fixture[0]['decisions'][0]['decision']='missing_obligation'
        self.source.write_text('changed source')
        observations=self.composed(fixture)
        for group in observations:
            check=group['rows'][0]['checks'][0]
            self.assertIs(check['verified'],False)
            self.assertIn('Source/reference changed',check['rejection'])
        fixture[0]['decisions'][0]['decision']='accept_versioned_scope'
        with self.assertRaisesRegex(ValueError,'Source/reference changed'):self.composed(fixture)


class RecoveryCollectionContracts(unittest.TestCase):
    setUp=RecoveryContracts.setUp
    composed_fixture=RecoveryContracts.composed_fixture
    composed=RecoveryContracts.composed

    def collection_fixture(self):
        fixture=list(self.composed_fixture());plan,version,old,baseline,candidates=fixture
        report=self.root/'collection-report.json';report.write_text('{}')
        original_audit=self.root/'collection-audit.json';original_audit.write_text(json.dumps(old))
        original=dict(label='additional',audit=str(original_audit),audit_sha256=recovery.digest(original_audit.read_bytes()),
                      report=str(report),report_sha256=recovery.digest(report.read_bytes()),
                      receipt=old['source_bound_evidence']['old-visible']['receipt'],
                      receipt_sha256=old['source_bound_evidence']['old-visible']['receipt_sha256'],
                      mapping_manifest=version['mapping_manifest'],mapping_manifest_sha256=version['mapping_manifest_sha256'])
        plan['receipt_collections']=['additional']
        for mode,name in [('visible','old-visible'),('unattended','old-fast')]:
            plan['claim_bindings'][mode]['QtGui.UnitProof']='collection:additional:'+name
        old['source_bound_evidence']={}
        return fixture+[{'versions':[original]}]

    def test_complete_claim_reuses_hash_bound_other_collection(self):
        fixture=self.collection_fixture();before=copy.deepcopy(self.claim)
        observed=self.composed(fixture)
        self.assertEqual(self.claim,before)
        self.assertEqual({g['cohort'] for g in observed},{'visible','unattended'})
        self.assertTrue(all(g['rows'][0]['checks'][0]['original_cohort'].startswith('collection:additional:') for g in observed))

    def test_unknown_duplicate_and_absent_ledger_reject(self):
        for labels,ledger in [(['unknown'],True),(['additional','additional'],True),(['additional'],False)]:
            fixture=self.collection_fixture();fixture[0]['receipt_collections']=labels
            if not ledger:fixture.pop()
            with self.subTest(labels=labels,ledger=ledger),self.assertRaises(ValueError):self.composed(fixture)

    def test_changed_original_collection_or_receipt_rejects(self):
        for field in ['audit','report','receipt','mapping_manifest']:
            fixture=self.collection_fixture();v=fixture[-1]['versions'][0]
            v[field+'_sha256']='0'*64
            with self.subTest(field=field),self.assertRaisesRegex(ValueError,'Retained identity mismatch'):self.composed(fixture)

    def test_cross_collection_cannot_bypass_assertions_or_mode(self):
        fixture=self.collection_fixture();fixture[0]['claim_bindings']['visible']['QtGui.UnitProof']='collection:additional:old-fast'
        with self.assertRaisesRegex(ValueError,'Execution mode'):self.composed(fixture)
        fixture=self.collection_fixture();self.source.write_text('changed assertion')
        with self.assertRaisesRegex(ValueError,'Source/reference changed'):self.composed(fixture)


if __name__ == '__main__':
    unittest.main()
