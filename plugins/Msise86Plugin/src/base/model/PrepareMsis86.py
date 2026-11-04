"""Isolate the original NASA MSIS-86 symbols and embed its exact coefficients.

All scientific routines remain unchanged apart from symbol names. PRMSG5's
working-directory-dependent file I/O becomes literal initialization from the
same fixed-format data. The original source/data remain separately inspectable.
"""
import argparse
from pathlib import Path
import re


def generate(source, data):
    text=source.read_text()
    # Only externally linked procedures, ENTRY points and COMMON blocks.
    names=['GTS5','METERS','DENSS','GLOBE5','TSELEC','TRETRV','GLOB5L','DNET','CCOR','PRMSG5','GGM',
           'UINR','GTS3C','LOWER5','PARM5','CSW','TTEST','DATIME','PARMB','FIT','LSQV','LPOLY']
    start=text.index('      SUBROUTINE PRMSG5');end=text.index('      SUBROUTINE GGM',start)
    values=[]
    for line in data.read_text().splitlines():
        for offset in range(1,len(line),13):
            value=line[offset:offset+13].strip()
            if value:float(value);values.append(value)
    assert len(values)==1464
    replacement='''      SUBROUTINE PRMSG5
C     Exact NASA coefficients embedded; no process-directory or I/O dependency.
      COMMON/UINR/IUMSIS,MONITO,IIEE
      COMMON/PARMB/GSURF,RE
      COMMON/PARM5/VALUES(1400)
      REAL LOWER
      COMMON/LOWER5/LOWER(64)
      GSURF=980.665
      RE=6356.77
      IIEE=0
'''
    replacement+=''.join('      '+('VALUES' if i<1400 else 'LOWER')+f'({i+1 if i<1400 else i-1399})={v}\n' for i,v in enumerate(values))
    replacement+='      END\nC--------------------------------------------------------------------\n'
    text=text[:start]+replacement+text[end:]
    # The historical (1) idiom means assumed-size arrays, not one element.
    text=text.replace('DIMENSION SV(1),SAV(25),SVV(1)', 'DIMENSION SV(*),SAV(25),SVV(*)')
    pattern=re.compile(r'\b('+'|'.join(names)+r')\b',re.I)
    return '\n'.join(line if not line or line[0] in 'Cc*!' else pattern.sub(lambda m:'GMAT_M86_'+m.group().upper(),line) for line in text.splitlines())+'\n'


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('data',type=Path);p.add_argument('output',type=Path);a=p.parse_args();a.output.write_text(generate(a.source,a.data))
