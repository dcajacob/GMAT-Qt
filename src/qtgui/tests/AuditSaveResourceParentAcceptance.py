"""Admit the complete original Save resource row using only immutable evidence."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import tarfile

import AuditNasaCoveragePrefix as prefix
import BuildNasaCoverageExtension as common
import NasaGuiCoverage as nasa
from AuditSaveResourceEvidence import compare

sha, load, require = common.sha, common.load, common.require
KEYS = ['FRC!E132']
KIND = 'source-versioned-complete-any-resource-save'
GATES = ['scientific_approval', 'technical_core_approval', 'full_current_build_qualified',
         'full_NASA_qualified', 'native_OS_input_qualified']


def check_scope(scope, report, row):
    require(scope['original_source'] == row and scope['qualified_keys'] == KEYS
            and scope['original_assertions_changed'] == 0 and not scope['failures_waived'],
            'Original Save requirement identity or bounds changed')
    require(row['text'] == 'The system shall have a command named Save that allows users to save any Resource object to a file.',
            'Original resource quantifier changed')
    require(report['status'] == 'passed' and report['source_versioned_only'] is True,
            'Resource evidence scope differs')
    menu = report['gui_catalogue_names']
    named = report['named_script_create_and_save']
    require(len(menu) == 85 and all(menu.values()) and len(named) == 17
            and all(len(p) == 2 for p in named.values()), 'Missing actual named resource persistence')
    require(len(report['named_script_aliases']) == 19 and all(z['configured'] and z['build_accepted']
            for z in report['named_script_aliases']), 'Script resource route unconfirmed')
    require(report['default_style_failures_preserved'] == 177
            and len(report['descriptor_roles']) == 156
            and all(z['owners'] or z['consumer'] for z in report['descriptor_roles']),
            'Incomplete descriptor roles or failures lost')
    require(all(scope[k] is False for k in GATES), 'Unrelated qualification promoted')


def qualify(repo, path, inventory):
    q = load(path)
    require(q['contract_kind'] == KIND and q['qualified_keys'] == KEYS
            and q['authority_sha256'] == inventory['sources']
            and all(q[k] is False for k in GATES), 'Changed Save authority')
    for name, digest in q['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed Save artifact: ' + name)
    report, scope = load(q['conjunction']), load(q['scope_review'])
    row = next(x for x in inventory['requirements'] if x['key'] == KEYS[0])
    check_scope(scope, report, row)
    bound = report['artifact_sha256']
    for name, digest in bound.items():
        require(sha(name) == digest, 'Changed actual Save observation: ' + name)
    frozen = {}
    for archive, ids in [('reader_source_archive', 'reader_source_sha256'),
                         ('proof_source_archive', 'proof_source_sha256')]:
        with tarfile.open(q[archive]) as bundle:
            require(set(bundle.getnames()) == set(q[ids]), 'Changed Save source partition')
            for name, digest in q[ids].items():
                raw = bundle.extractfile(name).read()
                require(hashlib.sha256(raw).hexdigest() == digest and not raw.startswith(b'\x7fELF'),
                        'Changed capsule member: ' + name)
                if ids == 'proof_source_sha256':
                    frozen[name] = raw
    original, authorized = load(q['protected_input_baseline']), load(q['authorized_input_manifest'])
    require(len(original) == len(authorized) == 1334 and all(q['proof_source_sha256'][n] == h
            for n, h in original.items()), 'Original protected Save inputs lost')
    changed = sorted(n for n in original if original[n] != authorized[n])
    require(changed == ['src/qtgui/tests/PlotIntegrationStepTests.cpp', 'src/qtgui/tests/SharedPlaybackTests.cpp']
            and all(hashlib.sha256(frozen[q['observer_preimages'][n]]).hexdigest() == authorized[n] for n in changed),
            'Authorized observer amendments lost')
    for review in report['descriptor_roles']:
        for owner in review['owners']:
            n = q['save_source_versions'][owner['path']]
            source = frozen[n].decode().splitlines()
            require(hashlib.sha256(frozen[n]).hexdigest() == owner['sha256']
                    and all(owner['anchor'] in source[i-1] for i in owner['lines']),
                    'Unbound descriptor source role: ' + owner['path'])
    for profile, count in [('final-offscreen-v1', 185), ('final-visible-v1', 186)]:
        raw = report['cohorts'][profile]
        require(raw == dict(passes=count, typed_snapshots=172, independent_writable_checks=6488),
                'Changed historical Save cohort')
    def inspect(folder, minimum=1):
        folder = Path(folder)
        a,b = folder/'runtime-native.json',folder/'reloaded-native.json'
        require(str(a) in bound and str(b) in bound, 'Unbound Save native snapshots')
        before,after=load(a),load(b)
        require(before.keys() == after.keys(), 'Lost saved resource')
        require(sum(compare(before[n],after[n],n) for n in before) >= minimum,
                'Missing actual native property comparison')
        return folder
    for name, evidence in report['named_script_create_and_save'].items():
        for directory in evidence:
            folder = inspect(directory, minimum=int(name not in scope['configuration_via_literals']))
            x = load(folder/'result.json')
            require(x['status']=='passed' and x['returncode']==0 and x['proofs'][0]['witnesses'],
                    'Missing actual named Save execution')
    for key, evidence in report['cohorts'].items():
        if isinstance(evidence,str):
            inspect(evidence)
    require(scope['configuration_via_literals'] == ['PointMassForce','FiniteThrust','RelativisticCorrection','ExternalModel'],
            'Named force scope changed')
    for key, replacement in [('descriptor_roles',[]),('default_style_failures_preserved',0),('gui_catalogue_names',{})]:
        invalid=copy.deepcopy(report)
        invalid[key]=replacement
        try:
            check_scope(scope,invalid,row)
        except ValueError:
            continue
        raise ValueError('Incomplete Save evidence accepted: '+key)
    return q, KEYS, dict(status='passed', qualified_keys=KEYS, whole_row_conjunction=True,
        component=dict(reused_headless=185, reused_visible=186, menu_resources=85, named_script_types=17,
                       named_script_aliases=2, exceptional_descriptor_identities=156,
                       protected_inputs=1334, retained_default_style_failures=177),
        new_requirement_credit=1, newly_resolved_original_hold_keys=[], damaged_admission_controls_rejected=3,
        native_tests_rerun=0, runtime_binary_copies=0, **{k:False for k in GATES})


def build(repo, baseline, path, output):
    seal, inventory, keys = prefix.baseline(repo, baseline)
    original = {kind:set(values) for kind,values in keys.items()}
    _, incoming, audit = qualify(repo, path, inventory)
    known = {kind:{row['key']:row for row in inventory[kind]} for kind in keys}
    require(not set(incoming) & keys['requirements'], 'Duplicate scope credit')
    keys['requirements'] = common.merge_keys(keys['requirements'], incoming, known['requirements'])
    coverage = {kind:{'verified':len(keys[kind]), 'total':len(known[kind]),
        'percent':round(100*len(keys[kind])/len(known[kind]),1),
        'new_verified':len(keys[kind])-len(original[kind])} for kind in keys}
    remaining = {kind:[row for row in seal['remaining_original_rows'][kind]
        if row['key'] not in keys[kind]] for kind in keys}
    result = {'status':'passed', 'scope':'Sealed source-versioned prefix plus complete original any-Resource Save conjunction',
        'baseline':str(baseline), 'baseline_sha256':sha(baseline), 'authority_sha256':inventory['sources'],
        'coverage':coverage, 'target_requirements':1101, 'target_reached':len(keys['requirements'])>=1101,
        'requirements_still_needed':max(0,1101-len(keys['requirements'])),
        'additions':[{'key':key, 'kind':'requirements', 'fingerprint':known['requirements'][key]['fingerprint'],
            'source_text':known['requirements'][key]['text'], 'qualification':str(path),
            'qualification_sha256':sha(path), 'status':'verified-source-versioned'} for key in incoming],
        'remaining_counts':{kind:len(rows) for kind,rows in remaining.items()},
        'remaining_original_rows':remaining, 'technical_core_approval':False,
        'full_current_build_qualified':False, 'full_NASA_qualified':False,
        'limits':['Original 1158/2367 denominators, texts and fingerprints are unchanged.',
                  'Only FRC E132 is added. All other row records and all59 formal hold statuses remain unchanged.',
                  'Exact named-resource routes, native typed before/after observations and meaningful initialization consumers establish Save. Default filename policy failures and broader qualification remain separate.']}
    with Path(output).open('x') as file:
        json.dump(result, file, indent=2); file.write('\n')
    return result, audit


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.baseline:
        result, _ = build(args.repo, args.baseline, args.qualification, args.output)
        print(json.dumps({'coverage':result['coverage'], 'still_needed':result['requirements_still_needed']}))
    else:
        _, _, result = qualify(args.repo, args.qualification, nasa.inventory(args.repo))
        with args.output.open('x') as file:
            json.dump(result, file, indent=2); file.write('\n')
        print(json.dumps(result))
