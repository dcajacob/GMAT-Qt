"""Admission guards for documentation credit; synthetic fixtures earn no NASA credit."""
import copy
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

import BuildAcceptanceCampaign as campaign
from AuditAllInputDocumentation import KEY, FINGERPRINT, TEXT


class DocumentationAdmissionContracts(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='gmat-documentation-admission-')
        self.addCleanup(self.temp.cleanup)
        self.repo = Path(self.temp.name)
        self.verifier = self.repo/'src/qtgui/tests/AuditAllInputDocumentation.py'
        self.verifier.parent.mkdir(parents=True)
        self.verifier.write_text('# synthetic verifier fixture\n')
        self.report = dict(contract_kind='input-documentation', qualified_keys=[KEY],
                           source_inputs_unchanged=True, full_current_build_qualified=False,
                           full_NASA_qualified=False, technical_core_approval=False,
                           status='passed', key=KEY, fingerprint=FINGERPRINT, requirement_text=TEXT,
                           all_input_documentation_qualified=True,
                           checks={key:True for key in campaign.DOCUMENTATION_CHECKS},
                           new_GUI_acceptance_replays=0, new_mission_executions=0)
        self.report_path = self.repo/'report.json'
        self.report_path.write_text(json.dumps(self.report))
        self.q = {key:self.report[key] for key in ('contract_kind','qualified_keys','source_inputs_unchanged',
                                                  'full_current_build_qualified','full_NASA_qualified','technical_core_approval')}
        self.q.update(report=str(self.report_path), report_sha256=campaign.sha(self.report_path),
                      verifier_sha256=campaign.sha(self.verifier), evidence_root=str(self.repo))
        self.path = self.repo/'qualification.json'
        self.path.write_text(json.dumps(self.q))

    def test_missing_false_or_unreviewed_conjunct_rejects(self):
        for key in campaign.DOCUMENTATION_CHECKS:
            for replacement in (False, None, 'true'):
                row = copy.deepcopy(self.report)
                row['checks'][key] = replacement
                with self.subTest(key=key, replacement=replacement), self.assertRaises(ValueError):
                    campaign.validate_documentation_contract(self.q, row)
            row = copy.deepcopy(self.report)
            del row['checks'][key]
            with self.assertRaises(ValueError):
                campaign.validate_documentation_contract(self.q, row)
        row = copy.deepcopy(self.report)
        row['checks']['new_unreviewed_claim'] = True
        with self.assertRaises(ValueError):
            campaign.validate_documentation_contract(self.q, row)

    def test_wrong_authority_subset_and_extra_credit_reject(self):
        for field, value in [('key','All Others!E14'), ('fingerprint','0'*64),
                             ('requirement_text','All selected fields have units.'),
                             ('qualified_keys',[]), ('qualified_keys',[KEY,'All Others!E14'])]:
            row = {**self.report, field:value}
            with self.subTest(field=field), self.assertRaises(ValueError):
                campaign.validate_documentation_contract(self.q, row)
        with self.assertRaises(ValueError):
            campaign.validate_documentation_contract({**self.q,'qualified_keys':[]}, self.report)

    def test_historical_documentary_proof_cannot_claim_full_or_runtime_qualification(self):
        for field, value in [('source_inputs_unchanged',False), ('full_current_build_qualified',True),
                             ('full_NASA_qualified',True), ('technical_core_approval',True),
                             ('contract_kind','native-GUI')]:
            for target in ('qualification','report'):
                q, r = copy.deepcopy(self.q), copy.deepcopy(self.report)
                (q if target=='qualification' else r)[field] = value
                with self.subTest(field=field,target=target), self.assertRaises(ValueError):
                    campaign.validate_documentation_contract(q,r)
        for field, value in [('status','pending'), ('all_input_documentation_qualified',False),
                             ('new_GUI_acceptance_replays',1), ('new_mission_executions',1)]:
            with self.subTest(field=field), self.assertRaises(ValueError):
                campaign.validate_documentation_contract(self.q,{**self.report,field:value})

    def test_changed_report_or_verifier_rejected_before_reaudit(self):
        for path in (self.report_path,self.verifier):
            old = path.read_bytes()
            path.write_bytes(old+b' changed')
            with mock.patch('AuditAllInputDocumentation.audit') as auditor, self.assertRaises(ValueError):
                campaign.documentation_contract_report(self.repo,self.path)
            auditor.assert_not_called()
            path.write_bytes(old)

    def test_changed_live_evidence_rejected_without_gui_replay(self):
        with mock.patch('AuditAllInputDocumentation.audit',return_value={**self.report,'source_sha256':{'changed':'x'}}) as auditor:
            with self.assertRaisesRegex(ValueError,'no longer match'):
                campaign.documentation_contract_report(self.repo,self.path)
            auditor.assert_called_once_with(self.repo,self.repo)

    def test_exact_reaudit_can_admit_only_documentary_key(self):
        with mock.patch('AuditAllInputDocumentation.audit',return_value=self.report):
            q, keys = campaign.documentation_contract_report(self.repo,self.path)
        self.assertEqual(keys,[KEY])
        self.assertFalse(q['full_current_build_qualified'])

    def test_supersession_preserves_sealed_gap_and_rejects_duplicate(self):
        gap = dict(key=KEY,decision='missing_obligation',inventory='sealed.json',inventory_sha256='a'*64,
                   rationale='Original exploratory census was incomplete.')
        findings, accepted = {KEY:copy.deepcopy(gap)}, {}
        campaign.add_documentation_contract(findings,accepted,self.path,self.q,[KEY])
        self.assertEqual(findings[KEY]['superseded_documentation_gap'],gap)
        self.assertEqual(findings[KEY]['decision'],'verified_documentation_contract')
        with self.assertRaises(ValueError):
            campaign.add_documentation_contract(findings,accepted,self.path,self.q,[KEY])

    def test_unrelated_or_missing_gap_cannot_be_superseded(self):
        cases = [({}, {}, [KEY]),
                 ({KEY:dict(decision='pending')}, {}, [KEY]),
                 ({'FRR!E294':dict(decision='missing_obligation',inventory='x',inventory_sha256='x')}, {}, ['FRR!E294'])]
        for findings, accepted, keys in cases:
            with self.subTest(keys=keys), self.assertRaises(ValueError):
                campaign.add_documentation_contract(findings,accepted,self.path,self.q,keys)


if __name__ == '__main__':
    unittest.main()
