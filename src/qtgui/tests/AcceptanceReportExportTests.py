"""Actual Excel97 readback, compiled TeX, HTML and failure-isolation contracts."""
import copy,hashlib,json,os
from pathlib import Path
import re,shutil,struct,subprocess,sys,tempfile,unittest
import xml.etree.ElementTree as ET

import AcceptanceCatalog as catalog
from AcceptanceCatalogTests import fixture
import AcceptanceCompare as compare
import AcceptanceReportExports as export

PROOF_FACETS=['genuine-excel97-biff8-native-readback','standalone-tex-compile-and-text','existing-html-export-actual-cli','literal-mixed-outcomes-unicode-and-injection-escaping','exact-counts-identities-input-isolation-and-format-limits']
EVIDENCE=Path(os.environ['GMAT_QT_ACCEPTANCE_DIR']) if os.environ.get('GMAT_QT_ACCEPTANCE_DIR') else None
CHECKED_FORMATS=set()

def save(path):
    if EVIDENCE:EVIDENCE.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,EVIDENCE/path.name)

def office_readback(path):
    """Independent native engine interprets the binary XLS; never inspect the ODS."""
    work=path.parent/'readback';work.mkdir()
    p=subprocess.run(['libreoffice','-env:UserInstallation='+(work/'profile').as_uri(),'--headless','--convert-to','fods:OpenDocument Spreadsheet Flat XML','--outdir',str(work),str(path)],env=dict(os.environ,GSETTINGS_BACKEND='memory',SAL_USE_VCLPLUGIN='svp'),capture_output=True,text=True,timeout=20)
    assert p.returncode==0,p.stdout+p.stderr
    root=ET.parse(work/(path.stem+'.fods')).getroot();ns={'t':export.NS['table'],'x':export.NS['text'],'o':export.NS['office'],'s':export.NS['style']};tables={};types={};formulas=[]
    styles={node.get('{'+ns['s']+'}name'):node for node in root.findall('.//s:style',ns)}
    def wrapping(name):
        visited=set()
        while name in styles and name not in visited:
            visited.add(name);style=styles[name];prop=style.find('s:table-cell-properties',ns)
            if prop is not None and prop.get('{'+export.NS['fo']+'}wrap-option'):return prop.get('{'+export.NS['fo']+'}wrap-option')
            name=style.get('{'+ns['s']+'}parent-style-name')
        return None
    def content(node):
        result=node.text or ''
        for child in node:
            if child.tag=='{'+ns['x']+'}s':result+=' '*int(child.get('{'+ns['x']+'}c','1'))
            elif child.tag=='{'+ns['x']+'}tab':result+='\t'
            elif child.tag=='{'+ns['x']+'}line-break':result+='\n'
            else:result+=content(child)
            result+=child.tail or ''
        return result
    for sheet in root.findall('.//t:table',ns):
        name=sheet.get('{'+ns['t']+'}name');rows=[];kind=[]
        for row in sheet.findall('t:table-row',ns):
            values=[];value_types=[]
            for cell in row:
                if cell.tag!='{'+ns['t']+'}table-cell':continue
                if cell.get('{'+ns['t']+'}formula'):formulas.append(cell.attrib)
                value='\n'.join(content(p) for p in cell.findall('x:p',ns));n=int(cell.get('{'+ns['t']+'}number-columns-repeated','1'))
                if len(value)>60:assert wrapping(cell.get('{'+ns['t']+'}style-name'))=='wrap','Native XLS long field lost wrapping: '+value[:80]
                if not value and n>256:continue
                assert n<=256;values += [value]*n;value_types += [cell.get('{'+ns['o']+'}value-type')]*n
            while values and not values[-1]:values.pop();value_types.pop()
            n=int(row.get('{'+ns['t']+'}number-rows-repeated','1'))
            if values:assert n<=65536;rows.extend([values]*n);kind.extend([value_types]*n)
        tables[name]=rows;types[name]=kind
    return tables,types,formulas

class ExportContracts(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='gmat-exports-Δ-');self.addCleanup(self.temp.cleanup);self.work=Path(self.temp.name)
    def cli(self,source,kind='catalogue',*options):
        return subprocess.run([sys.executable,str(Path(export.__file__)), '--'+kind,str(source),*map(str,options)],capture_output=True,text=True,timeout=50)
    def compile_tex(self,path):
        completed=subprocess.run(['lualatex','--no-shell-escape','--interaction=nonstopmode','--halt-on-error',path.name],cwd=path.parent,capture_output=True,text=True,timeout=20)
        self.assertEqual(completed.returncode,0,completed.stdout[-3500:]+completed.stderr)
        log=path.with_suffix('.log').read_text();self.assertNotIn('Missing character:',log);self.assertNotIn('Overfull \\hbox',log)
        pdf=path.with_suffix('.pdf');self.assertTrue(pdf.is_file());out=path.with_suffix('.txt');subprocess.run(['pdftotext','-layout',str(pdf),str(out)],check=True);save(path);save(pdf);save(out)
        return out.read_text()
    def xls(self,path):
        self.assertEqual(path.read_bytes()[:8],bytes.fromhex('d0cf11e0a1b11ae1'))
        import olefile
        with olefile.OleFileIO(path) as book:
            data=book.openstream('Workbook').read(8);self.assertEqual(struct.unpack('<HHHH',data),(0x809,16,0x600,5))
        save(path);return office_readback(path)
    def test_catalogue_genuine_xls_all_outcomes_counts_metadata_tex_and_html(self):
        r,n,m=fixture();document=catalog.catalogue(r,n,m);document['scopes'][0]['name']='=2+3';special=r'\input{never} % # & _ { } $ ~ ^ Ω'+'\nsecond  line\tend';document['scopes'][1]['name']=special
        source=self.work/'catalogue.json';source.write_text(json.dumps(document,ensure_ascii=False));before=hashlib.sha256(source.read_bytes()).hexdigest();xls=self.work/'catalogue.xls';tex=self.work/'catalogue.tex';page=self.work/'catalogue.html'
        result=self.cli(source,'catalogue','--xls',xls,'--tex',tex,'--html',page);self.assertEqual(result.returncode,0,result.stderr)
        sheets,types,formulas=self.xls(xls);self.assertEqual(set(sheets),{'Summary','Tests','Scopes'});self.assertEqual(formulas,[])
        expected={'tests_in_receipt':'8','tests_in_catalogue':'9','tests_run':'5','reused_passes':'1','execution_unproven':'1','tests_passed':'3','tests_failed':'2','failed_requirements':'2'}
        summary={row[0]:row[1] for row in sheets['Summary'] if len(row)==2}
        for key,value in expected.items():self.assertEqual(summary[key],value)
        for row,kind in zip(sheets['Summary'],types['Summary']):
            if row[0] in expected:self.assertEqual(kind[1],'float')
        observed={row[0]:row for row in sheets['Tests'][1:]};self.assertEqual({name:row[1] for name,row in observed.items()},{'Good':'passed','Bad':'failed','Slow':'timeout','Reused':'passed','Skipped':'skipped','Unrun':'not-run','Stale':'passed','Interrupted':'interrupted','Planned':'not-run'})
        self.assertEqual([observed[x][2] for x in ['Good','Reused','Stale','Unrun','Skipped']],['executed-in-recorded-run','reused','executed-in-recorded-run','not-run','execution-unproven'])
        self.assertEqual(json.loads(observed['Good'][6])['metadata'],m['tests']['Good'])
        scopes={row[0]:row for row in sheets['Scopes'][1:]};self.assertEqual(len(scopes),11);self.assertEqual(scopes['R1'][3],'=2+3');self.assertEqual(scopes['R2'][3],special);self.assertEqual(scopes['R4'][4],'no-tests');self.assertEqual(scopes['R5'][4],'unexecuted');self.assertEqual(scopes['R9'][4],'verified-in-supplied-report')
        text=self.compile_tex(tex);self.assertIn('Recorded acceptance report',text);self.assertIn('tests failed',text.replace('_',' '));self.assertIn('=2+3',text);self.assertIn('never',text);self.assertIn('Ω',text);self.assertFalse((self.work/'never').exists())
        html=page.read_text();self.assertIn('&amp;',html);self.assertIn('=2+3',html);self.assertIn('Recorded failures: 2',html);self.assertIn('Requirement R4',html);save(page);self.assertEqual(hashlib.sha256(source.read_bytes()).hexdigest(),before);CHECKED_FORMATS.update(['xls','tex','html'])
    def test_comparison_keeps_exact_decimal_differences_units_errors_and_input_identity(self):
        a=self.work/'actual.txt';t=self.work/'truth.txt';a.write_text('9007199254740993 7.25\n');t.write_text('9007199254740992 7\n');document=compare.Registry().compare(a,t,'numeric',{'absolute':'0.1'});self.assertEqual(document['status'],'failed');document['comparisons'][0]['unit']='km';document['errors']=['diagnostic Δ <not markup>'];document['status']='error'
        source=self.work/'comparison.json';source.write_text(json.dumps(document,ensure_ascii=False));xls=self.work/'comparison.xls';tex=self.work/'comparison.tex';page=self.work/'comparison.html';result=self.cli(source,'comparison','--xls',xls,'--tex',tex,'--html',page);self.assertEqual(result.returncode,0,result.stderr)
        sheets,types,formulas=self.xls(xls);self.assertEqual(formulas,[]);self.assertEqual(set(sheets),{'Summary','Numeric','Differences','Errors'});self.assertEqual(sheets['Numeric'][1][2:],['km','9007199254740993','9007199254740992','1','0.1','True']);self.assertEqual(sheets['Numeric'][2][3:],['7.25','7','0.25','0.1','True']);self.assertEqual(sheets['Errors'][1],['diagnostic Δ <not markup>'])
        for row in types['Numeric'][1:]:self.assertEqual(row[3:7],['string']*4)
        summary={r[0]:r[1] for r in sheets['Summary'] if len(r)==2};self.assertEqual(summary['Status'],'error');self.assertEqual(summary['Numeric fields'],'2');self.assertIn(document['inputs']['actual']['sha256'],summary['actual input']);text=self.compile_tex(tex);self.assertIn('9007199254740993',text);self.assertIn('diagnostic',text);self.assertIn('&lt;not markup&gt;',page.read_text());save(page)
    def test_limits_invalid_controls_nonfinite_counts_and_untrusted_tex_are_not_hidden(self):
        for sheets,pattern in [([('Rows',[['x']]*65537)],'row/column'),([('Columns',[['x']*257])],'row/column'),([('Long',[['x'*32768]])],'cell limit'),([('Nonfinite',[[float('nan')]])],'Nonfinite'),([('Invalid',[['\0']])],'control')]:
            with self.subTest(pattern=pattern),self.assertRaisesRegex(ValueError,pattern):export.validate_tables(sheets)
        escaped=export.escape_tex(r'\write18{touch forbidden}');self.assertNotIn(r'\write18',escaped);self.assertIn(r'\textbackslash{}',escaped)
    def test_overwrite_alias_collision_and_wrong_extension_leave_inputs_unchanged(self):
        r,n,m=fixture();source=self.work/'input.json';source.write_text(json.dumps(catalog.catalogue(r,n,m)));before=source.read_bytes();alias=self.work/'alias.tex';alias.symlink_to(source);hard=self.work/'hard.tex';os.link(source,hard)
        for args in [('--tex',source),('--tex',alias),('--tex',hard),('--html',self.work/'wrong.tex'),('--tex',self.work/'same.tex','--html',self.work/'same.tex')]:
            p=self.cli(source,'catalogue',*args);self.assertEqual(p.returncode,2,p.stdout);self.assertEqual(source.read_bytes(),before)
    def test_missing_or_false_success_converter_never_replaces_existing_output(self):
        r,n,m=fixture();source=self.work/'input.json';source.write_text(json.dumps(catalog.catalogue(r,n,m)));target=self.work/'keep.xls';target.write_bytes(b'preserve original');fake=self.work/'converter';fake.write_text('#!/bin/sh\nexit 0\n');fake.chmod(0o755)
        for converter in [self.work/'missing',fake]:
            p=self.cli(source,'catalogue','--xls',target,'--libreoffice',converter);self.assertEqual(p.returncode,2,p.stdout);self.assertEqual(target.read_bytes(),b'preserve original')

if __name__=='__main__':
    result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(ExportContracts))
    if result.wasSuccessful():
        for key in [327,328,329]:print('NASA_CHECK '+json.dumps(dict(checkpoint='report-formats-'+str(key),outcome='passed',formats=sorted(CHECKED_FORMATS),contracts=result.testsRun,source_key='All Others!E'+str(key),facets=PROOF_FACETS)))
    raise SystemExit(not result.wasSuccessful())
