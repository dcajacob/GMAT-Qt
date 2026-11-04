"""Direct DAF decoder, LSK clock algebra and frame/circle checks; no GMAT imports."""
from pathlib import Path
import datetime,hashlib,json,math,re,struct,sys
from EclipseGeometryOracle import dot,cross,norm

def clock(spec,lsk):
    data=lsk.split('\\begindata',1)[1]
    def value(name):
        match=re.search(r'DELTET/'+name+r'\s*=\s*([^\n]+)',data);assert match
        return [float(v.replace('D','e')) for v in re.findall(r'[-+]?\d+(?:\.\d*)?(?:[DE][-+]?\d+)?',match[1])]
    delta=value('DELTA_T_A')[0];k=value('K')[0];eb=value('EB')[0];m0,m1=value('M')
    months={m:i+1 for i,m in enumerate(['JAN','FEB','MAR','APR','MAY','JUN','JUL','AUG','SEP','OCT','NOV','DEC'])}
    fields=spec['utc'].split();day,month,year=int(fields[0]),months[fields[1].upper()],int(fields[2]);hh,mm,ss=map(float,fields[3].split(':'))
    start=datetime.datetime(year,month,day,int(hh),int(mm),int(ss),round((ss%1)*1e6))
    leaps=[(datetime.datetime(int(y),months[m],int(d)),int(offset))for offset,y,m,d in re.findall(r'(\d+),\s*@(\d+)-([A-Z]+)-(\d+)',data)]
    tai=max((v for v in leaps if v[0]<=start),key=lambda v:v[0])[1]
    tt0=(start-datetime.datetime(2000,1,1,12)).total_seconds()+tai+delta
    def et(elapsed):
        tt=tt0+elapsed;result=tt
        for _ in range(6):
            m=m0+m1*result;result=tt+k*math.sin(m+eb*math.sin(m))
        return result
    return tt0,et,dict(tai_minus_utc=tai,tt_minus_tai=delta,k=k,eb=eb,m0=m0,m1=m1)

def segments(raw):
    assert raw[:8]==b'DAF/SPK 'and len(raw)%1024==0
    endian='<'if raw[88:96]==b'LTL-IEEE'else'>'if raw[88:96]==b'BIG-IEEE'else None;assert endian
    nd,ni=struct.unpack_from(endian+'2i',raw,8);assert(nd,ni)==(2,6)
    record=struct.unpack_from(endian+'i',raw,76)[0];seen=set();result=[]
    while record:
        assert record not in seen and record>0;seen.add(record);offset=(record-1)*1024
        nxt,prev,count=struct.unpack_from(endian+'3d',raw,offset);assert count==int(count)and 0<count<=25
        for i in range(int(count)):
            pos=offset+24+i*40;dc=struct.unpack_from(endian+'2d',raw,pos);ic=struct.unpack_from(endian+'6i',raw,pos+16)
            assert ic[:4]==(-100056,399,1,13)and 1<=ic[4]<=ic[5]<=len(raw)//8
            words=struct.unpack_from(endian+str(ic[5]-ic[4]+1)+'d',raw,(ic[4]-1)*8);n=int(words[-1]);window=int(words[-2])+1
            assert words[-1]==n and words[-2]==window-1 and 8<=n<=10000 and len(words)==7*n+(n-1)//100+2
            states=[list(words[i*6:i*6+6])for i in range(n)];epochs=list(words[6*n:7*n])
            assert all(math.isfinite(x)for row in states for x in row)and all(math.isfinite(x)for x in epochs)
            assert all(y>x for x,y in zip(epochs,epochs[1:]))and dc[0]==epochs[0]and dc[1]==epochs[-1]
            assert list(words[7*n:-2])==[epochs[(i+1)*100-1]for i in range((n-1)//100)]
            result.append(dict(target=ic[0],center=ic[1],frame=ic[2],type=ic[3],degree=2*window-1,count=n,first=dc[0],last=dc[1],states=states,epochs=epochs))
        assert nxt==int(nxt);record=int(nxt)
    return result

def frame(table,epoch,vector):
    lo,hi=0,len(table)-1;mid=None
    while lo<hi-1:
        mid=(lo+hi)//2
        if epoch>table[mid][0]:lo=mid
        else:hi=mid
    assert mid is not None and table[0][0]<=epoch<=table[-1][0]
    first=max(0,mid-4);last=min(len(table)-1,first+9);first=max(0,last-9);rows=table[first:last+1];assert len(rows)==10
    weights=[math.prod((epoch-other[0])/(row[0]-other[0])for other in rows if other is not row)for row in rows]
    rv=[math.fsum(w*r[j+1]for w,r in zip(weights,rows))for j in range(3)];angle=norm(rv);assert angle>0
    axis=[v/angle for v in rv];out=[]
    for v in [vector[:3],vector[3:]]:
        c=cross(axis,v);d=dot(axis,v);out.extend(math.cos(angle)*v[j]-math.sin(angle)*c[j]+(1-math.cos(angle))*d*axis[j]for j in range(3))
    return out,dict(a1_epoch=epoch,first_index=first,samples=rows,rotation_vector=rv)

def circle(t,fixture):
    radius,mu=fixture['radius'],fixture['mu'];rate=math.sqrt(mu/radius**3);speed=math.sqrt(mu/radius);c=math.sqrt(.5)
    return[radius*math.cos(rate*t),radius*math.sin(rate*t)*c,radius*math.sin(rate*t)*c,-speed*math.sin(rate*t),speed*math.cos(rate*t)*c,speed*math.cos(rate*t)*c]

def validate(fixture,key,raw,native,observations,lsk,table,decoded):
    data=segments(raw);assert len(data)==2 and[s['count']for s in data]==[1000,436]and all(s['degree']==7 for s in data)
    rows=[]
    for line in native.strip().splitlines():
        row=list(map(float,line.split(',')));assert len(row)==7 and all(map(math.isfinite,row))
        if rows and abs(row[0]-rows[-1][0])<1e-12:assert max(abs(a-b)for a,b in zip(row[1:],rows[-1][1:]))<1e-8
        else:rows.append(row)
    assert len(rows)==1435 and rows==decoded['native_rows']and decoded['case']==key
    times=[(r[0]-fixture['a1_epoch'])*86400 for r in rows]
    assert all(abs(t-i*20)<.002 for i,t in enumerate(times))and all(b>a for a,b in zip(times,times[1:]))
    obs=[]
    for line in observations.splitlines():
        if not line.strip()or line.lstrip().startswith('%'):continue
        fields=line.split();assert len(fields)==6 and fields[1]=='Range_Skin'
        obs.append((float(fields[0])-fixture['utc_mjd'])*86400-fixture['tai_minus_utc'])
    assert len(obs)==358 and abs(times[-1]-max(obs))<.002 and abs(decoded['last_observation']-max(obs))<1e-9
    tt0,et,constants=clock(dict(utc=fixture['epoch_utc']),lsk)
    assert abs(decoded['tt0']-tt0)<5e-7 and abs(decoded['et0']-et(0))<5e-7 and constants['tai_minus_utc']==fixture['tai_minus_utc']
    assert len(decoded['segments'])==2 and len(decoded['knots'])==1436
    metrics=dict(raw_knots=1436,unique_knots=1435,segments=2,boundary_overlaps=1,reader_queries=2869,observations=358,max_clock_error_seconds=0.,max_raw_state_error=0.,max_position_error_km=0.,max_velocity_error_km_s=0.,max_query_position_error_km=0.,max_query_velocity_error_km_s=0.)
    index=-1;last=-math.inf;raw_index=0;overlaps=0;unique_epochs=[]
    for segment_no,segment in enumerate(data):
        for k,v in segment.items():
            if k!='states':assert decoded['segments'][segment_no][k]==v
        for i,(epoch,state)in enumerate(zip(segment['epochs'],segment['states'])):
            overlap=abs(epoch-last)<5e-7
            if overlap:
                assert segment_no==1 and i==0 and max(abs(a-b)for a,b in zip(state,decoded['knots'][raw_index-1]['state']))<1e-8;overlaps+=1
            else:index+=1;unique_epochs.append(epoch)
            assert epoch>=last-5e-7 and index<1435;last=epoch
            row=rows[index];t=times[index];expected=et(t);err=abs(epoch-expected);assert err<5e-5
            metrics['max_clock_error_seconds']=max(metrics['max_clock_error_seconds'],err)
            knot=decoded['knots'][raw_index];assert knot['segment']==segment_no and knot['native_index']==index and knot['et']==epoch and knot['elapsed']==t and knot['state']==state and abs(knot['expected_et']-expected)<5e-7
            native_frame,_=frame(table,row[0],row[1:]);analytic,_=frame(table,row[0],circle(t,fixture));rawerror=max(abs(a-b)for a,b in zip(state,native_frame));assert rawerror<1e-8;metrics['max_raw_state_error']=max(metrics['max_raw_state_error'],rawerror)
            for j,(a,b)in enumerate(zip(state,analytic)):
                err=abs(a-b);name='max_position_error_km'if j<3 else'max_velocity_error_km_s';assert err<fixture['position_tolerance'if j<3 else'velocity_tolerance'];metrics[name]=max(metrics[name],err)
            raw_index+=1
    assert raw_index==1436 and index==1434 and overlaps==1 and len(unique_epochs)==1435 and decoded['unique_knots']==1435 and decoded['boundary_overlaps']==1 and decoded['observations']==358
    assert len(decoded['reader_samples'])==2869
    for i,query in enumerate(decoded['reader_samples']):
        at=i//2;t=(times[at]+times[at+1])/2 if i%2 else times[at];epoch=(unique_epochs[at]+unique_epochs[at+1])/2 if i%2 else unique_epochs[at]
        assert query['kind']==('midpoint'if i%2 else'knot')and query['elapsed']==t and query['et']==epoch and len(query['state'])==6 and abs(epoch-et(t))<5e-5
        analytic,_=frame(table,fixture['a1_epoch']+t/86400,circle(t,fixture))
        for j,(a,b)in enumerate(zip(query['state'],analytic)):
            err=abs(a-b);assert err<fixture['position_tolerance'if j<3 else'velocity_tolerance'];name='max_query_position_error_km'if j<3 else'max_query_velocity_error_km_s';metrics[name]=max(metrics[name],err)
    for k in ['max_clock_error_seconds','max_raw_state_error','max_position_error_km','max_velocity_error_km_s','max_query_position_error_km','max_query_velocity_error_km_s']:assert abs(decoded[k]-metrics[k])<5e-10
    return dict(metrics=metrics,clock_constants=constants,independent_daf_decoder=True,independent_lsk_clock_algebra=True,independent_rodrigues_and_circle=True,shared_frame_lsk_and_native_measurements=True)

def validate_encoded_curve(raw,decoded):
    # A separate reader-contract result. The strict physical assertions in
    # validate() below/above remain mandatory and are not replaced by this.
    import SpkHermiteOracle
    queries=[[q['et']]+q['state']for q in decoded['reader_samples']]
    return SpkHermiteOracle.compare(SpkHermiteOracle.read_segments(raw),queries)

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser()
    for name in ['fixture','case','file','native','observations','lsk','table','decoded','output']:parser.add_argument('--'+name,required=True)
    args=parser.parse_args();fixture=json.loads(Path(args.fixture).read_text());decoded=json.loads(Path(args.decoded).read_text());table_path=Path(args.table);assert hashlib.sha256(table_path.read_bytes()).hexdigest()==decoded['frame_table_sha256'];table=[list(map(float,l.split(',')))for l in table_path.read_text().splitlines()]
    encoded=validate_encoded_curve(Path(args.file).read_bytes(),decoded);print('SPK_ENCODED_CURVE_CHECK '+json.dumps(encoded),flush=True)
    result=validate(fixture,args.case,Path(args.file).read_bytes(),Path(args.native).read_text(),Path(args.observations).read_text(),Path(args.lsk).read_text(),table,decoded);Path(args.output).write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
