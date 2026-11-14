"""Record specialist-input inventories, or reuse unchanged receipts to audit docs.

No window is shown and no mission is executed. Raw records are compressed;
application binaries/data are used in place. Recording refuses an existing
output directory. --reuse never replays the editor constructions.
"""
import argparse
import gzip
import hashlib
import json
import os
import re
import subprocess
from pathlib import Path
from InputConstructionInventory import inventory
from AuditConfiguredInputDocumentation import audit


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def prepare_startup(template, destination, logfile):
    text = template.read_text()
    def absolute(value):
        path = Path(value.strip())
        return str(path if path.is_absolute() else (template.parent / path).resolve())
    text, count = re.subn(r'^ROOT_PATH\s*=\s*([^\n#]+)', lambda m: 'ROOT_PATH = ' + absolute(m[1]) + '/', text, flags=re.M)
    if count != 1:
        raise ValueError('Expected one ROOT_PATH in startup template')
    text = re.sub(r'^PLUGIN\s*=\s*([^\n#]+)', lambda m: 'PLUGIN = ' + (m[1].strip() if re.match(r'^[A-Z_]+/', m[1].strip()) else absolute(m[1])), text, flags=re.M)
    text, count = re.subn(r'^OUTPUT_PATH\s*=.*$', lambda m: 'OUTPUT_PATH = ' + str(destination.resolve()) + '/', text, flags=re.M)
    if count != 1:
        raise ValueError('Expected one OUTPUT_PATH in startup template')
    text, count = re.subn(r'^LOG_FILE\s*=.*$', lambda m: 'LOG_FILE = ' + str(logfile.resolve()), text, flags=re.M)
    if count != 1:
        raise ValueError('Expected one LOG_FILE in startup template')
    return text


def record(repo, executable, template, output):
    if output.exists():
        raise ValueError('Recording requires a new directory; use --reuse for retained evidence')
    output.mkdir(parents=True)
    paths = {executable.resolve(), template.resolve(), Path(__file__).resolve(), Path(__file__).with_name('ConfiguredInputCatalog.cpp'), repo / 'src/base/spacecraft/Spacecraft.cpp', repo / 'src/gmatutil/util/TimeSystemConverter.cpp'}
    paths.update(repo / name for name in inventory(repo)['source_sha256'])
    paths.update((repo / 'src/base/attitude').glob('*.cpp'))
    paths.update((repo / 'src/base/attitude').glob('*.hpp'))
    for folder in ['application/bin', 'application/plugins']:
        paths.update(p.resolve() for p in (repo / folder).rglob('*.so*') if p.is_file())
    inputs = {str(p): sha(p) for p in sorted(paths)}
    results = []
    def run(mode, name):
        startup, rawfile, logfile = [output / (name + suffix) for suffix in ['-startup.txt', '.json', '.log']]
        destination = output / 'output'
        destination.mkdir(exist_ok=True)
        startup.write_text(prepare_startup(template, destination, output / (name + '-GmatLog.txt')))
        startup_hash = sha(startup)
        command = [str(executable), str(startup), mode, str(rawfile)]
        with logfile.open('w') as stream:
            result = subprocess.run(command, env=dict(os.environ, QT_QPA_PLATFORM='offscreen', GMAT_QT_ACCEPTANCE_MODE='headless'), stdout=stream, stderr=subprocess.STDOUT, timeout=120)
        unchanged = all(sha(p) == digest for p, digest in inputs.items()) and sha(startup) == startup_hash
        entry = dict(mode=mode, returncode=result.returncode, command=command, startup_sha256=startup_hash, log_sha256=sha(logfile), inputs_unchanged=unchanged)
        document = None
        if rawfile.exists():
            raw = rawfile.read_bytes()
            document = json.loads(raw)
            compressed = output / (name + '.json.gz')
            compressed.write_bytes(gzip.compress(raw, mtime=0))
            rawfile.unlink()
            entry.update(compressed_output=str(compressed), compressed_sha256=sha(compressed), uncompressed_sha256=hashlib.sha256(raw).hexdigest(), inventoried_states=document['inventoried_states'], rejected_states=document['rejected_states'], loaded_file_images=document['loaded_file_images'], loaded_file_image_set_measured=document['loaded_file_image_set_measured'])
        results.append(entry)
        (output / 'receipt-progress.json').write_text(json.dumps(dict(scope='In-progress inventory; no qualification.', source_and_runtime_sha256=inputs, results=results), indent=2) + '\n')
        print(mode, 'exit', result.returncode, 'inventoried', entry.get('inventoried_states'), 'rejected', entry.get('rejected_states'), flush=True)
        if not unchanged:
            raise ValueError('Execution inputs changed')
        return document
    # The model set comes from the runtime, never a shortened hardcoded list.
    catalogue = run('--configured-catalog', 'catalogue')
    if catalogue is None or results[-1]['returncode'] != 0:
        raise ValueError('Cannot enumerate configured-input factories')
    for model in catalogue['attitude_models']:
        run('--attitude=' + model, 'attitude-' + model)
    run('--orbit', 'orbit-anomalies')
    run('--epochs', 'epoch-formats')
    receipt = dict(scope='Specialist/configured editor construction only; no full E12 qualification.', source_and_runtime_sha256=inputs, results=results, inputs_unchanged=all(sha(p) == digest for p, digest in inputs.items()), loaded_file_images_bound=True, processes=len(results), inventoried_states=sum(r.get('inventoried_states', 0) for r in results), rejected_states=sum(r.get('rejected_states', 0) for r in results), process_failures=sum(r['returncode'] != 0 for r in results), mission_executions=0, shown_windows=0, screenshots=0, binary_or_data_copies=0, startup_text_fixtures=len(results), GUI_behavior_qualified=False, all_input_documentation_qualified=False)
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--build-dir', type=Path, default=Path('build/linux-gui'))
    parser.add_argument('--startup', type=Path, default=Path('application/bin/gmat_startup_qt.txt'))
    parser.add_argument('--census', type=Path, required=True)
    parser.add_argument('--review-output', type=Path, required=True)
    parser.add_argument('--reuse', action='store_true')
    args = parser.parse_args()
    repo = args.repo.resolve()
    if not args.reuse:
        record(repo, (repo / args.build_dir / 'src/qtgui/GmatQtConfiguredInputCatalog').resolve(), (repo / args.startup).resolve(), args.census.resolve())
    report = audit(repo, args.census.resolve())
    report['runner_sha256'] = sha(Path(__file__))
    args.review_output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS', report['states_reviewed'], 'configured states;', report['field_unit_pairs_reviewed'], 'field/unit meanings;', 'reused without replay' if args.reuse else 'newly inventoried')


if __name__ == '__main__':
    main()
