"""Audit hidden/conditional Save configuration and selected input modes.

File-derived region vertices are checked against retained input columns before
Save and after actual re-execution; the uninitialized rebuilt cache is recorded
as derived state. GroundTrack explicit grid settings must survive reload.
No whole any-Resource or scientific qualification is inferred.
"""
import argparse
import json
import re
from pathlib import Path

import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load, require, sha
from AuditCentralBodyPropagationAcceptance import read_run
from AuditSaveResourceEvidence import check_case
from AuditSaveCatalogueEvidence import audit as parent_audit

FIXTURE = 'src/qtgui/tests/NasaSaveModeCases.json'
PREFIX = 'QtGui.NasaSaveMode'


def audit(repo, path):
    evidence = load(path)
    require(evidence['contract_kind'] == 'source-versioned-bounded-Save-mode-conjunction'
            and evidence['qualified_keys'] == [] and evidence['goal_complete'] is False
            and evidence['whole_FRC_E132_qualified'] is False,
            'Bounded mode cases silently award complete Save/goal credit')
    inventory = nasa.inventory(repo)
    require(evidence['authority_sha256'] == inventory['sources'], 'NASA authority changed')
    require(sha(evidence['parent_evidence']) == evidence['parent_evidence_sha256']
            and sha(evidence['parent_seal']) == evidence['parent_seal_sha256'],
            'Prior109-case source-versioned evidence changed')
    for name, digest in evidence['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed mode artifact: '+name)
    parent = parent_audit(repo, evidence['parent_evidence'])
    require(parent['cumulative_cases'] == 109 and parent['cumulative_contexts'] == 218
            and parent['new_requirement_credit'] == [], 'Prior evidence conjunction changed')
    matrix = load(evidence['fixture']);cases = {c['key']: c for c in matrix['cases']}
    require(len(cases) == len(matrix['cases']), 'Duplicate mode case')
    require(set(evidence['physical_input_sha256'])=={'application/data/graphics/texture/Moon_HermesCelestiaMotherlode.jpg'}, 'Explicit texture input identity omitted/substituted')
    runs = {name: read_run(repo, spec, inventory, evidence['artifact_sha256'], evidence['physical_input_sha256'])
            for name, spec in evidence['runs'].items()}
    require(set(evidence['selected']) == {'headless', 'visible'}, 'Required mode mode omitted')
    count = 0;assets = 0;probe_contexts = 0
    for mode, selected in evidence['selected'].items():
        require(len(selected) == len(cases) and {e['case'] for e in selected} == set(cases),
                'Missing/duplicate mode case in mode '+mode)
        for entry in selected:
            receipt, results, sources = runs[entry['run']];name = PREFIX+entry['case']
            require(receipt['mode'] == mode and entry['test'] == name, 'Substituted mode/test')
            case = cases[entry['case']]
            code=sources['src/qtgui/tests/NasaSaveResourceTests.cpp'].decode()
            require('panel=dynamic_cast<CommandEditor *>(child->widget());require(panel,"Save resource live editor missing after Apply")' in code, 'Retired editor substituted for current panel')
            work=Path(results[name]['log']).parent
            if case['key']=='RegionFile':
                body=next(iter(case['fixture_files'].values())).strip().splitlines()
                require(body[0]=='PlanetographicRegion', 'Region input header changed')
                columns=list(zip(*[[float(v) for v in row.split()] for row in body[1:]]))
                expected={'Latitude':list(columns[0]),'Longitude':list(columns[1])}
                before=load(work/'runtime-native.json')['Subject'];after=load(work/'reloaded-native.json')['Subject']
                probe=load(work/'reload-probe-native.json')['Subject']
                for field_name,values in expected.items():
                    for obj in [before,probe]:
                        field=next(f for f in obj['fields'] if f['name']==field_name)
                        require(field['type']=='Rvector' and field['read_only'] is True and field['value']==values,
                                'File-derived vertices differ from independent retained columns')
                    require(next(f for f in after['fields'] if f['name']==field_name)['read_only'] is True, 'Uninitialized derived region field exposed as writable input')
                raw=(work/'export.data').read_text()
                require(not re.search(r'^[ \t]*(?:GMAT\s+)?Subject\.(?:Latitude|Longitude)\s*=',raw,re.M), 'Region export serializes both file and derived vertex input modes')
            count += check_case(case, results[name], sources, evidence['artifact_sha256'],
                                fixture_path=FIXTURE)
            assets += len(case.get('fixture_files', {}))
            probe_contexts += bool(case.get('reload_probe'))
    regression_contexts=0
    require(set(evidence['affected_regressions'])=={'headless','visible'}, 'Affected region regression mode omitted')
    for mode,entries in evidence['affected_regressions'].items():
        require(len(entries)==1 and entries[0]['test']=='QtGui.Region', 'Affected region regression omitted/duplicated')
        entry=entries[0];receipt,results,sources=runs[entry['run']];result=results['QtGui.Region']
        require(receipt['mode']==mode and result['status']=='passed' and result['returncode']==0,
                'Affected region workflow did not pass in its required mode')
        require('PASS Region paired inline creation/Cancel' in Path(result['log']).read_text(), 'Affected region workflow terminal assertions absent')
        code=sources['src/qtgui/tests/RegionWorkflowTests.cpp'].decode()
        for token in ['Initialized file-backed serialization defined both inputs','File-to-vertices left stale file/changed source','Area file was not committed']:
            require(token in code, 'Affected file/vertex/editor guard omitted')
        regression_contexts+=1
    require(evidence['remaining_gaps'], 'Remaining full-resource obligation hidden')
    return dict(status='mode-Save-evidence-passed; complete any-Resource obligation open',
                new_cases=len(cases), new_contexts=2*len(cases), prior_cases_replayed=0,
                prior_cases_reused=109, cumulative_cases=109+len(cases),
                cumulative_contexts=218+2*len(cases), new_writable_property_comparisons=count,
                cumulative_writable_property_comparisons=count+parent['cumulative_writable_property_comparisons'],
                text_asset_contexts=assets, reinitialized_region_contexts=probe_contexts,
                affected_regression_contexts=regression_contexts,
                original_row='FRC!E132', new_requirement_credit=[], goal_complete=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--evidence', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(audit(args.repo.resolve(), args.evidence)))
