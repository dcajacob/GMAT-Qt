"""Independent TDM and real binary-SPK comparator acceptance contracts."""
import ctypes as C
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import uuid
import xml.etree.ElementTree as ET

import AcceptanceCompare as base
import AcceptanceSpecializedCompare as specialized

ARTIFACTS = None
TDM = '''CCSDS_TDM_VERS = 2.0
CREATION_DATE = 2026-01-01T00:00:00
ORIGINATOR = Literal independent fixture
META_START
TIME_SYSTEM = UTC
START_TIME = 2026-001T00:00:00
STOP_TIME = 2026-001T00:01:00
PARTICIPANT_1 = DSS-14
PARTICIPANT_2 = SPACECRAFT-TEST
MODE = SEQUENTIAL
PATH = 1,2,1
RANGE_UNITS = km
ANGLE_TYPE = AZEL
REFERENCE_FRAME = EME2000
INTEGRATION_INTERVAL = 10
INTEGRATION_REF = END
TIMETAG_REF = RECEIVE
CORRECTION_RANGE = 0.25
CORRECTIONS_APPLIED = NO
META_STOP
DATA_START
RANGE = 2026-001T00:00:00 7000
DOPPLER_INTEGRATED = 2026-001T00:00:00 -1.25
ANGLE_1 = 2026-001T00:00:00 45.5
ANGLE_2 = 2026-001T00:00:00 30
RECEIVE_FREQ_1 = 2026-001T00:00:00 8400000000
TRANSMIT_PHASE_CT_1 = 2026-001T00:00:00 9007199254740993.00000001
RANGE = 2026-001T00:01:00 7001
DOPPLER_INTEGRATED = 2026-001T00:01:00 -1.5
DATA_STOP
'''


def xml_fixture():
    # A separately constructed XML message, not output from the parser under test.
    return '''<?xml version="1.0"?><tdm id="CCSDS_TDM_VERS" version="2.0" xmlns="urn:ccsds:schema:ndmxml">
<header><CREATION_DATE>2026-01-02T01:00:00</CREATION_DATE><ORIGINATOR>Different producer</ORIGINATOR></header>
<body><segment><metadata><TIME_SYSTEM>UTC</TIME_SYSTEM><START_TIME>2026-01-01T00:00:00.000Z</START_TIME><STOP_TIME>2026-01-01T00:01:00Z</STOP_TIME><PARTICIPANT_1>DSS-14</PARTICIPANT_1><PARTICIPANT_2>SPACECRAFT-TEST</PARTICIPANT_2><MODE>SEQUENTIAL</MODE><PATH>1, 2, 1</PATH><RANGE_UNITS>km</RANGE_UNITS><ANGLE_TYPE>AZEL</ANGLE_TYPE><REFERENCE_FRAME>EME2000</REFERENCE_FRAME><INTEGRATION_INTERVAL>10</INTEGRATION_INTERVAL><INTEGRATION_REF>END</INTEGRATION_REF><TIMETAG_REF>RECEIVE</TIMETAG_REF><CORRECTION_RANGE>0.25</CORRECTION_RANGE><CORRECTIONS_APPLIED>NO</CORRECTIONS_APPLIED></metadata><data>
<observation><EPOCH>2026-01-01T00:00:00Z</EPOCH><RANGE units="km">7D+3</RANGE></observation>
<observation><EPOCH>2026-01-01T00:00:00Z</EPOCH><DOPPLER_INTEGRATED units="km/s">-1.25</DOPPLER_INTEGRATED></observation>
<observation><EPOCH>2026-01-01T00:00:00Z</EPOCH><ANGLE_1 units="deg">45.5</ANGLE_1></observation>
<observation><EPOCH>2026-01-01T00:00:00Z</EPOCH><ANGLE_2 units="deg">30</ANGLE_2></observation>
<observation><EPOCH>2026-01-01T00:00:00Z</EPOCH><RECEIVE_FREQ_1 units="Hz">8400000000</RECEIVE_FREQ_1></observation>
<observation><EPOCH>2026-01-01T00:00:00Z</EPOCH><TRANSMIT_PHASE_CT_1>9007199254740993.00000001</TRANSMIT_PHASE_CT_1></observation>
<observation><EPOCH>2026-01-01T00:01:00Z</EPOCH><RANGE>7001</RANGE></observation>
<observation><EPOCH>2026-01-01T00:01:00Z</EPOCH><DOPPLER_INTEGRATED>-1.5</DOPPLER_INTEGRATED></observation>
</data></segment></body></tdm>'''


def polynomial(t, body=-1001, perturb=0):
    offset = 100 if body == -1002 else 0
    return [7000+offset+2*t+.01*t*t+perturb*t*(30-t), 500-3*t+.02*t*t, -400+t-.03*t*t,
            2+.02*t+perturb*(30-2*t), -3+.04*t, 1-.06*t]


def make_spk(path, kind=8, bodies=(-1001, -1002), windows=((0, 30),), perturb=0, frame='J2000', center=399):
    spice = specialized.Spice(specialized.SPK_DEFAULT['library'])
    spice.lib.spkopn_c.argtypes = [C.c_char_p, C.c_char_p, C.c_int, C.POINTER(C.c_int)]; spice.lib.spkopn_c.restype = None
    spice.lib.spkcls_c.argtypes = [C.c_int]; spice.lib.spkcls_c.restype = None
    row = C.c_double*6
    args = [C.c_int, C.c_int, C.c_int, C.c_char_p, C.c_double, C.c_double, C.c_char_p, C.c_int, C.c_int, C.POINTER(row)]
    spice.lib.spkw08_c.argtypes = args+[C.c_double, C.c_double]; spice.lib.spkw08_c.restype = None
    spice.lib.spkw13_c.argtypes = args+[C.POINTER(C.c_double)]; spice.lib.spkw13_c.restype = None
    handle = C.c_int(); spice.call('spkopn_c', str(path).encode(), b'Independent polynomial', 0, C.byref(handle))
    try:
        for body in bodies:
            for start, stop in windows:
                times = [start+(stop-start)*i/3 for i in range(4)]
                states = (row*4)(*(row(*polynomial(t, body, perturb)) for t in times))
                call_args = [handle.value, body, center, frame.encode(), start, stop, b'Polynomial truth', 3, 4, states]
                if kind == 8: spice.call('spkw08_c', *call_args, times[0], times[1]-times[0])
                else: spice.call('spkw13_c', *call_args, (C.c_double*4)(*times))
    finally: spice.call('spkcls_c', handle.value)


class SpecializedComparatorContracts(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='GMAT specialized Δ '); self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name); self.a = self.root/'actual α.dat'; self.t = self.root/'truth β.dat'
        self.registry = base.Registry(); specialized.register_comparators(self.registry)

    def tdm(self, actual=TDM, truth=TDM, policy=None):
        self.a.write_text(actual); self.t.write_text(truth)
        return self.registry.compare(self.a, self.t, 'tdm', policy)

    def spk(self, options=None, policy=None):
        selected = {**specialized.SPK_DEFAULT, 'observer':399, 'step':7, **(options or {})}
        registry = base.Registry(); registry.register('spk', lambda a,t,r,p: specialized.compare_spk(a,t,r,p,selected))
        return registry.compare(self.a, self.t, 'spk', policy or {'absolute':'1e-9'})

    def retain(self, name, report):
        if ARTIFACTS:
            (ARTIFACTS/(name+'.json')).write_text(json.dumps(report, indent=2)+'\n')
            (ARTIFACTS/(name+'.html')).write_text(specialized.render(report))
            for role, p in [('actual', self.a), ('truth', self.t)]: (ARTIFACTS/(name+'-'+role+'.dat')).write_bytes(p.read_bytes())

    def test_tdm_all_records_kvn_xml_and_independent_exact_values(self):
        report = self.tdm(xml_fixture(), TDM)
        self.assertEqual(report['status'], 'passed', report['errors'])
        self.assertEqual(report['compared_fields'], 8)
        expected = {'RANGE':['7000','7001'], 'DOPPLER_INTEGRATED':['-1.25','-1.5'], 'ANGLE_1':['45.5'], 'ANGLE_2':['30'], 'RECEIVE_FREQ_1':['8400000000'], 'TRANSMIT_PHASE_CT_1':['9007199254740993.00000001']}
        for kind, values in expected.items():
            rows = [c for c in report['comparisons'] if c['field'] == kind]
            self.assertEqual([c['truth'] for c in rows], values)
            self.assertTrue(all(c['delta'] == '0' and not c['exceeded'] for c in rows))
        self.retain('tdm-kvn-xml', report)
        self.assertEqual(self.tdm(TDM.replace('2.0','1.0',1))['status'], 'failed')
        self.assertEqual(self.tdm(TDM.replace('2.0','1.0',1),TDM.replace('2.0','1.0',1))['status'], 'passed')

    def test_tdm_exact_phase_delta_field_tolerances_and_inclusive_boundary(self):
        changed = TDM.replace('9007199254740993.00000001','9007199254740993.00000002')
        report = self.tdm(changed, policy={'absolute':'9e-9'})
        self.assertEqual((report['status'], report['exceeded_fields']), ('failed', 1))
        cell = next(c for c in report['comparisons'] if c['exceeded'])
        self.assertEqual((cell['delta'], cell['allowed']), ('1E-8','9E-9'))
        self.assertEqual(self.tdm(changed, policy={'fields':{'TRANSMIT_PHASE_CT_1':{'absolute':'1e-8'}}})['status'], 'passed')
        changed = TDM.replace('7001','7001.7')
        self.assertEqual(self.tdm(changed, policy={'relative':'.0001'})['status'], 'passed')
        self.assertEqual(self.tdm(changed, policy={'relative':'.00009'})['status'], 'failed')

    def test_tdm_all_semantic_metadata_and_extra_segments_are_checked(self):
        mutations = [('DSS-14','DSS-15'),('PATH = 1,2,1','PATH = 1,2'),('RANGE_UNITS = km','RANGE_UNITS = s'),('UTC','TAI'),('EME2000','ICRF'),('INTEGRATION_REF = END','INTEGRATION_REF = START'),('CORRECTIONS_APPLIED = NO','CORRECTIONS_APPLIED = YES'),('CORRECTION_RANGE = 0.25','CORRECTION_RANGE = 0.5'),('INTEGRATION_INTERVAL = 10','INTEGRATION_INTERVAL = 20')]
        for old, new in mutations:
            with self.subTest(new=new): self.assertEqual(self.tdm(TDM.replace(old,new))['status'], 'failed')
        second = TDM[TDM.index('META_START'):]
        report = self.tdm(TDM+second, TDM+second.replace('7001','7002'))
        self.assertEqual((report['compared_fields'],report['exceeded_fields']), (16, 1))
        self.assertEqual(self.tdm(TDM+second)['status'], 'failed')

    def test_tdm_clock_identity_leap_labels_and_per_observable_order(self):
        leap = TDM.replace('2026-001T00:00:00','2016-366T23:59:60').replace('2026-001T00:01:00','2017-001T00:00:00')
        report = self.tdm(leap, leap)
        self.assertEqual((report['status'],report['compared_fields']), ('passed',8))
        reordered = TDM.replace('RANGE = 2026-001T00:01:00 7001\n','').replace('DOPPLER_INTEGRATED = 2026-001T00:00:00', 'RANGE = 2026-001T00:01:00 7001\nDOPPLER_INTEGRATED = 2026-001T00:00:00')
        self.assertEqual(self.tdm(reordered)['status'], 'passed')
        self.assertEqual(self.tdm(TDM.replace('RANGE_UNITS = km\n',''))['status'], 'passed')

    def test_tdm_missing_duplicate_unknown_nonfinite_and_malformed_inputs_reject(self):
        row = 'RANGE = 2026-001T00:01:00 7001\n'
        self.assertEqual(self.tdm(TDM.replace(row,''))['status'], 'failed')
        for actual in [TDM.replace(row,row+row), TDM.replace('7001','nan'), TDM.replace('7001','inf'), TDM.replace('RANGE =','UNKNOWN ='),TDM.replace('TIME_SYSTEM = UTC\n',''),TDM.replace('PARTICIPANT_1 = DSS-14\n',''),TDM.replace('DATA_STOP',''),TDM.replace('2026-001','2026-999'),TDM.replace('PARTICIPANT_2 = SPACECRAFT-TEST\n',''),xml_fixture().replace('units="km"','units="m"'),xml_fixture().replace('</observation>','<RANGE>1</RANGE></observation>',1),xml_fixture().replace('<tdm ','<!DOCTYPE x [<!ENTITY e "v">]><tdm ',1)]:
            with self.subTest(actual=actual[:40]): self.assertEqual(self.tdm(actual)['status'], 'error')

    def test_spk_different_segment_types_and_all_samples_match_independent_polynomial(self):
        make_spk(self.a,8); make_spk(self.t,13)
        report = self.spk()
        self.assertEqual(report['status'], 'passed', report['errors'])
        self.assertNotEqual(report['inputs']['actual']['sha256'], report['inputs']['truth']['sha256'])
        self.assertEqual(report['compared_fields'], 72)
        self.assertEqual([r['tdb_seconds'] for r in report['spk_samples'][:6]], [0,7,14,21,28,30])
        for i, sample in enumerate(report['spk_samples']):
            expected = polynomial(sample['tdb_seconds'], sample['object'])
            for j, cell in enumerate(report['comparisons'][6*i:6*i+6]):
                self.assertAlmostEqual(float(cell['actual']), expected[j], delta=2e-10)
                self.assertAlmostEqual(float(cell['truth']), expected[j], delta=2e-10)
                self.assertEqual(cell['unit'], 'km' if j<3 else 'km/s')
        self.assertTrue(report['loaded_dependencies'])
        self.retain('spk-polynomial-types8-13',report)

    def test_spk_interior_drift_and_distinct_position_velocity_tolerance(self):
        make_spk(self.a,perturb=.0001); make_spk(self.t,8)
        report = self.spk()
        self.assertEqual(report['status'],'failed')
        xs = [c for c in report['comparisons'] if c['field']=='X']
        self.assertAlmostEqual(float(xs[0]['delta']),0,delta=1e-9)
        self.assertAlmostEqual(float(xs[-1]['delta']),0,delta=1e-9)
        self.assertTrue(any(c['exceeded'] for c in xs[1:-1]))
        for sample, cells in zip(report['spk_samples'],[report['comparisons'][i:i+6] for i in range(0,len(report['comparisons']),6)]):
            t=sample['tdb_seconds'];self.assertAlmostEqual(float(cells[0]['delta']),.0001*t*(30-t),delta=1e-9);self.assertAlmostEqual(float(cells[3]['delta']),.0001*(30-2*t),delta=1e-10)
        self.assertEqual(self.spk(policy={'fields':{'X':{'absolute':'.03'},'VX':{'absolute':'.004'}}})['status'],'passed')
        self.retain('spk-interior-drift',report)

    def test_spk_disjoint_complete_windows_and_coverage_object_mismatch(self):
        make_spk(self.a,windows=((0,10),(20,30)));make_spk(self.t,13,windows=((0,10),(20,30)))
        report=self.spk();self.assertEqual(report['status'],'passed',report['errors']);self.assertEqual([s['tdb_seconds'] for s in report['spk_samples'][:6]],[0,7,10,20,27,30])
        self.t.unlink();make_spk(self.t,13,windows=((0,30),))
        self.assertEqual(self.spk()['status'],'failed')
        self.t.unlink();make_spk(self.t,13,bodies=(-1001,),windows=((0,10),(20,30)))
        self.assertEqual(self.spk()['status'],'failed')

    def test_spk_frame_observer_missing_dependencies_bad_file_and_explicit_limits(self):
        make_spk(self.a,frame='ECLIPJ2000');make_spk(self.t,13)
        self.assertEqual(self.spk()['status'],'failed')
        self.assertEqual(self.spk(options={'observer':0})['status'],'error')
        self.assertEqual(self.spk(options={'frame':'UNKNOWN_FRAME'})['status'],'error')
        self.assertEqual(self.spk(options={'step':0})['status'],'error')
        self.assertEqual(self.spk(options={'max_samples':1})['status'],'error')
        self.a.write_bytes(b'not a binary SPK')
        self.assertEqual(self.spk()['status'],'error')

    def test_actual_cli_tdm_spk_json_html_exit_codes_and_input_preservation(self):
        self.a.write_text(TDM);self.t.write_text(TDM);out=self.root/'result.json';page=self.root/'result.html'
        command=[sys.executable,str(Path(specialized.__file__)), '--actual',str(self.a),'--truth',str(self.t),'--kind','tdm','--json',str(out),'--html',str(page)]
        p=subprocess.run(command,capture_output=True,text=True);self.assertEqual(p.returncode,0,p.stderr);self.assertEqual(json.loads(out.read_text())['compared_fields'],8);self.assertIn('9007199254740993.00000001',page.read_text());self.assertIn('No unit/time conversion',page.read_text())
        self.a.write_text(TDM.replace('7001','7002'));self.assertEqual(subprocess.run(command,capture_output=True).returncode,1)
        self.a.write_text('bad');self.assertEqual(subprocess.run(command,capture_output=True).returncode,2)
        before=self.t.read_bytes();bad=command.copy();bad[bad.index('--json')+1]=str(self.t);self.assertEqual(subprocess.run(bad,capture_output=True).returncode,2);self.assertEqual(self.t.read_bytes(),before)
        self.a.unlink();self.t.unlink();make_spk(self.a);make_spk(self.t,8)
        command[command.index('--kind')+1]='spk';command+=['--observer','399','--step','7']
        p=subprocess.run(command,capture_output=True,text=True);self.assertEqual(p.returncode,0,p.stderr);report=json.loads(out.read_text());self.assertEqual(report['compared_fields'],72);self.assertEqual(report['spk_policy']['time_system'],'TDB');self.assertIn('J2000',page.read_text());self.assertIn('399',page.read_text());self.assertIn('not a continuous-error bound',page.read_text())
        plugin=[sys.executable,str(Path(base.__file__)),'--actual',str(self.a),'--truth',str(self.t),'--kind','tdm','--custom-module',str(Path(specialized.__file__)),'--json',str(out)]
        self.a.write_text(TDM);self.t.write_text(TDM);self.assertEqual(subprocess.run(plugin,capture_output=True).returncode,0)


if __name__=='__main__':
    if '--output-root' in sys.argv:
        index=sys.argv.index('--output-root');root=Path(sys.argv[index+1]);del sys.argv[index:index+2];ARTIFACTS=root/uuid.uuid4().hex[:12];ARTIFACTS.mkdir(parents=True)
    result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(SpecializedComparatorContracts))
    if result.wasSuccessful():
        artifacts={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in ARTIFACTS.iterdir()} if ARTIFACTS else {}
        for key in (306,308):print('NASA_CHECK '+json.dumps(dict(checkpoint='specialized-comparator-'+str(key),outcome='passed',facets=['two-file-truth-comparison','independent-polynomial-or-exact-literal-oracles','explicit-identity-time-frame-unit-tolerance-policy','malformed-missing-duplicate-and-unsupported-input-errors','actual-cli-JSON-HTML-input-preservation'],contracts=result.testsRun,artifact_sha256=artifacts,GUI_executions=0,runtime_copies=0)))
    raise SystemExit(not result.wasSuccessful())
