"""Admit only the public spacecraft epoch and its three-child C-API parent.

This is native C ABI capability evidence at recorded source versions. Original
DEFERRED wording remains literal; no NASA disposition or MATLAB approval follows.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile
import xml.etree.ElementTree as ET

import AuditCapiAcceptance as capi
import NasaGuiCoverage as nasa

sha, load, require = capi.sha, capi.load, capi.require
KEYS = ['All Others!E269', 'All Others!E272']
CHILDREN = ['All Others!E270', 'All Others!E271', 'All Others!E272']


def unchanged_legacy_interface(earlier, current):
    """Permit only the separately tested, newly added getter body to differ.

    Existing C clients do not call it. Every preexisting interface function,
    include, global and availability update must remain byte-identical.
    """
    def outside_getter(code):
        marker = '   int GetSpacecraftEpoch('
        require(code.count(marker) == 1, 'Missing/ambiguous new getter definition')
        start = code.index(marker)
        opening = code.index('{', start)
        require(code[start:opening].strip() ==
                'int GetSpacecraftEpoch(const char *spacecraftName, double *epochA1Mjd)',
                'Changed new getter signature')
        # Ignore braces in ordinary C++ literals/comments; require the very
        # first complete function body to end before the next source section.
        lexer = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]')
        depth, end = 0, None
        for token in lexer.finditer(code, opening):
            if token[0] == '{':
                depth += 1
            elif token[0] == '}':
                depth -= 1
                if depth == 0:
                    end = token.end()
                    break
        require(end is not None, 'Unbounded new getter source partition')
        section = code.index('   //---------------------------------------------------------------------------', end)
        require(not code[end:section].strip(), 'Neighboring code omitted by getter partition')
        return code[:start] + code[section:]
    require(outside_getter(earlier) == outside_getter(current),
            'Previously tested interface code changed outside the new getter')


def validate_readings(readings, proof):
    a, b = 21545.125, 21600.875
    expected = [('post-run', 'A', a + 60 / 86400),
                ('post-run', 'B', b + 30 / 86400),
                ('unpropagated', 'C', 22000.0625)]
    expected += [('selected-model', name, value) for _ in range(2)
                 for name, value in [('B', b + 30 / 86400), ('A', a + 60 / 86400)]]
    expected += [('repeat', 'A', a + 60 / 86400), ('repeat', 'B', b + 30 / 86400),
                 ('prepare', 'A', a), ('prepare', 'B', b),
                 ('stopped', 'A', a + 10 / 86400), ('stopped', 'B', b),
                 ('runtime-error', 'A', a + 10 / 86400), ('runtime-error', 'B', b),
                 ('reload-backwards', 'A', 23000.25 - 20 / 86400),
                 ('reload', 'B', 24000.5),
                 ('mission-assignment', 'A', 25000.375),
                 ('mission-assignment', 'B', 26000.625),
                 ('restart', 'A', a + 60 / 86400), ('restart', 'B', b + 30 / 86400)]
    require(len(readings) == len(expected) == 21, 'Incomplete epoch lifetime scenarios')
    for row, (stage, name, value) in zip(readings, expected):
        require(row['stage'] == stage and row['spacecraft'] == name and
                row['expected'] == value and math.isfinite(row['actual']) and
                abs(row['actual'] - value) <= 1e-10, 'Independent named-spacecraft epoch differs')
    require(proof['checkpoint'] == 'c-api-spacecraft-epoch' and
            proof['outcome'] == 'passed' and proof['assertions'] >= 190 and
            proof['readings'] == 21 and proof['run_calls'] == 9 and
            proof['units'] == 'A1ModJulian' and proof['public_ABI'] is True and
            proof['private_probe'] is False and proof['read_only'] is True,
            'Incomplete actual public C epoch checkpoint')


def qualify(repo, path, inventory):
    q = load(path)
    require(q['contract_kind'] == 'source-versioned-public-C-API-spacecraft-epoch' and
            q['qualified_keys'] == KEYS and q['parent_children'] == CHILDREN,
            'Changed finite post-run access scope')
    require(q['authority_sha256'] == inventory['sources'], 'Changed NASA authority')
    require(all(q.get(k) is False for k in ('technical_core_approval',
            'full_current_build_qualified', 'full_NASA_qualified')), 'Epoch qualification exceeds scope')
    run = Path(q['run'])
    work = Path(load(run / 'receipt.json')['cwd'])
    required = {str(run / name) for name in ('receipt.json', 'inputs-before.json',
                'inputs-after.json', 'sources.tar.gz', 'client.log', 'junit.xml')}
    required.update(str(work / name) for name in ('acceptance.script', 'prepare.script',
                    'stopped.script', 'runtime-error.script', 'initialization-error.script',
                    'replacement.script', 'assigned.script', 'gmat_startup_file.txt'))
    required.update([q['review'], q['independent_review'],
                     *q['existing_children'].values(), *q['affected_children'].values()])
    require(required <= set(q['artifact_sha256']), 'Essential epoch artifacts omitted from seal')
    for name, digest in q['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed public epoch artifact: ' + name)
    known = {r['key']: r for r in inventory['requirements']}
    review = load(q['review'])
    require(review['parent_children'] == CHILDREN and review['deferred_wording_preserved'] is True,
            'Changed parent inheritance or original deferred obligation')
    require(len(review['reviews']) == 4 and
            {row['key'] for row in review['reviews']} == set(KEYS + CHILDREN),
            'Missing original parent/child review')
    for row in review['reviews']:
        require(row['fingerprint'] == known[row['key']]['fingerprint'] and
                row['source_text'] == known[row['key']]['text'] and row['rationale'],
                'Changed original C API obligation')
    run = Path(q['run'])
    receipt, before, after = (load(run / name) for name in
                             ('receipt.json', 'inputs-before.json', 'inputs-after.json'))
    require(before == after and receipt['inputs_unchanged'] is True and
            receipt['status'] == 'passed' and receipt['returncode'] == 0 and
            receipt['mode'] == 'native-C-API' and
            receipt['GUI_executions'] == receipt['screenshots'] == receipt['runtime_copies'] == 0,
            'Missing unchanged native C execution')
    require(all(receipt.get(k) is False for k in ('technical_core_approval',
            'full_current_build_qualified', 'full_NASA_qualified')), 'Epoch receipt exceeds scope')
    command = receipt['command']
    require(len(command) == 3 and command[2] == 'spacecraftepoch' and
            command[0] in receipt['loaded_project_images'], 'Public C consumer not loaded/bound')
    require(receipt['loaded_project_images'] and
            set(receipt['loaded_project_images']) <= set(receipt['binaries']) and
            any(Path(p).name.startswith('libCInterface.so') for p in receipt['loaded_project_images']),
            'Actual C interface library missing')
    for p, digest in {**receipt['binaries'], **receipt['loaded_project_images']}.items():
        require(before[p] == digest, 'Loaded epoch image differs from pre-run identity')
    work = Path(receipt['cwd'])
    for name in ('acceptance.script', 'prepare.script', 'stopped.script',
                 'runtime-error.script', 'initialization-error.script',
                 'replacement.script', 'assigned.script', 'gmat_startup_file.txt'):
        require(before[str(work / name)] == sha(work / name), 'Changed executed epoch fixture: ' + name)
    frozen = {}
    with tarfile.open(run / 'sources.tar.gz') as bundle:
        require({m.name for m in bundle.getmembers()} == set(receipt['sources']),
                'Epoch source archive partition differs')
        for name, digest in receipt['sources'].items():
            require(not Path(name).is_absolute() and '..' not in Path(name).parts,
                    'Unsafe epoch source member')
            data = bundle.extractfile(name).read()
            require(not data.startswith(b'\x7fELF') and
                    hashlib.sha256(data).hexdigest() == digest == before[str(repo / name)],
                    'Unbound epoch source member: ' + name)
            frozen[name] = data
    client = frozen['src/qtgui/tests/NasaCapiSpacecraftEpochTests.c'].decode()
    for token in ('#ifdef __cplusplus', '#include "CInterfaceFunctions.h"',
                  'dlsym(RTLD_DEFAULT,"GetSpacecraftEpoch")',
                  'GetSpacecraftEpoch(NULL,&sentinel)==-1',
                  'GetSpacecraftEpoch("A",&sentinel)==-2',
                  'GetSpacecraftEpoch(invalid[i],&sentinel)==-3',
                  'memcmp(state,GetState(),sizeof(state))==0',
                  'run("initialization-error.script",-2)',
                  'epoch("mission-assignment","A",25000.375)',
                  'StartGmat()==0 && unavailable()==0'):
        require(token in client, 'Missing actual epoch boundary assertion: ' + token)
    core = frozen['plugins/CInterfacePlugin/src/plugin/CInterfacePluginFunctions.cpp'].decode()
    require('theModerator->GetInternalObject(spacecraftName)' in core and
            'spacecraft->GetEpoch()' in core,
            'Getter does not read actual named runtime spacecraft')
    lines = (run / 'client.log').read_text().splitlines()
    proofs = [json.loads(s[11:]) for s in lines if s.startswith('NASA_CHECK ')]
    readings = [json.loads(s[11:]) for s in lines if s.startswith('NASA_EPOCH ')]
    require(len(proofs) == 1 and receipt['proofs'] == proofs, 'Epoch log/receipt proof differs')
    validate_readings(readings, proofs[0])
    cases = ET.parse(run / 'junit.xml').getroot().findall('testcase')
    require(len(cases) == 1 and cases[0].get('name') == 'QtAcceptance.NasaCInterfaceSpacecraftepoch'
            and not cases[0].findall('failure') and not cases[0].findall('skipped'),
            'Actual epoch C JUnit failed or omitted')
    # The affected state/STM clients guard the added lifecycle bookkeeping.
    # Reuse those passes if a later correction changes ONLY the new getter
    # they never call. Its own V4 client separately checks that correction.
    for mode, child in [('basic', CHILDREN[0]), ('stm', CHILDREN[1])]:
        old, keys = capi.qualify(repo, Path(q['existing_children'][mode]), inventory)
        require(old['case'] == mode and child in keys, 'Unqualified original post-run child')
        affected, keys = capi.qualify(repo, Path(q['affected_children'][mode]), inventory)
        require(affected['case'] == mode and child in keys, 'Affected state/STM regression missing')
        other = load(Path(affected['run']) / 'receipt.json')
        with tarfile.open(Path(affected['run']) / 'sources.tar.gz') as bundle:
            old_interface = bundle.extractfile('plugins/CInterfacePlugin/src/plugin/CInterfacePluginFunctions.cpp').read().decode()
            old_header = bundle.extractfile('plugins/CInterfacePlugin/src/plugin/CInterfaceFunctions.h').read()
            old_client = bundle.extractfile('src/qtgui/tests/NasaCapiAcceptanceTests.c').read().decode()
        unchanged_legacy_interface(old_interface, core)
        require(old_header == frozen['plugins/CInterfacePlugin/src/plugin/CInterfaceFunctions.h'] and
                'GetSpacecraftEpoch' not in old_client,
                'Changed legacy header or affected client calls the corrected getter')
        require(other['loaded_project_images'] and all(
            other['loaded_project_images'].get(p) == digest
            for p, digest in receipt['loaded_project_images'].items()
            if Path(p).name.startswith(('libGmatBase.so', 'libGmatUtil.so'))),
            'Parent children tested different unchanged engine libraries')
        if old_interface == core:
            require(all(other['loaded_project_images'].get(p) == digest
                        for p, digest in receipt['loaded_project_images'].items()
                        if Path(p).name.startswith('libCInterface.so')),
                    'Unexplained interface binary drift at identical source')
    independent = load(q['independent_review'])
    require(independent['status'] == 'passed' and independent['qualified_keys'] == KEYS and
            independent['source_sha256'] == {name: receipt['sources'][name] for name in
            ('plugins/CInterfacePlugin/src/plugin/CInterfaceFunctions.h',
             'plugins/CInterfacePlugin/src/plugin/CInterfacePluginFunctions.cpp',
             'src/qtgui/tests/NasaCapiSpacecraftEpochTests.c',
             'src/qtgui/tests/RunCapiSpacecraftEpochAcceptance.py')},
            'Independent epoch scope/core review missing or stale')
    return q, KEYS


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    p.add_argument('--qualification', type=Path, required=True)
    a = p.parse_args()
    _, keys = qualify(a.repo.resolve(), a.qualification, nasa.inventory(a.repo.resolve()))
    print(json.dumps(dict(status='passed', qualified_keys=keys, new_native_executions=0)))
