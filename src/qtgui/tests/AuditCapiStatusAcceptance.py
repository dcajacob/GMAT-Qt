"""Reuse an actual pure-C receipt for the complete last-status-message contract."""
import argparse,hashlib,json,tarfile
from pathlib import Path
import AuditCapiAcceptance as capi
import NasaGuiCoverage as nasa

def qualify(repo,path,inventory):
    q=capi.load(path);capi.require(q['contract_kind']=='source-versioned-C-API-status' and q['qualified_keys']==['All Others!E258'],'Unsupported/duplicate C API status credit')
    capi.require(all(q.get(k) is False for k in ('full_current_build_qualified','full_NASA_qualified','technical_core_approval')),'Status proof exceeds qualification scope')
    for p,d in q['artifact_sha256'].items():capi.require(capi.sha(p)==d,'Changed status artifact: '+p)
    capi.require(q['authority_sha256']==inventory['sources'],'Status NASA authority differs')
    base,keys=capi.qualify(repo,Path(q['original_qualification']),inventory);capi.require(base['case']=='basic' and set(keys)==capi.BASIC_KEYS,'Actual complete external C receipt is required')
    run=Path(base['run']);receipt=capi.load(run/'receipt.json')
    with tarfile.open(run/'sources.tar.gz') as b:code=b.extractfile('src/qtgui/tests/NasaCapiAcceptanceTests.c').read().decode()
    for assertion in ['CHECK(strstr(getLastMessage(),"initialized")!=NULL);','CHECK(LoadScript("nonexistent-acceptance-input.script")!=0);','CHECK(strlen(getLastMessage())>0);','CHECK(strstr(getLastMessage(),"success")!=NULL);','CHECK(strcmp(getLastMessage(),"Mission run succeeded!")==0);','CHECK(strstr(getLastMessage(),"updated")!=NULL);']:
        capi.require(assertion in code,'Original exact status assertion removed')
    capi.require(receipt['status']=='passed' and receipt['proofs'][0]['assertions']>=133,'Actual C status-bearing execution did not pass')
    review=capi.load(q['review']);row=next(r for r in inventory['requirements'] if r['key']=='All Others!E258')
    capi.require(review['sources']==inventory['sources'] and review['key']==row['key'] and review['fingerprint']==row['fingerprint'] and review['source_text']==row['text'] and review['rationale'],'Original status requirement changed')
    return q,q['qualified_keys']

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);a=p.parse_args();_,keys=qualify(a.repo.resolve(),a.qualification,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,new_GUI_or_engine_executions=0)))
