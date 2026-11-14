"""Reconcile the open exact-row queue without admitting or replaying requirements.

This is a planning/evidence overlay. The canonical ledger and every original
row remain unchanged. Four independently rerun offline plot audits are inputs;
their bounded passes do not imply complete plot or current-build qualification.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path

import NasaGuiCoverage as nasa

LEDGER = 'build/nasa-coverage/coverage95-20261103-default-orientation/coverage-snapshot-v1.json'
PRIOR = 'build/nasa-coverage/coverage95-20261103-function-special-orbit-bindings'
DOCS = 'doc/DevelopersDocs/Qt6ParityValidation/'
PLOTS = ['FRR!E329', 'FRR!E330', 'FRR!E633', 'FRR!E634']
TRACKS = {
    'plot-whole-row-conjunction': PLOTS,
    'function-command-and-grammar-conjunction':
        ['FRR!E450', 'FRR!E470', 'FRR!E472', 'FRR!E477', 'FRR!E480', 'FRC!E161'],
    'universal-science-and-scope-conjunction':
        ['FRR!E13', 'FRR!E52', 'FRR!E144', 'FRR!E491', 'FRR!E501',
         'FRR!E502', 'FRR!E525', 'FRR!E526'],
    'save-state-and-source-reconciliation': ['FRC!E132', 'FRC!E135'],
    'ephemeris-format-and-solution-conjunction':
        ['FRR!E374', 'FRR!E375', 'FRR!E377', 'FRR!E380', 'FRR!E381', 'FRR!E404', 'FRR!E406'],
    'public-C-interface-conjunction':
        ['All Others!E247', 'All Others!E249', 'All Others!E251', 'All Others!E252'],
    'additional-MarsGRAM-dependency': ['FRR!E' + str(n) for n in range(120, 126)],
    'console-interface-conjunction': ['All Others!E20'],
}
AUDITS = {
    'boundary': ('src/qtgui/tests/AuditGroundBoundaryCompatibility.py',
                 'build/nasa-coverage/coverage95-20261102-ground-boundary/evidence-manifest.json'),
    'origin': ('src/qtgui/tests/AuditPlotStepOriginEvidence.py',
               'build/nasa-coverage/coverage95-20261102-plot-step-origin/evidence-manifest.json'),
    'rotating': ('build/nasa-coverage/coverage95-20261102-plot-step-rotating/AuditPlotStepRotatingEvidence.py',
                 'build/nasa-coverage/coverage95-20261102-plot-step-rotating/evidence-manifest.json'),
    'solver-paint': ('build/nasa-coverage/coverage95-20261102-plot-step-solver-paint/AuditPlotStepSolverPaintEvidence.py',
                     'build/nasa-coverage/coverage95-20261102-plot-step-solver-paint/evidence-manifest.json'),
}
REPORTS = [
    'coverage95-20261102-ground-boundary.rst',
    'coverage95-20261102-plot-step-consumers.rst',
    'coverage95-20261102-plot-step-function.rst',
    'coverage95-20261102-plot-step-origin.rst',
    'coverage95-20261102-plot-step-rotating.rst',
    'coverage95-20261102-plot-step-tables.rst',
    'coverage95-20261102-plot-step-solver-paint.rst',
    'coverage95-20261102-save-file-attitudes.rst',
    'coverage95-20261102-save-forces.rst',
    'coverage95-20261103-function-command-scope.rst',
    'coverage95-20261103-function-scope-conjunction.rst',
    'coverage95-20261103-function-special-orbit-bindings.rst',
    'coverage95-20261103-e406-conjunction.rst',
    'coverage95-20261102-capi-configuration.rst',
]


def require(value, message):
    if not value:
        raise ValueError(message)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path):
    return json.loads(Path(path).read_text())


def review(repo, output):
    bindings = {}

    def bind(relative):
        path = repo / relative
        require(path.is_file() and not path.is_symlink(), 'Missing regular input: ' + relative)
        bindings[relative] = digest(path)
        return load(path) if path.suffix == '.json' else path.read_text()

    ledger = bind(LEDGER)
    prior = bind(PRIOR + '/stage-receipt-v1.json')
    worklist = bind(PRIOR + '/remaining-parameter-binding-worklist-v1.json')
    inventory = nasa.inventory(repo)
    require(inventory['sources'] == ledger['authority_sha256'], 'Original authority changed')
    require(len(inventory['requirements']) == 1158 and len(inventory['historical_cases']) == 2367,
            'Original denominator changed')
    require(ledger['coverage'] == prior['coverage'] and
            ledger['coverage']['requirements']['verified'] == 1019 and
            ledger['coverage']['historical_cases']['verified'] == 1816,
            'Recorded source-versioned coverage changed; review the new ledger explicitly')
    require(digest(repo / LEDGER) == prior['canonical_ledger_sha256'], 'Prior ledger binding changed')
    rows = ledger['remaining_original_rows']['requirements']
    known = {r['key']: r for r in inventory['requirements']}
    remaining = {r['key'] for r in rows}
    holds = set(prior['unchanged_prerequisite_holds'])
    require(len(rows) == len(remaining) == 139 and len(holds) == 59 and holds <= remaining,
            'Open rows or original prerequisite holds changed')
    require(worklist['all_system_parameter_count'] == 321 and
            worklist['all_system_parameters_whole_scope_open'] is True and
            len(worklist['native_bound_types']) == 298,
            'Parameter supporting scope changed')
    require(prior['matlab_licensed_installation_available'] is False,
            'MATLAB prerequisite changed; reassess rather than assume')
    hold_groups = {
        'removed-two-body': {k for k in holds if k.startswith('FRR!E') and int(k.split('E')[1]) < 239},
        'historical-OS': {k for k in holds if k.startswith('All Others!E') and 279 <= int(k.split('E')[1]) <= 287},
        'undefined-hardware': {'All Others!E' + str(n) for n in range(294, 297)},
    }
    hold_groups['MATLAB'] = holds - set().union(*hold_groups.values())
    require({k: len(v) for k, v in hold_groups.items()} ==
            {'removed-two-body': 11, 'historical-OS': 8, 'undefined-hardware': 3, 'MATLAB': 37},
            'Original hold partition changed')
    assignments = {}
    for track, keys in TRACKS.items():
        for key in keys:
            require(key in remaining and key not in assignments and key not in holds,
                    'Duplicate, held or unknown closure-track key: ' + key)
            assignments[key] = track
    for relative in [DOCS + p for p in REPORTS]:
        bind(relative)
    source_paths = [
        'src/base/subscriber/GroundTrack.cpp', 'src/base/subscriber/OrbitPlot.cpp',
        'src/base/executive/PlotOriginScope.hpp', 'src/base/forcemodel/ODEModel.cpp',
        'src/base/coordsystem/BodyFixedAxes.cpp',
        'src/base/coordsystem/ObjectReferencedAxes.cpp',
        'src/qtgui/tests/NasaGroundTrackIntegrationSkipTests.cpp',
        'src/qtgui/tests/NasaGuiCoverage.py', 'src/qtgui/tests/ReviewNasaClosureQueue.py',
    ]
    sources = {p: bind(p) for p in source_paths}
    require('integrationStepsToSkip >= 0' in sources[source_paths[0]] and
            'RepeatsPropagationStart(dat, len)' in sources[source_paths[0]],
            'Recorded explicit-step or original-boundary implementation changed')
    require('GetAttitude(theEpoch)' in sources['src/base/coordsystem/BodyFixedAxes.cpp'] and
            'GetAngularVelocity(theEpoch)' in sources['src/base/coordsystem/BodyFixedAxes.cpp'] and
            'std::map<const SpaceObject*,Rvector6> states' in sources['src/base/executive/PlotOriginScope.hpp'],
            'Review the sampled attitude boundary again')
    reviews = {}
    for name, (reader, manifest) in AUDITS.items():
        bind(reader)
        bind(manifest)
        path = output / (name + '-review-v1.json')
        data = load(path)
        require(data['bounded_evidence_status'] == 'passed' and data['new_requirement_credit'] == 0 and
                data['manifest_sha256'] == bindings[manifest], 'Invalid constituent review: ' + name)
        reviews[name] = {'review': str(path.relative_to(repo)), 'sha256': digest(path),
                         'reader': reader, 'reader_sha256': bindings[reader],
                         'manifest': manifest, 'manifest_sha256': bindings[manifest],
                         'checked': data['checked']}
    require(load(output / 'boundary-review-v1.json')['original_gate'] == 'passed at recorded sources',
            'Original enabled boundary gate is unresolved')
    reviewed = []
    for row in rows:
        key = row['key']
        require(row['fingerprint'] == known[key]['fingerprint'] and row['source_text'] == known[key]['text'],
                'Original obligation changed: ' + key)
        hold = next((kind for kind, keys in hold_groups.items() if key in keys), None)
        track = ('original-prerequisite-' + hold) if hold else assignments.get(key, 'source-or-dependent-child-reconciliation')
        action = row['closure_gate']
        notes = []
        if key in PLOTS:
            notes = ['Explicit IntegrationStepsToSkip exists; do not implement it again.',
                     'Original enabled GroundTrack boundary, coeval Cartesian translation, selected rotating point-mass frames and selected solver paints pass at recorded versions.',
                     'General sampled force/attitude inputs, further provider/reference lifetimes and remaining solver/render boundaries remain open; the four offline audits do not close these clauses.']
            action = ('First map every supported coordinate-frame dependency and sampled getter to its required coeval state. '
                      'Use that complete route census to specify the remaining capture/lifetime contract and the necessary new witnesses. '
                      'Do not launch another plot matrix or admit a row from the existing bounded passes alone.')
        elif key == 'FRR!E491':
            notes = ['298 native-bound Parameter types are supporting evidence; all 321 whole-scope obligations remain open.',
                     'The remaining 23 owner names are a backlog, not the next automatic native batch.']
            action = ('Review the complete E491 scoping conjunction and its role, mutability, reference, lifetime and science gaps '
                      'before selecting further owner probes. Require an explicit path from each new probe to a complete row.')
        elif key == 'FRR!E477':
            notes = ['The retained command-scope review keeps SaveMission function behavior and the complete command conjunction unresolved.']
        elif key == 'FRR!E406':
            notes = ['Retained terminal DC/Yukon counterexamples write ephemeris after EXCEEDED_ITERATIONS with unsatisfied controls.',
                     'More passing output-format canaries cannot decide the required solution policy.']
        elif key == 'FRC!E132':
            notes = ['131 Save cases and the offered type census remain partial persistence evidence.',
                     'Hidden persistent fields, reference graphs, modes and post-rebuild initialization remain open.']
        elif key in TRACKS['additional-MarsGRAM-dependency']:
            notes = ['These six additional availability gaps are outside the preserved 59-hold partition; do not conceal them in the optimistic ceiling.']
        reviewed.append({'key': key, 'original_row': row, 'whole_row_status': 'open',
                         'new_credit': 0, 'closure_track': track, 'review_notes': notes,
                         'next_action': action})
    counts = dict(sorted(Counter(r['closure_track'] for r in reviewed).items()))
    result = {
        'status': 'review-complete;acceptance-incomplete',
        'scope': 'Exact-row planning overlay, not a NASA qualification or replacement ledger',
        'previous_goal_turn': 'progress: product/engine dependency evidence changed the MATLAB acquisition advice; no requirement admitted',
        'canonical_ledger': LEDGER, 'canonical_ledger_sha256': bindings[LEDGER],
        'authority_sha256': inventory['sources'], 'coverage': ledger['coverage'],
        'target': 1101, 'requirements_still_needed': 82, 'reviewed_open_rows': reviewed,
        'track_counts': counts, 'unchanged_prerequisite_holds': sorted(holds),
        'hold_counts': {k: len(v) for k, v in hold_groups.items()},
        'nonheld_open_rows': 80, 'additional_MarsGRAM_gaps': 6,
        'optimistic_ceiling_with_59_holds': 1099,
        'ceiling_is_forecast_or_proof_of_other_80': False,
        'next_closure_review': {'keys': PLOTS, 'action': reviewed[next(i for i, r in enumerate(reviewed) if r['key'] in PLOTS)]['next_action']},
        'automatic_remaining_Parameter_batch_authorized_by_this_review': False,
        'constituent_offline_reviews': reviews, 'input_sha256': bindings,
        'new_requirement_credit': 0, 'native_replays': 0, 'production_changes': [],
        'scientific_approval': False, 'technical_core_approval': False,
        'full_current_build_qualified': False, 'full_NASA_qualified': False,
    }
    target = output / 'exact-row-closure-review-v1.json'
    require(not target.exists(), 'Use a fresh review destination')
    target.write_text(json.dumps(result, indent=2) + '\n')
    return {k: result[k] for k in ['status', 'track_counts', 'new_requirement_credit', 'requirements_still_needed']}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--output-dir', type=Path, required=True,
                        help='Directory containing the four fresh constituent offline reviews')
    args = parser.parse_args()
    print(json.dumps(review(args.repo.resolve(), args.output_dir.resolve())))
