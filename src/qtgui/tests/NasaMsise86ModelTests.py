"""Compare the optional MSIS-86 plugin with NASA's published ten-case table.

Check every density/species/temperature at the precision published by NASA,
then repeat out of order and interleave MSISE90 to check symbol isolation.
"""
import argparse
import ctypes as c
import json
import math
from pathlib import Path
import re

LABELS=['HE','O','N2','O2','AR','RHO','H','N','TINF','TG']

def expected(path):
    text=path.read_text();result={}
    for label in LABELS:
        rows=re.findall(r'^ '+label+r'\s+([^\n]+)',text,re.M)
        assert len(rows)==2
        result[label]=[float(v) for row in rows for v in row.split()]
        assert len(result[label])==10
    return result


def inputs():
    result=[]
    for i in range(10):
        result.append([81 if i==1 else 172,75000 if i==2 else 29000,100 if i==3 else 400,
                       0 if i==4 else 60,0 if i==5 else -70,4 if i==6 else 16,
                       70 if i==7 else 150,180 if i==8 else 150,40 if i==9 else 4])
    return result


def evaluate(function,row):
    date=c.c_int(row[0]);values=[c.c_float(v) for v in row[1:8]];ap=(c.c_float*7)(*[row[8]]*7);mass=c.c_int(48);d=(c.c_float*8)();t=(c.c_float*2)()
    function(c.byref(date),*[c.byref(v) for v in values],ap,c.byref(mass),d,t)
    return list(d)+list(t)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--plugin',type=Path,required=True);p.add_argument('--base',type=Path,required=True);p.add_argument('--reference',type=Path,required=True);p.add_argument('--output',type=Path);a=p.parse_args()
    base=c.CDLL(str(a.base.resolve()),mode=c.RTLD_GLOBAL);m90=base.gtd6_;baseline=[evaluate(m90,row) for row in inputs()]
    plugin=c.CDLL(str(a.plugin.resolve()));model=plugin.gmat_m86_gts5_;truth=expected(a.reference);records=[]
    for pass_name,order in [('published',range(10)),('reverse',reversed(range(10))),('interleaved',[4,0,9,1,7,2,8,3,6,5])]:
        for i in order:
            value=evaluate(model,inputs()[i]);assert all(math.isfinite(x) for x in value)
            for label,x in zip(LABELS,value):
                printed=float(format(x,'.2f' if label in ['TINF','TG'] else '.3e'))
                assert printed==truth[label][i],(pass_name,i,label,x,printed,truth[label][i])
            assert evaluate(m90,inputs()[i])==baseline[i],('MSISE90 changed',pass_name,i)
            records.append(dict(phase=pass_name,case=i,values=value))
    result=dict(status='passed',published_cases=10,published_values=100,compared_values=300,msise90_isolation_checks=30,records=records)
    if a.output:a.output.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS MSISE86: 100 published values; 300 comparisons; 30 interleaved MSISE90 checks')

if __name__=='__main__':main()
