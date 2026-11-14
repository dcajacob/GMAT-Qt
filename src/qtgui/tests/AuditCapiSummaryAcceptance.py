"""Audit source-versioned external-C command summaries and their cached results.

Credit is for retrieving the entire configured command graph, including native
unavailable records. Scientific fields/domains beyond the independent oracle
below remain separate qualification gates.
"""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import tarfile
import xml.etree.ElementTree as ET

from AuditCapiAcceptance import load, require, sha, near
import AuditCapiStatusAcceptance as status
import NasaGuiCoverage as nasa

KEYS = ['All Others!E256', 'All Others!E257']
NAMES = 'Before ResetCounter OuterLoop BranchChoice FirstArc Alternate SecondArc CloseChoice CloseLoop CounterLoop Increment CloseCounter NeverChoice Unexecuted CloseNever ScriptBlock ScriptValue CloseScript After LateState LateMass Late'.split()
TYPES = 'Report GMAT For If Propagate Else Propagate EndIf EndFor While GMAT EndWhile If Propagate EndIf BeginScript GMAT EndScript Report GMAT GMAT Report'.split()
LABELS = ['X', 'Y', 'Z', 'VX', 'VY', 'VZ', 'Cd', 'Dry mass']
CHECKPOINTS = [('Before', 0, 0, 800), ('FirstArc', 10, 0, 800),
               ('SecondArc', 20, 0, 800), ('After', 20, 0, 800),
               ('LateState', 20, 1234, 800), ('Late', 20, 1234, 991)]
SOURCE = 'plugins/CInterfacePlugin/src/plugin/CInterfacePluginFunctions.cpp'


def function(text, signature):
    """These C API functions contain no braces in strings/comments."""
    start = text.index(signature); body = text.index('{', start); depth = 1; end = body+1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}'); end += 1
    return text[start:end]


def expected(seconds, shift, mass, satellite):
    if satellite:
        return [0, 9000, 0, 0, 0, math.sqrt(398600.4415/9000), 1.75, 950]
    w = math.sqrt(398600.4415/7000**3)
    return [7000*math.cos(w*seconds)+shift, 7000*math.sin(w*seconds), 0,
            -7000*w*math.sin(w*seconds), 7000*w*math.cos(w*seconds), 0, 2.5, mass]


def summaries(path, mode):
    raw = path.read_text()
    blocks = [b for b in raw.split('\n-----------------------------------\n') if b.strip()]
    identities = [('BeginMissionSequence', 'Unnamed'), *zip(TYPES, NAMES)]
    if mode == 'stopped': identities.insert(6, ('Stop', 'Halt'))
    if mode == 'runtimeerror': identities[6:6] = [('GMAT', 'BadArgument'), ('GMAT', 'RuntimeError')]
    require(len(blocks) == len(identities), 'Missing/duplicate command summary record')
    result = {}
    for block, (kind, name) in zip(blocks, identities):
        display = {'GMAT': 'Equation', 'EndScript': 'ScriptEvent'}.get(kind, kind)
        titles = re.findall(r'^\s*(?:([A-Za-z]+) Command: (\w+)|No summary data for ([A-Za-z]+) command (\w+))\s*$', block, re.M)
        require(len(titles) == 1 and titles[0] in [(display, name, '', ''), ('', '', kind, name)], 'Wrong command type/name/order')
        result[name] = block
    unavailable = ['Unexecuted'] if mode == 'summary' else ['SecondArc', 'After', 'Unexecuted']
    for name in unavailable:
        require('did not execute due to control logic statements' in result[name]
                and 'Spacecraft       :' not in result[name], 'Unexecuted command contains stale cached state')
    return raw, result


def cached_values(blocks, matrix, checkpoints):
    require(len(matrix) == len(checkpoints), 'Incomplete cached checkpoint matrix')
    for observed, (name, seconds, shift, mass) in zip(matrix, checkpoints):
        require(len(observed) == 2, 'Missing second spacecraft')
        parts = re.split(r'Spacecraft\s+: ([AB])\n', blocks[name])
        require(len(parts) == 5 and parts[1::2] == ['A', 'B'], 'Missing/duplicate cached spacecraft')
        for sat, part in enumerate(parts[2::2]):
            require('Coordinate System: EarthMJ2000Eq' in part and len(observed[sat]) == 8, 'Wrong cached frame/dimension')
            for i, label in enumerate(LABELS):
                values = re.findall(r'^\s*'+re.escape(label)+r'\s*=\s*([-+\d.eE]+)', part, re.M)
                require(len(values) == 1, 'Missing/duplicate cached field: '+label)
                value = float(values[0]); oracle = expected(seconds, shift, mass, sat)[i]
                require(value == observed[sat][i] and near(value, oracle, 2e-5 if i<3 else 4e-8 if i<6 else 1e-7), 'Cached field differs from independent oracle: '+name+' '+label)


def reports(path, first_only=False):
    rows = list(csv.reader(path.open()))
    require(len(rows) == (1 if first_only else 3), 'Wrong independent report row count')
    for i, row in enumerate(rows):
        seconds = 20 if i else 0
        a = expected(seconds, 1234 if i == 2 else 0, 800, 0)
        b = expected(0, 0, 950, 1)
        oracle = [seconds, *a[:6], 0, *b[:6], 42 if i else 0]
        require(len(row) == 15 and all(near(float(x), y, 1e-4 if j == 0 else 1e-12 if j == 14 else 2e-5)
                    for j, (x, y) in enumerate(zip(row, oracle))), 'Report differs from independent circular/recovery oracle')


def execution(repo, run, mode, review, artifacts):
    before, after, receipt = (load(run/p) for p in ('inputs-before.json', 'inputs-after.json', 'receipt.json'))
    require(before == after and receipt['inputs_unchanged'] is True, 'C API execution inputs changed')
    require(receipt['mode'] == 'native-C-API' and receipt['status'] == 'passed' and receipt['returncode'] == 0
            and receipt['GUI_executions'] == receipt['screenshots'] == receipt['runtime_copies'] == 0, 'Missing real successful C client')
    require(all(receipt.get(k) is False for k in ('full_current_build_qualified', 'full_NASA_qualified', 'technical_core_approval')), 'C client exceeds qualification scope')
    cmd = receipt['command']; images = receipt['loaded_project_images']
    require(len(cmd) == 4 and cmd[2] == mode and cmd[0] in images and cmd[3] in images
            and set(images) <= set(receipt['binaries']) and any(Path(p).name.startswith('libCInterface.so') for p in images), 'Actual C client/library/observer not bound')
    require(all(before[p] == d for p, d in {**receipt['binaries'], **images}.items()), 'Changed pre-execution image identity')
    needed = [run/p for p in ('inputs-before.json', 'inputs-after.json', 'receipt.json', 'client.log', 'junit.xml', 'sources.tar.gz')]
    needed += [Path(cmd[1]), run/'session/gmat_startup_file.txt', run/'session/output/cache.csv']
    if mode == 'summary': needed += [run/'session/summary-first.txt']
    else: needed += [run/('session/'+p) for p in ('recovery.script', 'summary-stopped.txt', 'summary-recovered.txt', 'cache-before-recovery.csv')]
    require(all(str(p) in artifacts for p in needed), 'Qualification omitted an actual proof artifact')
    for p in [Path(cmd[1]), run/'session/gmat_startup_file.txt', *([run/'session/recovery.script'] if mode != 'summary' else [])]:
        require(before[str(p)] == sha(p), 'Changed pre-execution script/startup')
    with tarfile.open(run/'sources.tar.gz') as bundle:
        require({m.name for m in bundle.getmembers()} == set(receipt['sources']), 'Changed archived source partition')
        frozen = {}
        for name, digest in receipt['sources'].items():
            require(not Path(name).is_absolute() and '..' not in Path(name).parts, 'Unsafe source path')
            data = bundle.extractfile(name).read()
            require(not data.startswith(b'\x7fELF') and hashlib.sha256(data).hexdigest() == digest == before[str(repo/name)], 'Changed archived source')
            frozen[name] = data
    for name, digest in review['execution_source_sha256'][mode].items():
        require(hashlib.sha256(frozen[name]).hexdigest() == digest, 'Original reviewed assertion/implementation changed')
    code = frozen[SOURCE].decode()
    require(sha(run/'sources.tar.gz') == review['execution_archive_sha256'][mode], 'Unreviewed source execution version')
    require(hashlib.sha256(function(code, 'const char *GetRunSummary()\n   {').encode()).hexdigest() == review['summary_body_sha256'], 'Summary implementation differs between contexts')
    client = frozen['src/qtgui/tests/NasaCapiSummaryTests.c'].decode()
    for token in ['CHECK(total==23', 'before==after', 'CHECK(cached(raw,4,10,0,800,1)==0)',
                  'CHECK(cached(raw,19,20,1234,800,4)==0)', 'CHECK(strcmp(GetRunSummary(),raw)==0)',
                  'CHECK(CountObjects()==count)', 'CHECK(report()==0)', 'CHECK(fabs(actual-expected[i])<=tol)']:
        require(token in client, 'Original summary assertion removed: '+token)
    probe = frozen['src/qtgui/tests/NasaCapiSummaryMetadataProbe.cpp'].decode()
    require('GetSummaryName()' in probe and 'GetNextWhileExecuting()' in probe and 'GetStringParameter' not in probe, 'Observer supplies results or omits metadata')
    cases = ET.parse(run/'junit.xml').getroot().findall('testcase')
    require(len(cases) == 1 and cases[0].get('name') == 'QtAcceptance.NasaCInterface'+mode.title()
            and not cases[0].findall('failure') and not cases[0].findall('skipped'), 'Missing exact passing JUnit')
    proofs = [json.loads(s[len('NASA_CHECK '):]) for s in (run/'client.log').read_text().splitlines() if s.startswith('NASA_CHECK ')]
    require(len(proofs) == 1 and receipt['proofs'] == proofs, 'Receipt differs from actual native proof')
    p = proofs[0]; count = {'summary': 23, 'stopped': 24, 'runtimeerror': 25}[mode]
    require(p['checkpoint'] == 'c-api-summary'+('' if mode == 'summary' else '-'+mode) and p['outcome'] == 'passed'
            and p['assertions'] >= (790 if mode == 'summary' else 891) and p['commands'] == count and p['named_commands'] == count-1
            and p['metadata_unchanged'] is True and p['cached_fields'] == (96 if mode == 'summary' else 128) and p['recovery_runs'] == 1, 'Incomplete C summary proof')
    raw, blocks = summaries(run/('session/summary-first.txt' if mode == 'summary' else 'session/summary-stopped.txt'), mode)
    require(len(raw.encode()) == p['summary_bytes'], 'Summary bytes differ from actual proof')
    cached_values(blocks, p['cached'], CHECKPOINTS if mode == 'summary' else CHECKPOINTS[:2])
    reports(run/'session/output/cache.csv')
    if mode != 'summary':
        require(p['status'] == (-4 if mode == 'stopped' else -5) and p['report_rows_before_recovery'] == 1, 'Wrong failed-run status')
        _, recovered = summaries(run/'session/summary-recovered.txt', 'summary')
        cached_values(recovered, p['recovered_cached'], CHECKPOINTS)
        reports(run/'session/cache-before-recovery.csv', True)
    return p


def qualify(repo, path, inventory):
    q = load(path)
    require(q['contract_kind'] == 'source-versioned-native-C-API-summary' and q['qualified_keys'] == KEYS, 'Unsupported/duplicate summary credit')
    require(all(q.get(k) is False for k in ('full_current_build_qualified', 'full_NASA_qualified', 'technical_core_approval')), 'Summary proof exceeds qualification scope')
    require(q['authority_sha256'] == inventory['sources'], 'NASA authority changed')
    for p, digest in q['artifact_sha256'].items(): require(sha(p) == digest, 'Changed summary artifact: '+p)
    review = load(q['review']); known = {r['key']: r for r in inventory['requirements']}
    require(review['authority_sha256'] == inventory['sources'] and [r['key'] for r in review['reviews']] == KEYS, 'Incomplete NASA conjunction review')
    for r in review['reviews']:
        require(r['fingerprint'] == known[r['key']]['fingerprint'] and r['source_text'] == known[r['key']]['text'] and r['rationale'], 'Changed NASA source row')
    require(review['parent_children'] == ['All Others!E257', 'All Others!E258'] and review['structural_family_review'], 'Parent conjunction incomplete')
    status.qualify(repo, Path(q['status_qualification']), inventory)
    require(set(q['runs']) == {'summary', 'stopped', 'runtimeerror'}, 'Missing normal/Stop/runtime-error context')
    for mode, run in q['runs'].items(): execution(repo, Path(run), mode, review, q['artifact_sha256'])
    return q, KEYS


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    p.add_argument('--qualification', type=Path, required=True)
    a = p.parse_args(); _, keys = qualify(a.repo.resolve(), a.qualification, nasa.inventory(a.repo.resolve()))
    print(json.dumps(dict(status='passed', qualified_keys=keys, new_engine_or_GUI_executions=0)))
