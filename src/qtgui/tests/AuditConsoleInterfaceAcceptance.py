"""Qualify the literal GUI-interface exception using actual console evidence.

This is the frontend delivery boundary, not full console/GUI capability parity.
Existing native console executions and positive visible GUI delivery are reused
at their recorded source/runtime versions; no mission is executed here.
"""
import argparse
import hashlib
from pathlib import Path
import json
import tarfile

import NasaGuiCoverage as nasa
import AuditConsolePlotAcceptance as console
from AuditCapiAcceptance import load,require,sha

KEYS=['All Others!E23']


def qualify(repo,path,inventory):
    q=load(path)
    require(q['contract_kind']=='source-versioned-console-GUI-interface-exception' and q['qualified_keys']==KEYS,'Incomplete/extra GUI-interface exception credit')
    require(all(q.get(k) is False for k in ('technical_core_approval','full_current_build_qualified','full_NASA_qualified')),'GUI-interface exception exceeds recorded scope')
    require(q['authority_sha256']==inventory['sources'],'GUI-interface source baseline changed')
    for name,digest in q['artifact_sha256'].items():require(sha(name)==digest,'Changed GUI-interface exception artifact: '+name)
    review=load(q['review']);known={r['key']:r for r in inventory['requirements']};row=review['reviews']
    require(len(row)==1 and row[0]['key']==KEYS[0] and row[0]['fingerprint']==known[KEYS[0]]['fingerprint'] and row[0]['source_text']==known[KEYS[0]]['text'] and row[0]['rationale'],'Changed original GUI-interface obligation')
    require(review['unqualified_parent_keys']==['All Others!E20'],'Whole-product parity silently credited')
    old=load(q['console_qualification']);run=Path(old['run']);receipt=load(run/'receipt.json');before=load(run/'inputs-before.json');after=load(run/'inputs-after.json')
    require(before==after and receipt['status']=='passed' and receipt['inputs_unchanged'],'GUI-interface exception lacks unchanged actual native run')
    with tarfile.open(run/'sources.tar.gz') as bundle:
        sources={name:bundle.extractfile(name).read() for name in ['src/console/driver.cpp','src/console/ConsoleMessageReceiver.cpp','src/base/executive/PlotInterface.cpp']}
    for name,data in sources.items():require(hashlib.sha256(data).hexdigest()==receipt['sources'][name]==before[str(repo/name)],'Unbound recorded console frontend source')
    driver=sources['src/console/driver.cpp'].decode();receiver=sources['src/console/ConsoleMessageReceiver.cpp'].decode()
    for token in ['int main(int argc, char *argv[])','Console Based Version','ConsoleMessageReceiver::Instance()','MessageInterface::SetMessageReceiver(theMessageReceiver)','if (argc < 2)','std::cin >> scriptfile','RunScriptInterpreter','--minimize option ignored','GMAT server currently unavailable to GmatConsole']:
        require(token in driver,'Missing recorded console frontend boundary: '+token)
    require('std::cout' in receiver and 'std::cout' in driver and not any(token in driver for token in ['QApplication','wxApp','GmatMainFrame','QtPlotReceiver','SetPlotReceiver']), 'Recorded console frontend includes or installs a GUI interface')
    require(receipt['mode']=='native-console' and receipt['GUI_executions']==receipt['screenshots']==receipt['runtime_copies']==0,'Console boundary misrepresented')
    require(len(receipt['results'])==8 and all(r['status']=='passed' and r['display_variables_removed'] and r['loaded_project_images'] and not r['loaded_GUI_images'] for r in receipt['results']),'Actual display-free/no-GUI-library console delivery missing')
    # All existing numerical, source, actual-command, loaded-image and positive
    # visible GUI checks remain mandatory. Reuse does not turn an architecture
    # exception into all-command, plugin, platform or current-build parity.
    console.qualify(repo,Path(q['console_qualification']),inventory)
    return q,KEYS


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);a=p.parse_args();_,keys=qualify(a.repo.resolve(),a.qualification,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,GUI_or_engine_executions=0)))
