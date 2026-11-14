"""Independent mixed-outcome/metadata/CLI contracts for the reusable catalogue."""
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import AcceptanceCatalog as catalog


def fixture():
    receipt = dict(started_utc='2026-10-31T12:00:00Z', finished_utc='2026-10-31T12:01:00Z',
                   mode='fixture-recorded-run', snapshot_id='unit-source-version',
                   platform={'id': 'Linux-x86_64'}, results=[
        dict(name='Good', status='passed', command=['unit-probe'], returncode=0),
        dict(name='Bad', status='failed', command=['unit-probe'], returncode=1),
        dict(name='Slow', status='timeout', command=['unit-probe'], returncode=-9,
             platform='Windows-x86_64'),
        dict(name='Reused', status='passed', returncode=0, reused_from='retained/results.json'),
        dict(name='Skipped', status='skipped', reason='disabled registration'),
        dict(name='Unrun', status='not-run'),
        dict(name='Stale', status='passed', command=['unit-probe'], returncode=0),
        dict(name='Interrupted', status='interrupted', command=['unit-probe'], returncode=-2)])
    def scope(key, status, test=None, verified=False, declared_id='DUPLICATE-ID'):
        return dict(key=key, declared_id=declared_id, text='Requirement ' + key,
                    fingerprint='unit-' + key, status=status,
                    evidence=[dict(test=test, verified_in_run=verified, result='passed')] if test else [])
    report = dict(requirements=[scope('R1','verified','Good',True),
        scope('R2','mapped-unverified','Bad'), scope('R3','mapped-unverified','Slow'),
        scope('R4','reviewed-unverified'), scope('R5','mapped-unverified','Unrun'),
        scope('R6','mapped-unverified','Stale'), scope('R7','mapped-unverified','Skipped'),
        scope('R8','mapped-unverified','Interrupted'), scope('R9','verified','Reused',True),
        scope('R10','reviewed-unverified')],
        historical_cases=[scope('H1','mapped-unverified','Bad')])
    metadata = dict(schema_version=1, tests={
        'Good': dict(earliest_build_date='2026-01-02', latest_build_date='2026-12-30',
                     requirements=['R1'], author='unit author Δ',
                     truth_data=[dict(kind='literal', value=[2, 7], units=['s','km']),
                                 dict(kind='file', path='truth α.txt', sha256='a'*64)],
                     tolerances={'absolute': 1e-8, 'relative': 2e-10,
                                 'fields': {'clock': {'absolute': 1e-6}}},
                     comparator='position-velocity', extensions={'owner': {'reviewed': False, 'issue': 17}},
                     comments='First line\nSecond line α', creation_date='2026-01-01',
                     platform_dependencies=['Linux-x86_64'], test_type='Acceptance'),
        'Bad': dict(test_type='System', requirements=['R2']),
        'Slow': dict(test_type='Validation', requirements=['R3']),
        'Planned': dict(test_type='Validation', requirements=['R10'])})
    return receipt, report, metadata


class CatalogContracts(unittest.TestCase):
    def setUp(self):
        self.receipt, self.report, self.metadata = fixture()

    def make(self, **options):
        return catalog.catalogue(self.receipt, self.report, self.metadata, **options)

    def test_full_metadata_exact_roundtrip_preserves_truth_tolerances_and_extensions(self):
        serialized = json.dumps(self.metadata, ensure_ascii=False, allow_nan=False)
        loaded = catalog.validate_metadata(json.loads(serialized))
        self.assertEqual(loaded, self.metadata)
        record = next(t['metadata'] for t in self.make()['tests'] if t['name']=='Good')
        self.assertEqual(record, self.metadata['tests']['Good'])
        self.assertEqual(record['truth_data'][0]['value'], [2,7])
        self.assertEqual(record['tolerances']['absolute'], 1e-8)
        self.assertEqual(record['extensions'], {'owner': {'reviewed': False, 'issue': 17}})

    def test_inclusive_build_date_limits_and_exact_platform_dependencies(self):
        record = self.metadata['tests']['Good']
        expected = [('2026-01-01','ineligible'),('2026-01-02','eligible'),
                    ('2026-12-30','eligible'),('2026-12-31','ineligible')]
        for build, state in expected:
            self.assertEqual(catalog.metadata_eligibility(record, build, 'Linux-x86_64'),
                             {'build':state,'platform':'eligible'})
        self.assertEqual(catalog.metadata_eligibility(record,'2026-02-01','Windows-x86_64')['platform'], 'ineligible')
        self.assertEqual(catalog.metadata_eligibility({},'2026-02-01','Linux-x86_64'),
                         {'build':'unknown','platform':'unknown'})

    def test_invalid_dates_and_reversed_build_interval_reject(self):
        for value in ['2026-02-29','20260102','2026-1-2',False]:
            m=copy.deepcopy(self.metadata);m['tests']['Good']['creation_date']=value
            with self.subTest(value=value),self.assertRaises((ValueError,TypeError)):
                catalog.validate_metadata(m)
        m=copy.deepcopy(self.metadata);m['tests']['Good']['latest_build_date']='2025-01-01'
        with self.assertRaisesRegex(ValueError,'Earliest'):
            catalog.validate_metadata(m)

    def test_invalid_tolerance_nan_truth_type_and_duplicate_links_reject(self):
        mutations=[('tolerances',{'absolute':-1}),('tolerances',{'relative':True}),
                   ('tolerances',{'absolute':float('nan')}),('tolerances',{'fields':{'x':None}}),
                   ('truth_data',[{'kind':'file','path':'truth','sha256':'bad'}]),
                   ('truth_data',[{'kind':'literal'}]),('extensions',{'bad':float('inf')}),
                   ('test_type','GUI'),('requirements',['R1','R1']),('platform_dependencies',[''])]
        for field,value in mutations:
            m=copy.deepcopy(self.metadata);m['tests']['Good'][field]=value
            with self.subTest(field=field,value=value),self.assertRaises(ValueError):
                catalog.validate_metadata(m)
        m=copy.deepcopy(self.metadata);m['tests']['Good']['comparitor']='typo'
        with self.assertRaisesRegex(ValueError,'Unknown metadata'):
            catalog.validate_metadata(m)

    def test_exact_failed_type_platform_groups_do_not_confuse_qt_backend(self):
        self.receipt['trace']={'recorded_platforms':['xcb']}
        self.assertEqual(self.make()['failed_by_type_platform'],[
            {'test_type':'System','platform':'Linux-x86_64','tests':['Bad']},
            {'test_type':'Validation','platform':'Windows-x86_64','tests':['Slow']}])
        self.receipt.pop('platform')
        result=self.make()
        self.assertEqual(next(t['platform'] for t in result['tests'] if t['name']=='Bad'),'unknown')
        self.assertNotIn('xcb',{t['platform'] for t in result['tests']})

    def test_requirement_set_query_uses_source_keys_and_keeps_duplicate_declared_ids(self):
        result=self.make()
        self.assertEqual([t['name'] for t in catalog.query_requirements(result,['R1','R2'])],['Bad','Good'])
        self.assertEqual([t['name'] for t in catalog.query_requirements(result,['R10'])],['Planned'])
        self.assertEqual(len(result['scopes']),11)
        self.assertEqual(sum(s['declared_id']=='DUPLICATE-ID' for s in result['scopes']),11)
        with self.assertRaisesRegex(ValueError,'Unknown requested'):
            catalog.query_requirements(result,['DUPLICATE-ID'])

    def test_acceptance_failures_join_system_and_validation_groups(self):
        self.receipt['results'][0].update(status='failed',returncode=1)
        self.assertEqual(self.make()['failed_by_type_platform'],[
            {'test_type':'Acceptance','platform':'Linux-x86_64','tests':['Good']},
            {'test_type':'System','platform':'Linux-x86_64','tests':['Bad']},
            {'test_type':'Validation','platform':'Windows-x86_64','tests':['Slow']}])

    def test_no_test_query_distinguishes_unrun_stale_skipped_and_failed(self):
        result=self.make()
        self.assertEqual(result['requirements_without_tests'],['R4'])
        states={r['key']:r['state'] for r in result['scopes']}
        self.assertEqual(states,{'R1':'verified-in-supplied-report','R2':'failed','R3':'failed',
            'R4':'no-tests','R5':'unexecuted','R6':'unverified-evidence','R7':'skipped',
            'R8':'interrupted','R9':'verified-in-supplied-report','R10':'unexecuted','H1':'failed'})

    def test_exact_mixed_summary_counts_names_and_optional_fields(self):
        summary=self.make()['summary']
        self.assertEqual({k:summary[k] for k in ['tests_in_receipt','tests_in_catalogue','tests_run',
            'reused_passes','execution_unproven','tests_passed','tests_failed','failed_requirements']},
            {'tests_in_receipt':8,'tests_in_catalogue':9,'tests_run':5,'reused_passes':1,
             'execution_unproven':1,'tests_passed':3,'tests_failed':2,'failed_requirements':2})
        self.assertEqual(summary['failed_test_names'],['Bad','Slow'])
        self.assertEqual(summary['failed_requirement_keys'],['R2','R3'])
        self.assertEqual(summary['failed_requirement_names'],[
            {'key':'R2','declared_id':'DUPLICATE-ID','name':'Requirement R2'},
            {'key':'R3','declared_id':'DUPLICATE-ID','name':'Requirement R3'}])
        self.assertEqual(summary['executed_status_counts'],{'passed':2,'failed':1,'timeout':1,'interrupted':1})

    def test_unknown_metadata_and_execution_provenance_stay_unknown(self):
        self.receipt['results'].append({'name':'OldPass','status':'passed'})
        result=self.make();test=next(t for t in result['tests'] if t['name']=='OldPass')
        self.assertEqual(test['execution'],'execution-unproven')
        self.assertEqual(test['test_type'],'unclassified')
        self.assertIn('author',test['metadata_unspecified'])
        self.assertEqual(result['summary']['tests_run'],5)
        self.assertEqual(result['summary']['execution_unproven'],2)

    def test_failed_or_unrun_input_cannot_inherit_a_verified_mapping(self):
        self.receipt['results'][0]['status']='failed';self.receipt['results'][0]['returncode']=1
        states={r['key']:r['state'] for r in self.make()['scopes']}
        self.assertEqual(states['R1'],'failed')
        self.receipt['results'][0]['status']='not-run'
        self.assertEqual(next(r['state'] for r in self.make()['scopes'] if r['key']=='R1'),'unexecuted')

    def test_unknown_status_duplicate_case_invalid_reuse_and_unknown_scope_reject(self):
        original=copy.deepcopy(self.receipt)
        self.receipt['results'].append(copy.deepcopy(self.receipt['results'][0]))
        with self.assertRaisesRegex(ValueError,'duplicate'):
            self.make()
        self.receipt=copy.deepcopy(original);self.receipt['results'][0]['status']='probably passed'
        with self.assertRaisesRegex(ValueError,'Unknown test status'):
            self.make()
        self.receipt=copy.deepcopy(original);self.receipt['results'][1]['reused_from']='old.json'
        with self.assertRaisesRegex(ValueError,'Only passing'):
            self.make()
        self.receipt=copy.deepcopy(original);self.receipt['results'][0]['returncode']=1
        with self.assertRaisesRegex(ValueError,'nonzero returncode'):
            self.make()
        self.receipt=copy.deepcopy(original);self.metadata['tests']['Good']['requirements']=['typo']
        with self.assertRaisesRegex(ValueError,'unknown requirement'):
            self.make()

    def test_actual_cli_export_all_metadata_queries_and_input_identities(self):
        with tempfile.TemporaryDirectory(prefix='GMAT catalogue Δ ') as directory:
            root=Path(directory)
            paths=[]
            for name,value in [('receipt',self.receipt),('nasa',self.report),('metadata',self.metadata)]:
                path=root/(name+'.json');path.write_text(json.dumps(value,ensure_ascii=False));paths.append(path)
            output=root/'out.json'
            command=[sys.executable,str(Path(catalog.__file__)), '--receipt',str(paths[0]),
                     '--nasa-report',str(paths[1]),'--metadata',str(paths[2]),'--output',str(output)]
            def run(extra):
                completed=subprocess.run(command+extra,capture_output=True,text=True)
                self.assertEqual(completed.returncode,0,completed.stderr)
                return json.loads(output.read_text())
            result=run(['--build-date','2026-06-01','--platform','Linux-x86_64'])
            self.assertEqual(result['summary']['tests_run'],5)
            self.assertEqual(next(t['metadata'] for t in result['tests'] if t['name']=='Good'),self.metadata['tests']['Good'])
            self.assertEqual(result['input_sha256'],{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths})
            self.assertEqual(run(['--query','no-tests']),['R4'])
            self.assertEqual(run(['--query','summary'])['failed_requirement_keys'],['R2','R3'])
            self.assertEqual([t['name'] for t in run(['--query','requirements','--requirement','R1','--requirement','R2'])],['Bad','Good'])
            self.assertEqual(run(['--query','failed']),[
                {'test_type':'System','platform':'Linux-x86_64','tests':['Bad']},
                {'test_type':'Validation','platform':'Windows-x86_64','tests':['Slow']}])


PROOF_FIELDS = {
    311:'failed_by_type_platform',312:'test_type',313:'platform',314:'requirement-set-query',
    315:'requirements_without_tests',316:'earliest_build_date',317:'latest_build_date',
    318:'requirements',319:'author',320:'truth_data',321:'tolerances',322:'comparator',
    323:'extensions',324:'comments',325:'creation_date',326:'platform_dependencies',
    331:'summary',332:'tests_run',333:'failed_test_names',334:'tests_passed',
    335:'failed_requirement_keys-and-names'}


if __name__ == '__main__':
    suite=unittest.defaultTestLoader.loadTestsFromTestCase(CatalogContracts)
    result=unittest.TextTestRunner(verbosity=2).run(suite)
    if result.wasSuccessful():
        for row,field in PROOF_FIELDS.items():
            print('NASA_CHECK '+json.dumps(dict(checkpoint='catalogue-'+str(row),outcome='passed',
                field=field,source_key='All Others!E'+str(row),contracts=result.testsRun,
                facets=['mixed-outcome-independent-literal-oracle','metadata-roundtrip-and-validation',
                        'actual-cli-exports-and-queries','no-new-gui-execution-or-fixture-tolerance-change'])),flush=True)
    sys.exit(0 if result.wasSuccessful() else 1)
