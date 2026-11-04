"""Execute untested native conversion routes and check independent references.

No GUI credit, screenshots or runtime copies. Preserve failed native attempts
and their source versions; source-prototype parity is not averaging science.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tarfile
import uuid
import xml.etree.ElementTree as ET

import BrouwerPrototypeOracle as brouwer


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def run(repo, executable, output):
    output = output.resolve(); output.mkdir(parents=True, exist_ok=False)
    work = output/'session'; work.mkdir()
    fixture = repo/'src/qtgui/tests/NasaDirectOrbitConversionCases.json'
    matrix = json.loads(fixture.read_text())
    sources = {p for p in (repo/'src/gmatutil').rglob('*')
               if p.is_file() and p.suffix in ('.c', '.cpp', '.h', '.hpp', '.inc')}
    sources.update(repo/name for name in matrix['reference_sha256'])
    sources.update([fixture, Path(__file__), repo/'src/qtgui/tests/NasaDirectOrbitConversionTests.cpp',
                    repo/'src/qtgui/CMakeLists.txt'])
    binaries = {executable.resolve(), (repo/'application/bin/libGmatUtil.so').resolve()}
    inputs = sorted(sources | binaries)
    before = {str(p): sha(p) for p in inputs}
    (output/'inputs-before.json').write_text(json.dumps(before, indent=2)+'\n')
    with tarfile.open(output/'sources.tar.gz', 'w:gz') as archive:
        for p in sorted(sources): archive.add(p, arcname=str(p.relative_to(repo)), recursive=False)
    command = [str(executable.resolve()), str(fixture), str(repo)]
    result = subprocess.run(command, cwd=work, env=dict(os.environ, GMAT_QT_ACCEPTANCE_DIR=str(work)),
                            capture_output=True, text=True, timeout=60)
    (output/'client.log').write_text(result.stdout+result.stderr)
    after = {str(p): sha(p) for p in inputs}
    (output/'inputs-after.json').write_text(json.dumps(after, indent=2)+'\n')
    proofs = [json.loads(s[11:]) for s in result.stdout.splitlines() if s.startswith('NASA_CHECK ')]
    loaded = sorted({s[12:] for s in result.stdout.splitlines()
                     if s.startswith('NASA_LOADED ') and s[12:].startswith(str(repo)+'/')})
    functions = {kind: brouwer.adapt(repo/'prototype/StateConv'/name)[0] for kind, name in
                 [('BrouwerMeanShort', 'BROLYD_short.m'), ('BrouwerMeanLong', 'BROLYD.m')]}
    independent = []
    if len(proofs) == 1:
        cases = {c['key']: c for c in matrix['cases']}
        for row in proofs[0]['results']:
            if row['target'] not in functions: continue
            observed = dict(case=row['case'], source=row['source'], target=row['target'])
            try:
                spec = cases[row['case']]
                actual = brouwer.cartesian(functions[row['target']](row['actual']), spec['mu'])
                errors = [abs(x-y) for x, y in zip(actual, spec['cartesian'])]
                observed.update(cartesian=actual, errors=errors,
                    passed=len(errors)==6 and all(e <= (1e-6 if i<3 else 1e-9)
                                                 for i, e in enumerate(errors)))
            except Exception as error:
                observed.update(passed=False, error=str(error))
            independent.append(observed)
    (output/'independent-reconstruction.json').write_text(json.dumps(independent, indent=2)+'\n')
    ok = (result.returncode == 0 and before == after and len(proofs) == 1 and
          proofs[0]['outcome'] == 'passed' and len(independent) == 26 and
          all(r['passed'] for r in independent) and loaded and set(loaded) <= set(before))
    suite = ET.Element('testsuite', name='DirectOrbitConversion', tests='1', failures=str(int(not ok)))
    case = ET.SubElement(suite, 'testcase', name='QtAcceptance.NasaDirectOrbitConversion')
    if not ok: ET.SubElement(case, 'failure', message='Native dispatch or independent reconstruction failed')
    ET.ElementTree(suite).write(output/'junit.xml', encoding='unicode')
    receipt = dict(status='passed' if ok else 'failed', mode='native-library-math', command=command,
        returncode=result.returncode, inputs_unchanged=before==after,
        source_sha256={str(p.relative_to(repo)):before[str(p)] for p in sources},
        binary_sha256={str(p):before[str(p)] for p in binaries},
        loaded_project_images={p:before[p] for p in loaded if p in before}, proofs=proofs,
        independent_mean_reconstructions=len(independent),
        failed_independent_mean_reconstructions=sum(not r['passed'] for r in independent),
        GUI_executions=0, screenshots=0, runtime_copies=0,
        independent_physical_averaging_qualified=False, full_current_build_qualified=False,
        full_NASA_qualified=False, technical_core_approval=False)
    (output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], output=str(output),
        native_failed_contexts=proofs[0]['failed_contexts'] if proofs else None,
        independent_failures=receipt['failed_independent_mean_reconstructions'])))
    return 0 if ok else 1


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    p.add_argument('--executable', type=Path, required=True)
    dest = p.add_mutually_exclusive_group(required=True)
    dest.add_argument('--output', type=Path); dest.add_argument('--output-root', type=Path)
    a = p.parse_args(); output = a.output or a.output_root/('direct-conversion-'+uuid.uuid4().hex[:12])
    raise SystemExit(run(a.repo.resolve(), a.executable.resolve(), output))
