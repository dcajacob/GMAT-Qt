"""Audit attitude-owned configuration persistence at its recorded source versions.

Native owned getters and explicit input literals check models and active state/
rate representations. Existing scientific attitude oracles keep their original
scope; kernel/AEM and full any-Resource obligations remain separate gaps.
"""
import argparse
import json
from pathlib import Path

import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load, require, sha
from AuditCentralBodyPropagationAcceptance import read_run
from AuditSaveResourceEvidence import check_case
from AuditSaveAttitudeEvidence import audit as parent_audit

FIXTURE = 'src/qtgui/tests/NasaSaveFileAttitudeCases.json'
PREFIX = 'QtGui.NasaSaveFileAttitude'


def audit(repo, path):
    evidence = load(path)
    require(evidence['contract_kind'] == 'source-versioned-bounded-Save-file-attitude-conjunction'
            and evidence['qualified_keys'] == [] and evidence['goal_complete'] is False
            and evidence['whole_FRC_E132_qualified'] is False,
            'Bounded attitude cases silently award complete Save/goal credit')
    inventory = nasa.inventory(repo)
    require(evidence['authority_sha256'] == inventory['sources'], 'NASA authority changed')
    require(sha(evidence['parent_evidence']) == evidence['parent_evidence_sha256']
            and sha(evidence['parent_seal']) == evidence['parent_seal_sha256'],
            'Prior128-case source-versioned evidence changed')
    for name, digest in evidence['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed attitude artifact: '+name)
    parent = parent_audit(repo, evidence['parent_evidence'])
    require(parent['cumulative_cases'] == 128 and parent['cumulative_contexts'] == 256
            and parent['new_requirement_credit'] == [], 'Prior evidence conjunction changed')
    required_inputs = {name: digest for name, digest in evidence['physical_input_sha256'].items()}
    require(set(required_inputs) == {'application/data/vehicle/ephem/spk/MarsExpress_ATNM_PTR00012_100531_002.BC', 'application/data/vehicle/ephem/spk/MarsExpress_MEX_100921_STEP.TSC'}, 'Missing/unexpected CK/SCLK input identity')
    matrix = load(evidence['fixture']);cases = {c['key']: c for c in matrix['cases']}
    require(len(cases) == len(matrix['cases']), 'Duplicate attitude case')
    runs = {name: read_run(repo, spec, inventory, evidence['artifact_sha256'], evidence['physical_input_sha256'])
            for name, spec in evidence['runs'].items()}
    require(set(evidence['selected']) == {'headless', 'visible'}, 'Required attitude mode omitted')
    count = 0;assets = 0;probe_contexts = 0
    for mode, selected in evidence['selected'].items():
        require(len(selected) == len(cases) and {e['case'] for e in selected} == set(cases),
                'Missing/duplicate attitude case in mode '+mode)
        for entry in selected:
            receipt, results, sources = runs[entry['run']];name = PREFIX+entry['case']
            require(receipt['mode'] == mode and entry['test'] == name, 'Substituted mode/test')
            case = cases[entry['case']]
            work=Path(results[name]['log']).parent
            for artifact in ['runtime-native.json','reloaded-native.json']:
                root=load(work/artifact)['Subject'];model=case['expected_attitude_model']
                require([child['type'] for child in root['owned']]==[model], 'Saved attitude native class identity differs')
                fields={field['name']:field for field in root['fields']}
                require(fields['Attitude']['value']==model, 'Spacecraft attitude selector differs from native owned model')
            code=sources['src/qtgui/tests/NasaSaveResourceTests.cpp'].decode()
            require('Owned literal witness path missing or ambiguous' in code, 'Owned attitude getter guard absent')
            count += check_case(case, results[name], sources, evidence['artifact_sha256'],
                                fixture_path=FIXTURE)
            assets += len(case.get('fixture_files', {}))
            probe_contexts += bool(case.get('reload_probe'))
    require(evidence['remaining_gaps'], 'Remaining full-resource obligation hidden')
    return dict(status='file-attitude-Save-evidence-passed; complete any-Resource obligation open',
                new_cases=len(cases), new_contexts=2*len(cases), prior_cases_replayed=0,
                prior_cases_reused=128, cumulative_cases=128+len(cases),
                cumulative_contexts=256+2*len(cases), new_writable_property_comparisons=count,
                cumulative_writable_property_comparisons=count+parent['cumulative_writable_property_comparisons'],
                text_asset_contexts=assets, reloaded_function_execution_contexts=probe_contexts,
                original_row='FRC!E132', new_requirement_credit=[], goal_complete=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--evidence', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(audit(args.repo.resolve(), args.evidence)))
