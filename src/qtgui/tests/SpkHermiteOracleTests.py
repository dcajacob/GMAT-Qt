"""Offline type13 encoded-curve contracts; no GUI or mission replay."""
import copy,csv,hashlib,json,math,os,struct,sys,unittest
from pathlib import Path
import SpkHermiteOracle as oracle

DATA=Path(__file__).with_name('spk-hermite')
RESULTS=[]

def polynomial(t,degree):
    values=[];derivatives=[]
    for axis in range(3):
        coefficients=[(axis+1)*.02*(j+1)*(-1)**(j+axis)for j in range(degree+1)]
        values.append(math.fsum(c*t**j for j,c in enumerate(coefficients)))
        derivatives.append(math.fsum(j*coefficients[j]*t**(j-1)for j in range(1,len(coefficients))))
    return values+derivatives

def kernel(order='<'):
    epochs=[-.8,-.23,.14,.41,.92];states=[polynomial(t,9)for t in epochs]
    words=[v for state in states for v in state]+epochs+[4.,5.]
    raw=bytearray(4096);raw[:8]=b'DAF/SPK ';raw[88:96]=b'LTL-IEEE'if order=='<'else b'BIG-IEEE'
    struct.pack_into(order+'2i',raw,8,2,6);struct.pack_into(order+'3i',raw,76,2,2,385+len(words));struct.pack_into(order+'3d',raw,1024,0,0,1)
    struct.pack_into(order+'2d6i',raw,1048,epochs[0],epochs[-1],-100056,399,1,13,385,384+len(words));struct.pack_into(order+str(len(words))+'d',raw,3072,*words)
    return bytes(raw)

class Contract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest=json.loads((DATA/'manifest.json').read_text());cls.cases=[]
        for case in cls.manifest['cases']:
            raw=(DATA/case['kernel']).read_bytes();samples=(DATA/case['queries']).read_bytes()
            assert hashlib.sha256(raw).hexdigest()==case['kernel_sha256']and hashlib.sha256(samples).hexdigest()==case['queries_sha256']
            cls.cases.append((case,raw,oracle.read_segments(raw),oracle.read_queries(DATA/case['queries'])))

    def test_all_five_hermite_orders_against_independent_polynomials(self):
        for size in range(1,6):
            degree=2*size-1;epochs=[-.8,-.23,.14,.41,.92][:size];states=[polynomial(t,degree)for t in epochs]
            for t in [(-.71+i*.067)for i in range(23)]+epochs:
                with self.subTest(degree=degree,t=t):
                    actual=oracle.hermite(epochs,states,t);expected=polynomial(t,degree)
                    self.assertLess(max(abs(a-b)for a,b in zip(actual,expected)),2e-9)

    def test_large_epoch_offset_and_derivative_units(self):
        base=392558466.;scale=864.;offsets=[0.,13.,37.,61.,108.]
        epochs=[base+t for t in offsets];states=[]
        for t in offsets:
            state=polynomial(t/scale,7);states.append(state[:3]+[v/scale for v in state[3:]])
        for t in [1.5,25.5,50.5,90.5]:
            expected=polynomial(t/scale,7);expected=expected[:3]+[v/scale for v in expected[3:]]
            actual=oracle.hermite(epochs,states,base+t)
            self.assertLess(max(abs(a-b)for a,b in zip(actual,expected)),1e-12)

    def test_centered_windows_and_later_segment_priority(self):
        epochs=[0,10,21,33,49,66,88]
        self.assertEqual([oracle.window_start(epochs,4,t)for t in [0,27,88]],[0,1,3])
        self.assertEqual([oracle.window_start(epochs,3,t)for t in [0,33,88]],[0,2,4])
        self.assertEqual([oracle.window_start(epochs,1,t)for t in [12,19]],[1,2])
        def segment(first,offset):
            times=[first+i for i in range(4)]
            return dict(target=-100056,center=399,frame=1,window=4,first=times[0],last=times[-1],epochs=times,states=[[offset+t,2*t,3*t,1,2,3]for t in times])
        self.assertEqual(oracle.evaluate([segment(0,0),segment(2,10)],2),[12,4,6,1,2,3])
        with self.assertRaises(ValueError):oracle.evaluate([segment(0,0)],4)
        with self.assertRaises(ValueError):oracle.evaluate([segment(0,0)],1,target=-1)

    def test_both_daf_encodings(self):
        for order in ['<','>']:
            segments=oracle.read_segments(kernel(order));self.assertEqual(segments[0]['degree'],9)
            for t in [-.5,.01,.3,.8]:
                actual=oracle.evaluate(segments,t);wanted=polynomial(t,9)
                self.assertLess(max(abs(a-b)for a,b in zip(actual,wanted)),1e-10)

    def test_header_chain_and_nonfinite_data_rejected(self):
        original=kernel();cycle=bytearray(original);struct.pack_into('<d',cycle,1024,2.)
        nonfinite=bytearray(original);struct.pack_into('<d',nonfinite,3072,float('nan'))
        kind=bytearray(original);struct.pack_into('<i',kind,1076,9)
        for changed in [original[:-8],bytes(cycle),bytes(nonfinite),bytes(kind)]:
            with self.assertRaises(ValueError):oracle.read_segments(changed)
        with self.assertRaises(ValueError):oracle.hermite([0,0],[[0]*6]*2,.2)

    def test_complete_retained_spice_queries(self):
        for case,raw,segments,queries in self.cases:
            result=oracle.compare(segments,queries)
            self.assertEqual(result['queries'],2869);self.assertEqual(result['components'],17214)
            self.assertEqual(case['knots'],1435);self.assertEqual(case['midpoints'],1434)
            self.assertFalse(result['physical_trajectory_qualified'])
            RESULTS.append(dict(case=case['case'],**result))

    def test_existing_batch_decoder_entrypoint_runs_encoded_check(self):
        import BatchSpkOracle
        _,raw,_,queries=self.cases[0]
        decoded=dict(reader_samples=[dict(et=q[0],state=q[1:])for q in queries])
        result=BatchSpkOracle.validate_encoded_curve(raw,decoded)
        self.assertEqual(result['queries'],2869)
        bad=copy.deepcopy(decoded);bad['reader_samples'][1]['state'][3]+=.001
        with self.assertRaises(ValueError):BatchSpkOracle.validate_encoded_curve(raw,bad)

    def test_wrong_query_units_time_and_velocity_rejected(self):
        _,_,segments,queries=self.cases[0]
        for field,change in [(1,1000.),(4,1e-7),(0,.5)]:
            query=queries[1].copy()
            if field==1:query[1:]=[v*change for v in query[1:]]
            else:query[field]+=change
            with self.assertRaises(ValueError):oracle.compare(segments,[query])
        with self.assertRaises(ValueError):oracle.compare(segments,[[float('nan')]+[0]*6])
        with self.assertRaises(ValueError):oracle.compare(segments,[[queries[0][0]]+[0]*5])

    def test_raw_directory_and_midpoint_state_mutations_rejected(self):
        _,raw,_,queries=self.cases[0]
        directory=bytearray(raw);record=struct.unpack_from('<i',raw,76)[0];descriptor=(record-1)*1024+24;fields=struct.unpack_from('<6i',raw,descriptor+16);start=(fields[4]-1)*8
        n=int(struct.unpack_from('<d',raw,(fields[5]-1)*8)[0]);struct.pack_into('<d',directory,start+7*n*8,0.)
        with self.assertRaises(ValueError):oracle.read_segments(bytes(directory))
        changed=bytearray(raw);value=struct.unpack_from('<d',raw,start+6*8)[0];struct.pack_into('<d',changed,start+6*8,value+.001)
        segments=oracle.read_segments(bytes(changed))
        with self.assertRaises(ValueError):oracle.compare(segments,[queries[1]])

if __name__=='__main__':
    suite=unittest.defaultTestLoader.loadTestsFromTestCase(Contract);run=unittest.TextTestRunner(verbosity=2).run(suite)
    if run.wasSuccessful():
        proof=dict(contract='independent-SPK-type13-Hermite',diagnostic_only=True,tests=run.testsRun,corpus_results=RESULTS,corpus_queries=sum(r['queries']for r in RESULTS),corpus_components=sum(r['components']for r in RESULTS),gui_replays=0,physical_gate_closed=False)
        directory=os.environ.get('GMAT_QT_ACCEPTANCE_DIR')
        if directory:Path(directory,'support-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
        print('SPK_HERMITE_CHECK '+json.dumps(proof,separators=(',',':')))
    sys.exit(0 if run.wasSuccessful()else 1)
