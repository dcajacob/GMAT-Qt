"""Audit remaining offered Save function resources and alias persistence.

The prior87 native state/source receipts remain source-versioned evidence.
Their old retired-editor clean checks are not promoted to live-widget proof.
New selections require actual replacement-editor binding and original guards.
No complete any-Resource or goal qualification is admitted by this conjunction.
"""
import argparse
import json
import re
from pathlib import Path

import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load, require, sha
from AuditCentralBodyPropagationAcceptance import read_run
from AuditSaveResourceEvidence import check_case
from AuditSaveBodyEvidence import audit as parent_audit

FIXTURE = 'src/qtgui/tests/NasaSaveCatalogueCases.json'
PREFIX = 'QtGui.NasaSaveCatalogue'


def audit(repo, path):
    evidence = load(path)
    require(evidence['contract_kind'] == 'source-versioned-bounded-Save-catalogue-conjunction'
            and evidence['qualified_keys'] == [] and evidence['goal_complete'] is False
            and evidence['whole_FRC_E132_qualified'] is False,
            'Bounded catalogue cases silently award complete Save/goal credit')
    inventory = nasa.inventory(repo)
    require(evidence['authority_sha256'] == inventory['sources'], 'NASA authority changed')
    require(sha(evidence['parent_evidence']) == evidence['parent_evidence_sha256']
            and sha(evidence['parent_seal']) == evidence['parent_seal_sha256'],
            'Prior87-case source-versioned evidence changed')
    for name, digest in evidence['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed catalogue artifact: '+name)
    parent = parent_audit(repo, evidence['parent_evidence'])
    require(parent['cumulative_cases'] == 87 and parent['cumulative_contexts'] == 174
            and parent['new_requirement_credit'] == [], 'Prior evidence conjunction changed')
    matrix = load(evidence['fixture']);cases = {c['key']: c for c in matrix['cases']}
    require(len(cases) == len(matrix['cases']), 'Duplicate catalogue case')
    runs = {name: read_run(repo, spec, inventory, evidence['artifact_sha256'], {})
            for name, spec in evidence['runs'].items()}
    require(set(evidence['selected']) == {'headless', 'visible'}, 'Required catalogue mode omitted')
    count = 0;assets = 0;probe_contexts = 0
    for mode, selected in evidence['selected'].items():
        require(len(selected) == len(cases) and {e['case'] for e in selected} == set(cases),
                'Missing/duplicate catalogue case in mode '+mode)
        for entry in selected:
            receipt, results, sources = runs[entry['run']];name = PREFIX+entry['case']
            require(receipt['mode'] == mode and entry['test'] == name, 'Substituted mode/test')
            case = cases[entry['case']]
            code = sources['src/qtgui/tests/NasaSaveResourceTests.cpp'].decode()
            require('panel=dynamic_cast<CommandEditor *>(child->widget());require(panel,"Save resource live editor missing after Apply")' in code,
                    'Save clean-editor assertion still uses retired widget')
            work = Path(results[name]['log']).parent
            for artifact in ('applied-source.script', 'apply-diagnostics.json'):
                require(str(work/artifact) in evidence['artifact_sha256'], 'Live Apply artifact omitted')
            require((work/'applied-source.script').read_text() == (work/'accepted.script').read_text()
                    and load(work/'apply-diagnostics.json')['has_changes'] is False,
                    'Actual current editor/source not accepted cleanly')
            if 'expected_implementation' in case:
                cls=case['expected_implementation'];body=sources['src/base/function/'+cls+'.cpp'].decode()
                begin=body.index(cls+'::'+cls+'(');end=body.find('//  ~'+cls,begin);body=body[begin:end]
                witnesses={w['field']:w for w in case['witnesses']}
                for field, variable in [('Input','inputNames'),('Output','outputNames')]:
                    expected=re.findall(variable+r'.push_back\("([^"]*)"\)',body)
                    require(witnesses[field]['expected'] == expected, 'Builtin interface differs from native constructor declaration')
            count += check_case(case, results[name], sources, evidence['artifact_sha256'],
                                fixture_path=FIXTURE)
            assets += len(case.get('fixture_files', {}))
            probe_contexts += bool(case.get('reload_probe'))
    regression_contexts=0;regression_values=0
    require(set(evidence['affected_regressions'])=={'headless','visible'}, 'Affected regression mode omitted')
    for mode, entries in evidence['affected_regressions'].items():
        require(len(entries)==2 and {e['test'] for e in entries}=={'QtGui.NasaFunctionBuiltinSign','QtGui.NasaFunctionBuiltinSignAlias'}, 'Affected function-call regression omitted/duplicated')
        for entry in entries:
            receipt, results, sources=runs[entry['run']];name=entry['test'];result=results[name];work=Path(result['log']).parent
            require(receipt['mode']==mode and result['status']=='passed' and result['returncode']==0, 'Failed/substituted affected regression')
            saved=load(work/'proof.json');key=name.removeprefix('QtGui.NasaFunctionBuiltin')
            require([v for v in nasa.proof_records(result) if v.get('checkpoint')=='function-builtin-'+key]==[saved]
                    and saved['case']==key and saved['outcome']=='passed' and saved['missions']==2 and saved['function_calls']==4
                    and saved['oracle']=='literal' and saved['expected_domain_rejections']==0, 'Affected regression proof differs')
            native=load(work/'native-results.json');require(saved['runs']==native and len(native)==2, 'Affected native phase proof differs')
            fixture=evidence['affected_regression_fixture'];before=load(evidence['runs'][entry['run']]['before'])
            require(str(Path(fixture)) in evidence['artifact_sha256']
                    and sha(fixture)==evidence['affected_regression_fixture_sha256']==before['sources']['src/qtgui/tests/NasaFunctionBuiltinCases.json']
                    and str(repo/'src/qtgui/tests/NasaFunctionBuiltinCases.json') in result['registered_command'],
                    'Affected regression fixture not bound to its recorded execution')
            matrix=load(fixture)
            case=next(c for c in matrix['cases'] if c['key']==key)
            expected=[-1,0,1,-1,1]
            require(case['expected']==expected and case['builtin_calls_per_function']==5, 'Sign reference inputs/oracle changed')
            for i,phase in enumerate(['initial','reopened']):
                for artifact in ['proof.json','native-results.json',phase+'-observed-native.json',phase+'-values.csv',phase+'-observed-values.csv','initial.script','initial.gmf','valid.gmf']:
                    require(str(work/artifact) in evidence['artifact_sha256'], 'Affected regression artifact omitted: '+artifact)
                state=load(work/(phase+'-observed-native.json'));row=native[i]
                require(row['phase']==phase and row['first']==row['second']==state['first']==state['second']==expected
                        and row['continuation']==42 and row['private_name_absent'] is True and row['configured_prototypes_unchanged'] is True,
                        'Affected function result/identity/continuation differs')
                for artifact in [phase+'-values.csv',phase+'-observed-values.csv']:
                    values=[[float(v) for v in line.split(',')] for line in (work/artifact).read_text().strip().splitlines()]
                    require(values==[expected,expected], 'Raw affected regression report differs from independent sign truth')
                regression_values+=10
            regression_contexts+=1
    require(evidence['retired_editor_assertion_gap_recorded'] is True, 'Earlier retired-editor proof gap hidden')
    require(evidence['remaining_gaps'], 'Remaining full-resource obligation hidden')
    return dict(status='catalogue-Save-evidence-passed; complete any-Resource obligation open',
                new_cases=len(cases), new_contexts=2*len(cases), prior_cases_replayed=0,
                prior_cases_reused=87, cumulative_cases=87+len(cases),
                cumulative_contexts=174+2*len(cases), new_writable_property_comparisons=count,
                cumulative_writable_property_comparisons=count+parent['cumulative_writable_property_comparisons'],
                text_asset_contexts=assets, reloaded_function_execution_contexts=probe_contexts,
                affected_regression_contexts=regression_contexts, affected_regression_native_values=regression_values,
                original_row='FRC!E132', new_requirement_credit=[], goal_complete=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--evidence', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(audit(args.repo.resolve(), args.evidence)))
