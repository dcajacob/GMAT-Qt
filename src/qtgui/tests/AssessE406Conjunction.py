"""Assess the exact E406 conjunction without awarding a terminal iterate as a solution."""
import argparse,hashlib,importlib,json
from pathlib import Path
from BuildNasaCoverageExtension import require,sha,load
from AuditCommandNavigationOperations import compare
from ReviewE406Retained import KEY,FINGERPRINT
import NasaGuiCoverage as nasa

STAGES=[('nested-ephemeris','AuditNestedEphemeris',2),('interrupted-ephemeris','AuditInterruptedEphemeris',1),('terminal-ephemeris','AuditTerminalEphemeris',1),('filter-stk','AuditFilterStk',1),('batch-output-edges','AuditBatchOutputEdges',2),('nested-batch','AuditNestedBatch',1)]


def assess(repo,retained):
    old=load(retained);require(old['original_key']==KEY and old['fingerprint']==FINGERPRINT and old['qualified_keys']==[] and old['metrics']['old_native_contexts']==28,'Wrong retained conjunction')
    for logical,binding in old['input_bindings'].items():
        path=Path(binding['physical']);require(path.is_file() and not path.is_symlink() and sha(path)==binding['physical_sha256'],'Changed retained physical input '+str(path))
        from AuditAcceptanceRecovery import payload
        require(hashlib.sha256(payload(logical)).hexdigest()==binding['payload_sha256'],'Changed retained payload '+logical)
    authority=nasa.inventory(repo);source=next(x for x in authority['requirements'] if x['key']==KEY);require(source['fingerprint']==FINGERPRINT,'Changed requirement authority');new=[]
    for name,module,version in STAGES:
        stage=repo/'build/nasa-coverage'/('coverage95-20261103-'+name);path=stage/f'evidence-audit-v{version}.json';recorded=load(path);actual=importlib.import_module(module).audit(repo,stage);require(actual==recorded,'Constituent audit no longer reproduces '+name)
        manifest=stage/f'evidence-inputs-v{version}.json';require(manifest.is_file(),'Missing constituent manifest');new.append(dict(stage=name,audit=str(path),audit_sha256=sha(path),manifest=str(manifest),manifest_sha256=sha(manifest),auditor='src/qtgui/tests/'+module+'.py',auditor_sha256=sha(repo/'src/qtgui/tests'/ (module+'.py')),status=actual['status'],qualified_keys=actual['qualified_keys'],metrics={k:v for k,v in actual.items() if k not in ['selected','failures','negative_controls','runtime_census']}))
    terminal=repo/'build/gui-acceptance/terminal-ephemeris-visible-v1';counterexamples=[]
    for family,outer in [('NestedDC','DC'),('NestedYukon','Opt')]:
        root=terminal/('QtGui.NasaTerminalEphemeris'+family);proof=load(root/'proof.json');failed,recovered=proof['phases'];require(not failed['converged'] and len(failed['rows'])==25,'Non-solution terminal counterexample missing');require(recovered['converged'],'Known solution/recovery absent');compare([failed['U'],failed['V'],failed['radius']],[1,3,7130],[1e-6,1e-6,2e-5]);compare([recovered['U'],recovered['V'],recovered['radius']],[2,3,7230],[1e-6,1e-6,2e-5]);state=next(s for s in failed['terminal_states'] if s['name']==outer);require(state['state']==10014 and state['status']==-1,'Native FINISHED/EXCEEDED distinction lost')
        counterexamples.append(dict(family=family,proof=str(root/'proof.json'),proof_sha256=sha(root/'proof.json'),requested_outer_solution=2,terminal_outer_value=1,outer_residual=1,terminal_status=state['status'],emitted_OEM_rows=25,terminal_radius_km=7130,converged_recovery_radius_km=7230))
    census_path=repo/'build/nasa-coverage/coverage95-20261103-nested-batch/finite-source-census-v1.json';census=load(census_path);docs={n:sha(repo/n) for n in ['doc/help/src/Resource_EphemerisFile.xml','doc/help/src/Command_Target.xml','doc/help/src/Command_Optimize.xml']};text=(repo/'doc/help/src/Resource_EphemerisFile.xml').read_text();require('for the final solution from the iterative processes' in text,'Current output guide changed')
    return dict(status='whole-E406-not-admitted;terminal-non-solution-output-is-a-source-behavior-conflict',original_key=KEY,fingerprint=FINGERPRINT,original_source=source,authority_sha256=authority['sources'],qualified_keys=[],retained_audit=str(retained),retained_audit_sha256=sha(retained),retained_metrics=old['metrics'],newer_constituents=new,finite_census=str(census_path),finite_census_sha256=sha(census_path),native_census=census['runtime_census'],counterexamples=counterexamples,whole_row_decision=dict(proven='All recovered output-format constituents and six newer membership/interruption/recovery audits reproduce at recorded versions.',not_proven='A known non-solution is written after DC/Yukon iteration limits. FINISHED and final-pass membership do not establish the original solution obligation.',rationale='The outer equation independently requires U=2; U=1 has residual 1 and native EXCEEDED_ITERATIONS. The current guide applies last calculated controls but does not state that they satisfy the requested problem. No convergence waiver or inferred NASA source retirement is made.',next_action='Keep E406 open as an explicit source/runtime conflict. No more passing permutations are needed to demonstrate this gap. A source-backed supported-behavior decision is required before changing the native terminal-output contract.',separate_gate='The unconstrained VF13ad accuracy failure remains open independently; it is not added as a new output-membership clause.'),current_guide_sha256=docs,production_changed_files=[],native_missions_replayed=0,historical_receipts_modified=False,current_reference_rebindings=0,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--retained-audit',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=assess(Path(__file__).resolve().parents[3],a.retained_audit.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k in ['status','original_key','qualified_keys','retained_metrics','whole_row_decision']}))
