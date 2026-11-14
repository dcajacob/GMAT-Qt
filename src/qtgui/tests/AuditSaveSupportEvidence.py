"""Conjoin additional native Save families and the repaired GroundTrack case.

Original45-case evidence stays immutable. Its GroundTrack literal reload gap is
superseded by a source-versioned affected-case result; the other44 passes are
reused. Complete original any-Resource and95-percent claims remain forbidden.
"""
import argparse
import json
from pathlib import Path

import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load, require, sha
from AuditCentralBodyPropagationAcceptance import read_run
from AuditSaveResourceEvidence import audit as parent_audit, check_case

FIXTURE = 'src/qtgui/tests/NasaSaveSupportCases.json'
PREFIX = 'QtGui.NasaSaveSupport'


def audit(repo, path):
    evidence = load(path)
    require(evidence['contract_kind'] == 'source-versioned-bounded-Save-support-conjunction'
            and evidence['qualified_keys'] == [] and evidence['goal_complete'] is False
            and evidence['whole_FRC_E132_qualified'] is False,
            'Additional Save family evidence silently awards whole-row credit')
    inventory = nasa.inventory(repo)
    require(evidence['authority_sha256'] == inventory['sources'], 'NASA authority changed')
    require(sha(evidence['original_parent_evidence']) == evidence['original_parent_evidence_sha256']
            and sha(evidence['original_parent_seal']) == evidence['original_parent_seal_sha256'],
            'Earlier immutable Save evidence changed')
    for name, digest in evidence['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed additional Save artifact: '+name)
    parent = parent_audit(repo, evidence['parent_conjoined_evidence'])
    require(parent['new_cases'] == 45 and parent['contexts'] == 90
            and parent['new_requirement_credit'] == [], 'Previous family conjunction changed')
    original, conjoined = load(evidence['original_parent_evidence']), load(evidence['parent_conjoined_evidence'])
    for mode in ('headless', 'visible'):
        old = {e['case']: e for e in original['selected'][mode]}
        new = {e['case']: e for e in conjoined['selected'][mode]}
        require(set(old) == set(new) and {k for k in old if old[k] != new[k]}
                == {'GroundTrackPlotPopulated'}, 'Unrelated prior passing case replayed/substituted')
    fixture = load(evidence['fixture']);cases = {c['key']: c for c in fixture['cases']}
    require(len(cases) == len(fixture['cases']), 'Duplicate support case')
    runs = {name: read_run(repo, spec, inventory, evidence['artifact_sha256'], {})
            for name, spec in evidence['runs'].items()}
    require(set(evidence['selected']) == {'headless', 'visible'}, 'Required support mode omitted')
    count = 0
    for mode, entries in evidence['selected'].items():
        require(len(entries) == len(cases) and {e['case'] for e in entries} == set(cases),
                'Missing/duplicate additional Save case in mode '+mode)
        for entry in entries:
            receipt, results, sources = runs[entry['run']]
            name = PREFIX+entry['case']
            require(receipt['mode'] == mode and entry['test'] == name,
                    'Substituted support mode/test')
            count += check_case(cases[entry['case']], results[name], sources,
                                evidence['artifact_sha256'], fixture_path=FIXTURE)
            if entry['case'].startswith('DynamicData'):
                code = sources['src/qtgui/tests/NasaSaveResourceTests.cpp'].decode()
                require('GetDynamicDataStruct()' in code and 'isTextColorUserSet' in code
                        and 'Save resource reloaded literal/reference witness differs' in code,
                        'Dedicated native display state or reloaded literal assertion omitted')
    require(evidence['remaining_gaps'] and evidence['parent_old_groundtrack_reload_gap_recorded'],
            'Missing older gap history or remaining scope')
    return dict(status='additional-native-Save-evidence-passed; any-Resource row stays open',
                new_cases=len(cases), new_contexts=2*len(cases),
                affected_previous_cases=1, affected_previous_contexts=2,
                previous_cases_reused=44, cumulative_cases=45+len(cases),
                cumulative_contexts=90+2*len(cases),
                new_writable_property_comparisons=count,
                cumulative_writable_property_comparisons=count+parent['writable_property_comparisons'],
                original_row='FRC!E132', new_requirement_credit=[], goal_complete=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--evidence', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(audit(args.repo.resolve(), args.evidence)))
