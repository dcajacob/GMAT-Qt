"""Re-audit all original NASA report-format rows from actual harness receipts."""
import argparse,hashlib,json
from pathlib import Path
import NasaGuiCoverage as nasa
from RunHarnessAcceptance import passed_junit

KEYS={'All Others!E327','All Others!E328','All Others!E329'}
TOKENS={
 'src/qtgui/tests/AcceptanceReportExportTests.py':[
  "struct.unpack('<HHHH',data),(0x809,16,0x600,5)","'fods:OpenDocument Spreadsheet Flat XML'",
  "self.assertEqual(formulas,[])","'9007199254740993','9007199254740992','1','0.1','True'",
  "self.assertEqual(row[3:7],['string']*4)",
  "'Native XLS long field lost wrapping: '",
  "self.assertNotIn('Overfull \\\\hbox',log)","self.assertNotIn('Missing character:',log)",
  "'tests_passed':'3','tests_failed':'2','failed_requirements':'2'", "self.assertEqual(target.read_bytes(),b'preserve original')",
  "self.assertEqual(json.loads(observed['Good'][6])['metadata'],m['tests']['Good'])",'65537','32768'],
 'src/qtgui/tests/AcceptanceReportExports.py':["'xls:MS Excel 97'","'--headless'",'validate_tables(sheets)',
  'report is not truncated',"str(source.resolve())","hashlib.sha256(source.read_bytes()).hexdigest()",'compare.validate_output(target',
  "'Actual (exact text)'",'os.replace(temporary,path)']}

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text())
def need(ok,message):
    if not ok:raise ValueError(message)

def qualify(repo,path,inventory):
    q=load(path);need(q['contract_kind']=='source-versioned-harness-reports' and len(q['qualified_keys'])==3 and set(q['qualified_keys'])==KEYS,'Partial/extra/duplicate report-format credit')
    need(all(q.get(k) is False for k in ('full_current_build_qualified','full_NASA_qualified','technical_core_approval')),'Report proof exceeds qualification scope')
    for p,d in q['artifact_sha256'].items():need(sha(p)==d,'Changed report qualification artifact: '+p)
    need(q['authority_sha256']==inventory['sources'],'Original NASA report authority differs')
    original=load(q['original_qualification']);receipt=load(original['receipt']);frozen=Path(original['source_root'])
    need(original['source_inputs_unchanged'] is True and receipt['source_inputs_unchanged'] is True and receipt['mode']=='harness-current-source','Actual source-bound harness execution absent')
    need(sha(original['receipt'])==original['receipt_sha256'] and sha(original['report'])==original['report_sha256'] and sha(original['review'])==original['review_sha256'],'Original harness receipt/review/report changed')
    copied=original['preserved_source_sha256'];authority=original['authority_inputs_not_copied'];need(authority==inventory['sources'] and not set(copied)&set(authority) and {**copied,**authority}==receipt['source_hashes'],'Incomplete original harness source partition')
    for source,digest in copied.items():
        need(not Path(source).is_absolute() and '..' not in Path(source).parts,'Unsafe harness source member')
        data=(frozen/source).read_bytes();need(hashlib.sha256(data).hexdigest()==digest and not data.startswith(b'\x7fELF'),'Changed/non-source original harness input')
    need(copied['src/qtgui/tests/NasaGuiCoverage.py']==sha(nasa.__file__),'Changed original claim interpreter')
    review=nasa.load_review(frozen,inventory,manifest=original['review']);need(all(r['applicability']=='harness' for r in review['reviews']) and len(review['reviews'])==3 and {r['key'] for r in review['reviews']}==KEYS,'Original report row conjunction changed')
    parent=next(r for r in review['reviews'] if r['key']=='All Others!E327');need(parent['constituent_requirements']==['All Others!E328','All Others!E329','All Others!E330'],'Original report-format parent omitted a child')
    results=receipt['results'];need(len(results)==1 and results[0]['name']=='QtAcceptance.ReportExportContracts','Extra/missing report contract execution')
    result=results[0];need(result['status']=='passed' and passed_junit(Path(result['junit']),result['name'],result['returncode']),'Actual report contract/JUnit did not pass')
    for field in ('log','junit'):need(sha(result[field])==result[field+'_sha256'],'Changed actual harness '+field)
    for source,tokens in TOKENS.items():
        code=(frozen/source).read_text()
        for token in tokens:need(token in code,'Missing original native export assertion: '+token)
    records=nasa.proof_records(result);need(len(records)==3 and {p['checkpoint'] for p in records}=={'report-formats-'+str(i) for i in (327,328,329)} and all(p['outcome']=='passed' and p['formats']==['html','tex','xls'] and p['contracts']==5 for p in records),'Actual all-format completion checkpoints missing')
    checked=nasa.evaluate(frozen,inventory,review,receipt);keys={r['key'] for kind in ('requirements','historical_cases') for r in checked[kind] if r['status']=='verified'}
    need(keys==KEYS==set(original['qualified_keys']),'Complete report-format clauses did not qualify')
    need(load(q['external_inputs_before'])==load(q['external_inputs_after']) and len(load(q['external_inputs_before']))==5,'External report tools changed across execution')
    return q,sorted(KEYS)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);a=p.parse_args();_,keys=qualify(a.repo.resolve(),a.qualification,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,new_GUI_or_engine_executions=0)))
