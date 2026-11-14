"""Independent file-comparator, plugin, CLI, and HTML acceptance contracts."""
import hashlib
from html.parser import HTMLParser
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

import AcceptanceCompare as compare


PV = '''# FRAME=EarthMJ2000Eq
# TIME_SYSTEM=A1
# TIME_UNIT=day
# POSITION_UNIT=km
# VELOCITY_UNIT=km/s
epoch x y z vx vy vz
1 7000 2 3 4 5 6
2 7001 3 4 5 6 7
'''


def oem(version='2.0', acceleration=False, covariance=False):
    result = '''CCSDS_OEM_VERS = VERSION
CREATION_DATE = 2026-01-01T00:00:00
ORIGINATOR = Independent literal fixture
META_START
OBJECT_NAME = Satellite
OBJECT_ID = 2026-001A
CENTER_NAME = Earth
REF_FRAME = EME2000
TIME_SYSTEM = UTC
START_TIME = 2026-01-01T00:00:00
STOP_TIME = 2026-01-01T00:01:00
META_STOP
2026-01-01T00:00:00 7000 2 3 4 5 6EXTRA
2026-01-01T00:01:00 7001 3 4 5 6 7EXTRA
'''.replace('VERSION',version).replace('EXTRA',' 8 9 10' if acceleration else '')
    if covariance:
        result += '''COVARIANCE_START
EPOCH = 2026-01-01T00:00:00
COV_REF_FRAME = EME2000
1
2 3
4 5 6
7 8 9 10
11 12 13 14 15
16 17 18 19 20 21
COVARIANCE_STOP
'''
    return result


class Page(HTMLParser):
    def __init__(self, text):
        super().__init__();self.tags=[];self.data=[];self.links=[];self.feed(text)

    def handle_starttag(self, tag, attrs):
        self.tags.append(tag)
        self.links += [value for key,value in attrs if key in ('src','href')]

    def handle_data(self, text):
        self.data.append(text)


class ComparatorContracts(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='GMAT compare Δ ')
        self.addCleanup(self.temp.cleanup);self.root=Path(self.temp.name)
        self.actual=self.root/'actual α.txt';self.truth=self.root/'truth β.txt'
        self.registry=compare.Registry()

    def run_compare(self, actual, truth=None, kind='numeric', policy=None, **kwargs):
        if truth is None:truth=actual
        self.actual.write_bytes(actual if isinstance(actual,bytes) else actual.encode())
        self.truth.write_bytes(truth if isinstance(truth,bytes) else truth.encode())
        return self.registry.compare(self.actual,self.truth,kind,policy,**kwargs)

    def cli(self, *options):
        return subprocess.run([sys.executable,str(Path(compare.__file__)),*map(str,options)],
                              capture_output=True,text=True)

    def test_exact_bytes_identity_unicode_bom_eol_and_binary_differences(self):
        self.assertEqual(self.run_compare(b'\x00\xff',kind='file')['status'],'passed')
        for actual,truth in [(b'a\r\n',b'a\n'),(b'\xef\xbb\xbfa',b'a'),('α','β'),(b'a',b'a\n')]:
            with self.subTest(actual=actual):
                result=self.run_compare(actual,truth,'file')
                self.assertEqual(result['status'],'failed')
                self.assertNotEqual(result['inputs']['actual']['sha256'],result['inputs']['truth']['sha256'])
        result=self.run_compare('a\n'*300,'b\n'*300,'file')
        self.assertEqual(len(result['diff_preview']),200)
        self.assertTrue(result['diff_preview_truncated'])

    def test_all_numeric_fields_signed_deltas_and_inclusive_absolute_boundary(self):
        actual='tag 10.125 -2.125 3D+0\n4 5 6\n'
        truth='tag 10 -2 3\n4 5 6\n'
        result=self.run_compare(actual,truth,policy={'absolute':'.125'})
        self.assertEqual((result['status'],result['compared_fields']),('passed',6))
        self.assertEqual([c['delta'] for c in result['comparisons']],['0.125','-0.125','0','0','0','0'])
        result=self.run_compare(actual,truth,policy={'absolute':'.124'})
        self.assertEqual((result['status'],result['exceeded_fields']),('failed',2))

    def test_decimal_small_delta_at_large_origin_is_never_rounded_away(self):
        result=self.run_compare('1000000000.00000001','1000000000',policy={'absolute':'0.000000009'})
        self.assertEqual(result['status'],'failed')
        self.assertEqual(result['comparisons'][0]['delta'],'1E-8')
        self.assertEqual(result['comparisons'][0]['allowed'],'9E-9')

    def test_relative_tolerance_is_truth_scaled_and_field_override_is_exact(self):
        result=self.run_compare('11 11','10 10',policy={'relative':'.09','fields':{'2':{'absolute':'.1'}}})
        self.assertEqual([c['exceeded'] for c in result['comparisons']],[True,False])
        self.assertEqual([c['allowed'] for c in result['comparisons']],['0.90','1.00'])

    def test_tolerance_sum_and_long_delta_keep_all_decimal_digits(self):
        result=self.run_compare('0.'+'1'*60,'0')
        self.assertEqual(result['status'],'failed')
        self.assertEqual(result['comparisons'][0]['delta'],'0.'+'1'*60)
        cell=compare.numeric_cell('x','x','1e-100','1e-100',{'absolute':'1e-100','relative':'1e-100'})
        self.assertEqual(compare.decimal(cell['allowed']),compare.decimal('1.'+'0'*99+'1E-100'))

    def test_csv_quoted_identity_and_all_numeric_columns(self):
        result=self.run_compare('"clock, position",1,2.1','"clock, position",1,2',policy={'absolute':'.1'})
        self.assertEqual((result['status'],result['compared_fields']),('passed',2))
        self.assertEqual([c['field'] for c in result['comparisons']],['2','3'])

    def test_structural_rows_columns_headers_and_numeric_type_mismatch_fail(self):
        for actual,truth in [('A 1','B 1'),('1 2','1'),('1\n2','1'),('1 nope','1 2')]:
            with self.subTest(actual=actual):
                result=self.run_compare(actual,truth)
                self.assertEqual(result['status'],'failed')
                self.assertTrue(result['differences'])

    def test_nonfinite_missing_inputs_nonnumeric_and_invalid_policies_error(self):
        for payload in ['NaN 1','inf 1','-Infinity 1','header only']:
            self.assertEqual(self.run_compare(payload)['status'],'error')
        for policy in [False,{'absolute':True},{'relative':-1},{'absolute':'NaN'},
                       {'unknown':0},{'fields':{'X':{'fields':{}}}}]:
            self.assertEqual(self.run_compare('1',policy=policy)['status'],'error')
        self.truth.unlink()
        self.assertEqual(self.registry.compare(self.actual,self.truth,'numeric')['status'],'error')

    def test_position_velocity_all_components_units_and_exact_epoch_matching(self):
        result=self.run_compare(PV,kind='position-velocity')
        self.assertEqual((result['status'],result['compared_fields']),('passed',12))
        self.assertEqual([c['field'] for c in result['comparisons'][:6]],['X','Y','Z','VX','VY','VZ'])
        result=self.run_compare(PV.replace('7000','7000.25'),PV,'position-velocity',{'fields':{'X':{'absolute':'.25'}}})
        self.assertEqual(result['status'],'passed')
        self.assertEqual(result['comparisons'][0]['delta'],'0.25')
        self.assertEqual([c['unit'] for c in result['comparisons'][:6]],['km']*3+['km/s']*3)
        result=self.run_compare(PV.replace('1 7000','1.1 7000'),PV,'position-velocity')
        self.assertEqual((result['status'],result['compared_fields']),('failed',6))

    def test_position_velocity_identity_mismatch_is_never_numerically_compared(self):
        for old,new in [('EarthMJ2000Eq','EarthFixed'),('TIME_SYSTEM=A1','TIME_SYSTEM=UTC'),
                        ('POSITION_UNIT=km','POSITION_UNIT=m'),('TIME_UNIT=day','TIME_UNIT=s')]:
            result=self.run_compare(PV.replace(old,new),PV,'position-velocity')
            self.assertEqual((result['status'],result['compared_fields']),('failed',0))

    def test_position_velocity_missing_headers_dimensions_order_and_rows(self):
        for actual in [PV.replace('# POSITION_UNIT=km\n',''),PV.replace('7000 2','7000'),
                       PV.replace('2 7001','1 7001'),PV.replace('2 7001','0 7001'),
                       PV.replace('7000','NaN')]:
            self.assertEqual(self.run_compare(actual,PV,'position-velocity')['status'],'error')
        self.assertEqual(self.run_compare(PV.replace('2 7001 3 4 5 6 7\n',''),PV,'position-velocity')['status'],'failed')

    def test_oem_versions_all_states_acceleration_covariance_and_segments(self):
        for version in ['1.0','2.0']:
            result=self.run_compare(oem(version),kind='oem')
            self.assertEqual((result['status'],result['compared_fields']),('passed',12))
        fixture=oem(acceleration=True,covariance=True)
        # Two independently specified segments; compare every state and lower triangle.
        fixture += fixture[fixture.index('META_START'):].replace('Satellite','Satellite2')
        result=self.run_compare(fixture,kind='oem')
        self.assertEqual((result['status'],result['compared_fields']),('passed',78))
        self.assertEqual([c['field'] for c in result['comparisons'][18:39]],
                         ['C11','C21','C22','C31','C32','C33','C41','C42','C43','C44',
                          'C51','C52','C53','C54','C55','C61','C62','C63','C64','C65','C66'])
        changed=fixture.replace('16 17 18 19 20 21','16 17 18 19 20 21.5',1)
        result=self.run_compare(changed,fixture,'oem')
        self.assertEqual((result['status'],result['exceeded_fields']),('failed',1))
        self.assertEqual([c['delta'] for c in result['comparisons'] if c['exceeded']],['0.5'])

    def test_oem_metadata_frame_object_timescale_and_segment_conjunction(self):
        fixture=oem()
        for old,new in [('EME2000','ITRF2000'),('Satellite','Other'),('2026-001A','2026-002A'),
                        ('TIME_SYSTEM = UTC','TIME_SYSTEM = TAI'),('CENTER_NAME = Earth','CENTER_NAME = Mars')]:
            result=self.run_compare(fixture.replace(old,new),fixture,'oem')
            self.assertEqual((result['status'],result['compared_fields']),('failed',0))
        result=self.run_compare(fixture+fixture[fixture.index('META_START'):],fixture,'oem')
        self.assertEqual(result['status'],'failed')
        self.assertTrue(any(d['field']=='segment-count' for d in result['differences']))

    def test_oem_epoch_precision_zero_format_volatile_headers_and_strict_option(self):
        fixture=oem()
        changed=fixture.replace('00:00:00','00:00:00.000Z').replace('Independent literal fixture','A different generator')
        result=self.run_compare(changed,fixture,'oem')
        self.assertEqual(result['status'],'passed')
        self.assertEqual(result['ignored_header_fields'],['CREATION_DATE','ORIGINATOR'])
        self.assertEqual(self.run_compare(changed,fixture,'oem',strict_header=True)['status'],'failed')
        changed=fixture.replace('T00:00:00 7000','T00:00:00.000000001 7000')
        self.assertEqual(self.run_compare(changed,fixture,'oem')['status'],'failed')
        self.assertNotEqual(compare.epoch('2016-12-31T23:59:60','UTC'),compare.epoch('2017-01-01T00:00:00','UTC'))
        for label,system in [('2026-02-30T00:00:00','UTC'),('2026-01-01T25:00:00','UTC'),
                             ('2026-01-01T12:00:60','UTC'),('2026-01-01T00:00:00Z','TAI')]:
            with self.assertRaises(ValueError):compare.epoch(label,system)

    def test_oem_missing_acceleration_covariance_and_epoch_records_fail(self):
        fixture=oem(acceleration=True,covariance=True)
        without_acceleration='\n'.join(line.removesuffix(' 8 9 10') if line.startswith('2026-') else line
                                       for line in fixture.split('\n'))
        for changed in [without_acceleration,oem(acceleration=True),
                        fixture.replace('2026-01-01T00:01:00 7001 3 4 5 6 7 8 9 10\n',''),
                        fixture.replace('COV_REF_FRAME = EME2000','COV_REF_FRAME = ITRF2000')]:
            self.assertEqual(self.run_compare(changed,fixture,'oem')['status'],'failed')

    def test_oem_invalid_structure_version_nonfinite_triangle_order_and_bounds_error(self):
        fixture=oem(covariance=True)
        invalid=[oem('3.0'),oem('1.0',acceleration=True),oem('1.0',covariance=True),
                 fixture.replace('META_STOP\n',''),fixture.replace('OBJECT_ID = 2026-001A\n',''),
                 fixture.replace('16 17 18 19 20 21','16 17'),fixture.replace('COVARIANCE_STOP',''),
                 fixture.replace('7000','NaN'),fixture.replace('00:01:00 7001','00:00:00 7001'),
                 fixture.replace('EPOCH = 2026-01-01','EPOCH = 2025-01-01'),
                 fixture.replace('STOP_TIME = 2026-01-01T00:01:00','STOP_TIME = 2026-01-01T00:00:00'),
                 '<oem>unsupported XML</oem>','']
        for changed in invalid:
            with self.subTest(changed=changed[:80]):
                self.assertEqual(self.run_compare(changed,fixture,'oem')['status'],'error')

    def test_custom_comparator_registration_success_and_structure_failures(self):
        def custom(a,t,r,p):
            r['comparisons'].append(compare.numeric_cell('custom','length',len(a),len(t),p,'byte'))
        self.registry.register('length',custom)
        result=self.run_compare('abc','xy','length',{'absolute':1})
        self.assertEqual((result['status'],result['comparisons'][0]['delta']),('passed','1'))
        self.assertEqual(self.run_compare('abc','xy','length')['status'],'failed')
        for name,callback in [('numeric',custom),('file',custom),('length',custom),('',custom),('bad',None)]:
            with self.assertRaises(ValueError):self.registry.register(name,callback)

    def test_invalid_throwing_empty_and_nonserializable_custom_output_is_renderable_error(self):
        def throwing(a,t,r,p):raise RuntimeError('plugin failed')
        callbacks=[lambda a,t,r,p:None,throwing,
                   lambda a,t,r,p:r['comparisons'].append({'exceeded':False}),
                   lambda a,t,r,p:r['differences'].append({'location':'bad'}),
                   lambda a,t,r,p:r['differences'].append(dict(location='bad',field='bad',actual=float('nan'),truth=0)),
                   lambda a,t,r,p:r['comparisons'].append(dict(location='x',field='x',actual='1',truth='0',delta='1',allowed='0',exceeded=False))]
        for i,callback in enumerate(callbacks):
            self.registry.register(str(i),callback)
            result=self.run_compare('1',kind=str(i))
            self.assertEqual(result['status'],'error')
            json.dumps(result,allow_nan=False)
            self.assertIn('ERROR',compare.render_comparison(result))
        self.assertEqual(self.run_compare('1',kind='unknown')['status'],'error')

    def test_actual_cli_pass_fail_error_json_html_unicode_and_sha(self):
        self.run_compare('1.125','1')
        output=self.root/'report Δ.json';page=self.root/'report Δ.html'
        arguments=['--actual',self.actual,'--truth',self.truth,'--json',output,'--html',page]
        for tolerance,code,status in [('.125',0,'passed'),('.124',1,'failed'),('NaN',2,'error')]:
            process=self.cli(*arguments,'--absolute',tolerance)
            self.assertEqual(process.returncode,code,process.stderr)
            report=json.loads(output.read_text())
            self.assertEqual(report['status'],status)
            self.assertIn(status.upper(),page.read_text())
            if code<2:self.assertEqual(report['inputs']['actual']['sha256'],hashlib.sha256(self.actual.read_bytes()).hexdigest())

    def test_actual_cli_custom_plugin_and_policy_file(self):
        plugin=self.root/'my comparator.py'
        plugin.write_text('''def register_comparators(registry):
    def compare(a,t,r,p):
        from AcceptanceCompare import numeric_cell
        r['comparisons'].append(numeric_cell('plugin','bytes',len(a),len(t),p))
    registry.register('bytes',compare)
''')
        self.run_compare('abc','xy');policy=self.root/'policy.json';policy.write_text('{"absolute":1}')
        output=self.root/'report.json'
        process=self.cli('--actual',self.actual,'--truth',self.truth,'--kind','bytes',
                         '--custom-module',plugin,'--policy',policy,'--json',output)
        self.assertEqual(process.returncode,0,process.stderr)
        report=json.loads(output.read_text())
        self.assertEqual(report['comparisons'][0]['delta'],'1')
        self.assertEqual(report['custom_module']['sha256'],hashlib.sha256(plugin.read_bytes()).hexdigest())

    def test_cli_protects_inputs_symlinks_hardlinks_and_preflights_both_outputs(self):
        self.run_compare('1','2');original=self.actual.read_bytes()
        symlink=self.root/'symbolic.txt';symlink.symlink_to(self.actual)
        hardlink=self.root/'hard.txt';hardlink.hardlink_to(self.actual)
        safe=self.root/'safe.json'
        for output in [self.actual,self.truth,symlink,hardlink]:
            process=self.cli('--actual',self.actual,'--truth',self.truth,'--json',safe,'--html',output)
            self.assertEqual(process.returncode,2,process.stderr)
            self.assertFalse(safe.exists())
            self.assertEqual(self.actual.read_bytes(),original)
        process=self.cli('--actual',self.actual,'--truth',self.truth,'--json',safe,'--html',safe)
        self.assertEqual(process.returncode,2)
        self.assertFalse(safe.exists())

    def test_atomic_output_failure_preserves_old_report_and_cleans_temporary(self):
        output=self.root/'report.html';output.write_text('old report')
        before=set(self.root.iterdir())
        with mock.patch.object(compare.os,'replace',side_effect=OSError('full drive')):
            with self.assertRaises(OSError):compare.write_output(output,'new report',[])
        self.assertEqual(output.read_text(),'old report')
        self.assertEqual(set(self.root.iterdir()),before)

    def test_html_escapes_unsafe_content_reports_every_difference_and_is_standalone(self):
        result=self.run_compare('1.25 2','1 2')
        result['comparisons'][0]['field']='<script>alert("x")</script>'
        result['inputs']['actual']['path']='<img src=x onerror=alert(1)>'
        page=compare.render_comparison(result);parsed=Page(page)
        self.assertNotIn('script',parsed.tags);self.assertNotIn('img',parsed.tags)
        self.assertEqual(parsed.links,[])
        text=' '.join(parsed.data)
        for expected in ['FAILED','2 numeric fields checked','0.25','0','SHA256','<script>alert("x")</script>']:
            self.assertIn(expected,text)
        self.assertEqual(parsed.tags.count('table'),3)
        self.assertIn('Content-Security-Policy',page)

    def test_actual_catalogue_html_preserves_recorded_failures_reuse_unknown_and_no_execution(self):
        # Fixed literal catalogue, independent of catalogue construction logic.
        catalog=dict(schema_version=1,summary=dict(tests_failed=2,tests_passed=3,tests_run=4,reused_passes=1),
            tests=[dict(name='Unsafe <failure>',status='failed',execution='executed',test_type='System',
                        platform='unknown',requirements=['R1']),
                   dict(name='Retained pass',status='passed',execution='reused',test_type='Acceptance',
                        platform='Linux',requirements=['R2'])],
            scopes=[dict(key='R1',kind='requirement',name='Failed <requirement>',state='failed',tests=['Unsafe <failure>'])])
        source=self.root/'catalogue.json';source.write_text(json.dumps(catalog));original=source.read_bytes()
        output=self.root/'catalogue.html'
        process=self.cli('--catalogue',source,'--html',output)
        self.assertEqual(process.returncode,0,process.stderr)
        page=Page(output.read_text());text=' '.join(page.data)
        for expected in ['Recorded failures: 2','passed: 3','run: 4','reused: 1','unknown','Failed <requirement>']:
            self.assertIn(expected,text)
        self.assertEqual(source.read_bytes(),original)
        self.assertEqual(page.links,[])


PROOF_FIELDS = {
    303:'position-velocity',304:'file-diff',305:'all-numeric-fields-and-differences',
    307:'CCSDS-OEM',309:'user-defined-comparator',330:'standalone-html'}
FACETS = ['independent-literal-numeric-and-structure-oracles','actual-cli-and-input-protection',
          'complete-state-acceleration-covariance-comparison','custom-plugin-validation',
          'escaped-standalone-html-and-mixed-catalogue','no-gui-core-or-fixture-assertion-change']


if __name__=='__main__':
    result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(ComparatorContracts))
    if result.wasSuccessful():
        for row,field in PROOF_FIELDS.items():
            print('NASA_CHECK '+json.dumps(dict(checkpoint='comparator-'+str(row),outcome='passed',
                field=field,source_key='All Others!E'+str(row),contracts=result.testsRun,facets=FACETS)),flush=True)
    sys.exit(0 if result.wasSuccessful() else 1)
