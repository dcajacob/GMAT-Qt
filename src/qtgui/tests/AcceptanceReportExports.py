"""Export recorded acceptance catalogues/comparisons as genuine XLS, TeX or HTML.

XLS uses LibreOffice's Excel97 BIFF8 filter in an isolated headless profile.
There is no GUI/engine replay and no recomputation or promotion of scope credit.
Counts remain numeric. Decimal comparison strings keep their exact precision.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET
import zipfile

import AcceptanceCompare as compare

NS={k:'urn:oasis:names:tc:opendocument:xmlns:'+v+':1.0' for k,v in
    [('office','office'),('table','table'),('text','text'),('style','style')]}
NS['fo']='urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0'
NS['number']='urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0'
for prefix,url in NS.items():ET.register_namespace(prefix,url)
def tag(prefix,name):return '{'+NS[prefix]+'}'+name

def text(value):
    if value is None:return ''
    if isinstance(value,str):return value
    if isinstance(value,(dict,list)):return json.dumps(value,ensure_ascii=False,allow_nan=False,sort_keys=True)
    return str(value)

def tables(document,kind,source):
    """One common, explicit record view in every format; all extra fields survive."""
    if kind=='catalogue':
        compare.render_catalogue(document)  # Existing required catalogue schema.
        summary=[['Recorded acceptance catalogue'],['Source',str(source.resolve())],['SHA256',hashlib.sha256(source.read_bytes()).hexdigest()],['Scope',document.get('scope','Supplied recorded evidence; no new execution.')],[] ,['Metric','Recorded value']]
        summary += [[k,v if isinstance(v,(int,float)) and not isinstance(v,bool) else text(v)] for k,v in document['summary'].items()]
        tests=[['Test','Status','Execution','Type','Platform','Requirements','Recorded details']]
        tests += [[r['name'],r['status'],r['execution'],r['test_type'],r['platform'],text(r['requirements']),text({k:v for k,v in r.items() if k not in ('name','status','execution','test_type','platform','requirements')})] for r in document['tests']]
        scopes=[['Source key','Kind','Declared ID','Requirement','Evidence state','Tests','Recorded details']]
        scopes += [[r['key'],r['kind'],r.get('declared_id',''),r['name'],r['state'],text(r['tests']),text({k:v for k,v in r.items() if k not in ('key','kind','declared_id','name','state','tests')})] for r in document['scopes']]
        return [('Summary',summary),('Tests',tests),('Scopes',scopes)]
    compare.need(kind=='comparison','Unknown report kind');compare.render_comparison(document)
    summary=[['Recorded acceptance comparison'],['Source',str(source.resolve())],['SHA256',hashlib.sha256(source.read_bytes()).hexdigest()],['Status',document['status']],['Comparator',document['kind']],['Tolerance policy',text(document.get('policy',{}))],['Numeric fields',document.get('compared_fields',0)],['Exceeding fields',document.get('exceeded_fields',0)]]
    summary += [[role+' input',text(value)] for role,value in document['inputs'].items()]
    numeric=[['Location','Field','Unit','Actual (exact text)','Truth (exact text)','Difference (exact text)','Allowed (exact text)','Exceeds']]
    numeric += [[r['location'],r['field'],r.get('unit',''),r['actual'],r['truth'],r['delta'],r['allowed'],str(r['exceeded'])] for r in document['comparisons']]
    differences=[['Location','Field','Actual','Truth']]+[[r['location'],r['field'],text(r['actual']),text(r['truth'])] for r in document['differences']]
    errors=[['Input/comparator errors']]+[[v] for v in document['errors']]
    return [('Summary',summary),('Numeric',numeric),('Differences',differences),('Errors',errors)]

def validate_tables(sheets):
    compare.need(sheets and len({name for name,_ in sheets})==len(sheets),'Empty/duplicate report sheets')
    for name,rows in sheets:
        compare.need(rows and len(rows)<=65536 and max(map(len,rows))<=256,'Excel97 row/column limit exceeded; report is not truncated')
        compare.need(len(name)<=31 and not any(c in name for c in '[]:*?/\\'),'Invalid Excel sheet name')
        for row in rows:
            for value in row:
                if isinstance(value,(int,float)) and not isinstance(value,bool):compare.need(math.isfinite(value),'Nonfinite report count')
                value=text(value)
                compare.need(len(value.encode('utf-16-le'))//2<=32767,'Excel97 cell limit exceeded; report is not truncated')
                compare.need(all(c in '\t\n\r' or ord(c)>=32 for c in value),'Invalid report control character')

def ods(sheets,path):
    validate_tables(sheets)
    root=ET.Element(tag('office','document-content'),{tag('office','version'):'1.2'})
    styles=ET.SubElement(root,tag('office','automatic-styles'))
    number=ET.SubElement(styles,tag('number','text-style'),{tag('style','name'):'ExactText'})
    ET.SubElement(number,tag('number','text-content'))
    for name,width in [('label','1.8in'),('detail','3.4in')]:
        style=ET.SubElement(styles,tag('style','style'),{tag('style','name'):name,tag('style','family'):'table-column'})
        ET.SubElement(style,tag('style','table-column-properties'),{tag('style','column-width'):width})
    # Automatic styles cannot serve as reliable parent styles in the Excel97
    # conversion. Give every concrete cell style its own required properties.
    for name in ('body','text','header'):
        attrs={tag('style','name'):name,tag('style','family'):'table-cell'}
        if name!='body':attrs[tag('style','data-style-name')]='ExactText'
        style=ET.SubElement(styles,tag('style','style'),attrs)
        cell={tag('fo','wrap-option'):'wrap',tag('style','vertical-align'):'middle'}
        font={tag('fo','font-size'):'10pt',tag('fo','font-family'):'Arial'}
        if name=='header':cell[tag('fo','background-color')]='#e8edf5';font[tag('fo','font-weight')]='bold'
        ET.SubElement(style,tag('style','table-cell-properties'),cell)
        ET.SubElement(style,tag('style','text-properties'),font)
    body=ET.SubElement(ET.SubElement(root,tag('office','body')),tag('office','spreadsheet'))
    for name,rows in sheets:
        table=ET.SubElement(body,tag('table','table'),{tag('table','name'):name})
        for i in range(max(map(len,rows))):ET.SubElement(table,tag('table','table-column'),{tag('table','style-name'):'detail' if i in (1,3,5,6) else 'label'})
        for index,row in enumerate(rows):
            line=ET.SubElement(table,tag('table','table-row'))
            for value in row:
                numeric=isinstance(value,(int,float)) and not isinstance(value,bool)
                attrs={tag('office','value-type'):'float' if numeric else 'string',tag('table','style-name'):'header' if index==0 else 'body' if numeric else 'text'}
                if numeric:attrs[tag('office','value')]=str(value)
                cell=ET.SubElement(line,tag('table','table-cell'),attrs)
                # Explicit string values never become formulas or hyperlinks.
                for paragraph in text(value).split('\n'):
                    p=ET.SubElement(cell,tag('text','p'));last=None
                    for part in re.split(r'([ \t]+)',paragraph):
                        if part.startswith((' ','\t')):
                            for spaces in re.findall(r' +|\t',part):
                                last=ET.SubElement(p,tag('text','tab') if spaces=='\t' else tag('text','s'),{} if spaces=='\t' else {tag('text','c'):str(len(spaces))})
                        elif last is None:p.text=(p.text or '')+part
                        else:last.tail=(last.tail or '')+part
    with zipfile.ZipFile(path,'w') as z:
        z.writestr('mimetype','application/vnd.oasis.opendocument.spreadsheet',compress_type=zipfile.ZIP_STORED)
        z.writestr('content.xml',ET.tostring(root,encoding='utf-8',xml_declaration=True))
        z.writestr('META-INF/manifest.xml','<?xml version="1.0"?><manifest:manifest xmlns:manifest="urn:oasis:names:tc:opendocument:xmlns:manifest:1.0" manifest:version="1.2"><manifest:file-entry manifest:full-path="/" manifest:media-type="application/vnd.oasis.opendocument.spreadsheet"/><manifest:file-entry manifest:full-path="content.xml" manifest:media-type="text/xml"/></manifest:manifest>')

def write_xls(sheets,path,libreoffice='libreoffice'):
    compare.need(shutil.which(libreoffice) is not None,'XLS export requires LibreOffice (Excel97 filter)')
    with tempfile.TemporaryDirectory(prefix='gmat-report-xls-') as folder:
        work=Path(folder);source=work/'recorded.ods';ods(sheets,source);out=work/'out';out.mkdir()
        command=[libreoffice,'-env:UserInstallation='+(work/'profile').as_uri(),'--headless','--nologo','--nodefault','--nolockcheck','--convert-to','xls:MS Excel 97','--outdir',str(out),str(source)]
        completed=subprocess.run(command,env=dict(os.environ,GSETTINGS_BACKEND='memory',SAL_USE_VCLPLUGIN='svp'),capture_output=True,text=True,timeout=45)
        target=out/'recorded.xls'
        compare.need(completed.returncode==0 and target.is_file(),'Excel97 conversion failed: '+completed.stdout+completed.stderr)
        data=target.read_bytes();compare.need(data.startswith(bytes.fromhex('d0cf11e0a1b11ae1')),'XLS converter did not produce a binary compound workbook')
        write_bytes(path,data)

def escape_tex(value):
    escapes={'\\':r'\textbackslash{}','{':r'\{','}':r'\}','$':r'\$','&':r'\&','#':r'\#','_':r'\_','%':r'\%','~':r'\textasciitilde{}','^':r'\textasciicircum{}'}
    value=text(value)
    # Identity hashes and long identifiers must wrap without changing text.
    breaks={m.start()+i for m in re.finditer(r'[A-Za-z0-9]{25,}',value) for i in range(7,len(m[0]),8)}
    return ''.join(r'\newline{}' if c=='\n' else ' ' if c in '\r\t' else escapes.get(c,c)+(r'\allowbreak{}' if c in '/.,:-_' or i in breaks else '') for i,c in enumerate(value))

def tex(sheets):
    validate_tables(sheets)
    result=r'''\documentclass[10pt]{article}
\usepackage[a4paper,margin=0.65in]{geometry}
\usepackage{fontspec,longtable,array}
\setmainfont{DejaVu Sans}[RawFeature={-tlig}]
\setlength{\parindent}{0pt}
\setlength{\tabcolsep}{5pt}
\renewcommand{\arraystretch}{1.2}
\begin{document}
\section*{Recorded acceptance report}
'''
    for name,rows in sheets:
        result+='\\subsection*{'+escape_tex(name)+'}\n'+r'\begin{longtable}{>{\raggedright\arraybackslash}p{0.23\linewidth}>{\raggedright\arraybackslash}p{0.70\linewidth}}'+'\n'
        if name=='Summary':pairs=[(row[0],row[1] if len(row)>1 else '') for row in rows if row]
        else:pairs=[(heading,value) for row in rows[1:] for heading,value in zip(rows[0],row)]
        result+='\n'.join(escape_tex(k)+' & '+escape_tex(v)+r' \\' for k,v in pairs)+'\n'+r'\end{longtable}'+'\n'
    return result+r'\end{document}'+'\n'

def write_bytes(path,data):
    path=Path(path);path.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=path.parent,delete=False) as f:
        temporary=Path(f.name)
        try:f.write(data);f.close();os.replace(temporary,path)
        finally:temporary.unlink(missing_ok=True)

def main(argv=None):
    p=argparse.ArgumentParser(description=__doc__);source=p.add_mutually_exclusive_group(required=True);source.add_argument('--catalogue',type=Path);source.add_argument('--comparison',type=Path)
    p.add_argument('--xls',type=Path);p.add_argument('--tex',type=Path);p.add_argument('--html',type=Path);p.add_argument('--libreoffice',default='libreoffice');args=p.parse_args(argv)
    targets=[(kind,getattr(args,kind)) for kind in ('xls','tex','html') if getattr(args,kind)];compare.need(targets,'Select at least one report output')
    path=args.catalogue or args.comparison;document=json.loads(path.read_text());kind='catalogue' if args.catalogue else 'comparison'
    for extension,target in targets:
        compare.need(target.suffix.lower()=='.'+extension,'Output extension differs from requested report format')
        compare.validate_output(target,[path]+[other for _,other in targets if other is not target])
    sheets=tables(document,kind,path);validate_tables(sheets)
    for extension,target in targets:
        if extension=='xls':write_xls(sheets,target,args.libreoffice)
        elif extension=='tex':write_bytes(target,tex(sheets).encode())
        else:write_bytes(target,(compare.render_catalogue(document) if kind=='catalogue' else compare.render_comparison(document)).encode())
    return 0

if __name__=='__main__':
    try:raise SystemExit(main())
    except (ValueError,OSError,TypeError,KeyError,subprocess.TimeoutExpired) as e:
        print('Acceptance export error: '+str(e),file=__import__('sys').stderr);raise SystemExit(2)
