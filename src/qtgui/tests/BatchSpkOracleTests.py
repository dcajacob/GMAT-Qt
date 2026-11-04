"""DAF, clock, frame and scientific negative controls for batch SPK decoding."""
import copy,json,math,struct,unittest
from pathlib import Path
import BatchSpkOracle as oracle

ROOT=Path(__file__).resolve().parents[3]
FIXTURE=json.loads(Path(__file__).with_name('NasaBatchSpkCases.json').read_text())
LSK=(ROOT/'application/data/time/SPICELeapSecondKernel.tls').read_text()
TABLE=[list(map(float,l.split(',')))for l in(ROOT/'application/data/icrf/ICRF_Table.txt').read_text().splitlines()]
OBS='\n'.join(f'{FIXTURE["utc_mjd"]+(i*28680/357+34)/86400:.16f} Range_Skin 1 A B 1'for i in range(358))

def sample(order='<'):
    tt0,et,_=oracle.clock(dict(utc=FIXTURE['epoch_utc']),LSK)
    native=[];times=[];states=[];epochs=[]
    for i in range(1435):
        a1=FIXTURE['a1_epoch']+i*20/86400;t=(a1-FIXTURE['a1_epoch'])*86400;state=oracle.circle(t,FIXTURE)
        native.append([a1]+state);times.append(t);epochs.append(et(t));states.append(oracle.frame(TABLE,a1,state)[0])
    words=[];descriptors=[];segments=[];knots=[]
    for at,indices in enumerate([list(range(1000)),list(range(999,1435))]):
        n=len(indices);values=[v for i in indices for v in states[i]]+[epochs[i]for i in indices]
        values += [epochs[indices[(j+1)*100-1]]for j in range((n-1)//100)]+[3.,float(n)]
        start=385+len(words);end=start+len(values)-1;words+=values;descriptors.append((epochs[indices[0]],epochs[indices[-1]],-100056,399,1,13,start,end))
        segments.append(dict(target=-100056,center=399,frame=1,type=13,degree=7,count=n,first=epochs[indices[0]],last=epochs[indices[-1]],epochs=[epochs[i]for i in indices]))
        knots += [dict(segment=at,native_index=i,et=epochs[i],expected_et=epochs[i],elapsed=times[i],state=states[i])for i in indices]
    size=math.ceil((3072+len(words)*8)/1024)*1024;raw=bytearray(size);raw[:8]=b'DAF/SPK ';raw[88:96]=b'LTL-IEEE'if order=='<'else b'BIG-IEEE';struct.pack_into(order+'2i',raw,8,2,6);struct.pack_into(order+'3i',raw,76,2,2,385+len(words));struct.pack_into(order+'3d',raw,1024,0,0,2)
    for i,desc in enumerate(descriptors):struct.pack_into(order+'2d6i',raw,1048+40*i,*desc)
    struct.pack_into(order+str(len(words))+'d',raw,3072,*words)
    queries=[]
    for i in range(2869):
        at=i//2;t=(times[at]+times[at+1])/2 if i%2 else times[at];epoch=(epochs[at]+epochs[at+1])/2 if i%2 else epochs[at]
        state=oracle.frame(TABLE,FIXTURE['a1_epoch']+t/86400,oracle.circle(t,FIXTURE))[0]
        queries.append(dict(kind='midpoint'if i%2 else'knot',elapsed=t,et=epoch,state=state))
    observed_last=(float(OBS.splitlines()[-1].split()[0])-FIXTURE['utc_mjd'])*86400-FIXTURE['tai_minus_utc']
    decoded=dict(case='Positive',tt0=tt0,et0=et(0),segments=segments,knots=knots,native_rows=native,reader_samples=queries,last_observation=observed_last,observations=358,unique_knots=1435,boundary_overlaps=1)
    for k in ['max_clock_error_seconds','max_raw_state_error','max_position_error_km','max_velocity_error_km_s','max_query_position_error_km','max_query_velocity_error_km_s']:decoded[k]=0.
    text='\n'.join(','.join(map(str,row))for row in native)
    return bytes(raw),text,decoded

class Contract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.raw,cls.native,cls.decoded=sample()
    def validate(self,raw=None,native=None,decoded=None,table=None,lsk=None):
        return oracle.validate(FIXTURE,'Positive',self.raw if raw is None else raw,self.native if native is None else native,OBS,LSK if lsk is None else lsk,TABLE if table is None else table,self.decoded if decoded is None else decoded)
    def test_valid_two_segments_and_both_byte_orders(self):
        self.assertEqual(self.validate()['metrics']['raw_knots'],1436)
        raw,native,decoded=sample('>');self.assertEqual(self.validate(raw,native,decoded)['metrics']['boundary_overlaps'],1)
    def test_wrong_target_and_truncation_rejected(self):
        raw=bytearray(self.raw);struct.pack_into('<i',raw,1064,-100055)
        for bad in [bytes(raw),self.raw[:-8]]:
            with self.assertRaises(AssertionError):self.validate(raw=bad)
    def test_corrupt_directory_rejected(self):
        raw=bytearray(self.raw);struct.pack_into('<d',raw,3072+7000*8,0.)
        with self.assertRaises(AssertionError):self.validate(raw=bytes(raw))
    def test_coherent_wrong_clock_rejected(self):
        raw=bytearray(self.raw);decoded=copy.deepcopy(self.decoded)
        for at,segment in enumerate(oracle.segments(self.raw)):
            desc=1048+40*at;ic=struct.unpack_from('<6i',self.raw,desc+16);data=(ic[4]-1)*8;n=segment['count']
            epochs=[t+1 for t in segment['epochs']];struct.pack_into('<2d',raw,desc,epochs[0],epochs[-1]);struct.pack_into('<'+str(n)+'d',raw,data+6*n*8,*epochs)
            for d in range((n-1)//100):struct.pack_into('<d',raw,data+(7*n+d)*8,epochs[(d+1)*100-1])
            decoded['segments'][at].update(first=epochs[0],last=epochs[-1],epochs=epochs)
        for k in decoded['knots']:k['et']+=1
        for q in decoded['reader_samples']:q['et']+=1
        with self.assertRaises(AssertionError):self.validate(raw=bytes(raw),decoded=decoded)
    def test_coherent_intermediate_orbit_rejected(self):
        raw=bytearray(self.raw);decoded=copy.deepcopy(self.decoded);k=900;value=decoded['knots'][k]['state'][0]+.001;struct.pack_into('<d',raw,3072+6*k*8,value);decoded['knots'][k]['state'][0]=value
        with self.assertRaises(AssertionError):self.validate(raw=bytes(raw),decoded=decoded)
    def test_mismatched_boundary_overlap_rejected(self):
        raw=bytearray(self.raw);ic=struct.unpack_from('<6i',raw,1088+16);struct.pack_into('<d',raw,(ic[4]-1)*8,0.)
        with self.assertRaises(AssertionError):self.validate(raw=bytes(raw))
    def test_native_grid_and_reader_midpoint_rejected(self):
        with self.assertRaises(AssertionError):self.validate(native='\n'.join(self.native.splitlines()[:-1]))
        decoded=copy.deepcopy(self.decoded);decoded['reader_samples'][1]['state'][0]+=.001
        with self.assertRaises(AssertionError):self.validate(decoded=decoded)
    def test_shared_frame_definition_mutation_rejected(self):
        table=copy.deepcopy(TABLE)
        for row in table:row[1]+=.001
        with self.assertRaises(AssertionError):self.validate(table=table)

if __name__=='__main__':unittest.main()
