"""Immutable project inputs for acceptance runs, independent of later rebuilds.

System tools/libraries and external startup paths are deliberately not claimed
as frozen. Hashes bind the copied project artifacts, not compiler provenance.
"""
import copy
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import uuid


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def fingerprint(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def remap(value, original, frozen):
    if isinstance(value, str):
        return re.sub(re.escape(str(original)) + r'''(?=$|[/\\\s"';:\]])''', lambda _: str(frozen), value)
    if isinstance(value, list):
        return [remap(item, original, frozen) for item in value]
    if isinstance(value, dict):
        return {key: remap(item, original, frozen) for key, item in value.items()}
    return value


def files_under(directory, excluded=(), directories=None, writable_links=()):
    directory = Path(directory)
    if not directory.exists():
        return
    for parent, names, files in os.walk(directory, followlinks=False):
        names[:] = [name for name in names if name not in excluded]
        if directories is not None:
            directories.add(Path(parent))
        for name in names:
            if (Path(parent) / name).is_symlink() and Path(parent) / name not in writable_links:
                raise ValueError('Snapshot directory symlink needs explicit review: ' + str(Path(parent) / name))
        for name in files:
            if name not in excluded:
                yield Path(parent) / name


def validate_selection(manifest, names):
    preserved = manifest.get('preserved_case_names')
    if preserved is not None and not set(names) <= set(preserved):
        raise ValueError('Selection includes an executable not preserved by this snapshot; narrow --filter or create a new snapshot')


def create(repo, build, cases, source_files, output, preserve_cases=None, cache_dir=None):
    repo, build, output = Path(repo).resolve(), Path(build).resolve(), Path(output).resolve()
    root = output / 'snapshot' / 'repo'
    paths = {repo / name for name in source_files}
    directories = {repo / 'application/bin'}
    preserved = None if preserve_cases is None else set(preserve_cases)
    if preserved is not None and (not preserved or not preserved <= {c['name'] for c in cases}):
        raise ValueError('Invalid preserved case selection')
    # These directories contain runtime libraries, shipped data and helper/input
    # files used implicitly by tests, not merely the explicit argv fixtures.
    for directory, excluded in [(repo / 'application', ('output', 'MyGmat.ini', 'GmatLog.txt', '__pycache__')),
                                (repo / 'src/qtgui/tests', ('__pycache__',))]:
        paths.update(files_under(directory, excluded, directories))
    for case in cases:
        if preserved is not None and case['name'] not in preserved:
            continue
        for argument in case.get('command', []):
            candidate = Path(argument)
            if candidate.is_absolute() and candidate.is_file() and candidate.is_relative_to(repo):
                paths.add(candidate)
            elif candidate.is_absolute() and candidate.is_dir() and candidate.is_relative_to(repo):
                directories.add(candidate)
        working = next((p['value'] for p in case.get('properties', []) if p['name'] == 'WORKING_DIRECTORY'), None)
        if working and Path(working).is_relative_to(repo):
            directories.add(Path(working))
    definition = build / 'src/qtgui/CTestTestfile.cmake'
    if not definition.is_file():
        definition = build / 'CTestTestfile.cmake'
    if not definition.is_file():
        raise ValueError('No CTest registration file to freeze')
    paths.add(definition)
    for path in paths:
        if not path.is_file() or not path.is_relative_to(repo):
            raise ValueError('Cannot snapshot missing or out-of-project input: ' + str(path))
    before = {str(path.relative_to(repo)): sha(path) for path in sorted(paths)}
    root.mkdir(parents=True, exist_ok=False)
    for directory in directories:
        (root / directory.relative_to(repo)).mkdir(parents=True, exist_ok=True)
    cached = []
    cache = Path(cache_dir).resolve() if cache_dir else None
    if cache:
        cache.mkdir(parents=True, exist_ok=True)
    for relative, digest in before.items():
        source, target = repo / relative, root / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        # All captured inputs are immutable, including the many small help/data
        # files. Only the registration copy must stay private for path remapping.
        if cache and source != definition:
            mode = stat.S_IMODE(source.stat().st_mode)
            entry = cache/(digest+'-'+oct(mode)[2:])
            if (entry.is_symlink() or not entry.is_file() or sha(entry) != digest
                    or stat.S_IMODE(entry.stat().st_mode) != (mode & ~0o222)):
                temporary = cache/('.capture-'+uuid.uuid4().hex)
                shutil.copy2(source, temporary, follow_symlinks=True)
                if sha(temporary) != digest:
                    temporary.unlink()
                    raise ValueError('Input changed while filling cache: '+relative)
                temporary.chmod(mode & ~0o222)
                os.replace(temporary, entry)
            os.link(entry, target)
            cached.append(relative)
        else:
            shutil.copy2(source, target, follow_symlinks=True)
        if sha(target) != digest:
            raise ValueError('Input changed while being copied: ' + relative)
    # Detect cross-file changes across the entire capture, including a rebuild
    # that replaced an executable after that executable had already been copied.
    if any(sha(repo / relative) != digest for relative, digest in before.items()):
        raise ValueError('Project changed during snapshot creation; create a new run after the build finishes')
    frozen_cases = remap(copy.deepcopy(cases), repo, root)
    for original, frozen in zip(cases, frozen_cases):
        frozen['original_command'] = original.get('command', [])
        # Existing tests accept nonexistent build-tree stems for screenshots and
        # comparison products. They are outputs, not preserved input files.
        outputs = {}
        for index, argument in enumerate(original.get('command', [])):
            candidate = Path(argument)
            if candidate.is_absolute() and candidate.is_relative_to(build) and not candidate.exists():
                relative = candidate.relative_to(build)
                if '..' in relative.parts:
                    raise ValueError('Output path escapes the build directory')
                outputs[str(index)] = str(relative)
        if outputs:
            frozen['output_arguments'] = outputs
    frozen_definition = root / definition.relative_to(repo)
    frozen_definition.write_text(remap(frozen_definition.read_text(), repo, root))
    # Preserve both the original registration digest and the executable copy.
    artifacts = dict(before)
    artifacts[str(definition.relative_to(repo))] = sha(frozen_definition)
    # The shipped launcher discovers this directory through its default startup
    # beside the executable. Keep that discovery path, but route mutable output
    # away from the preserved inputs. The link itself is part of the contract.
    runtime_output = root.parent / 'runtime-output'
    runtime_output.mkdir()
    (root / 'application/output').symlink_to(runtime_output, target_is_directory=True)
    manifest = {'schema_version': 1, 'original_repo': str(repo), 'original_build': str(build),
                'root': str(root), 'ctest_definition': str(frozen_definition),
                'original_hashes': before, 'artifact_hashes': artifacts,
                'source_files': sorted(source_files), 'cases': frozen_cases,
                'runtime_outputs': {'application/output': str(runtime_output)},
                'limit': 'Project artifacts only. External tools/libraries/paths are not frozen; compiler provenance is not established.'}
    if preserved is not None:
        manifest['preserved_case_names'] = sorted(preserved)
    if cached:
        manifest['shared_read_only_inputs'] = cached
    manifest['snapshot_id'] = fingerprint(manifest)
    path = root.parent / 'manifest.json'
    path.write_text(json.dumps(manifest, indent=2) + '\n')
    return manifest, path


def verify(manifest):
    value = dict(manifest)
    identity = value.pop('snapshot_id', None)
    if identity != fingerprint(value):
        raise ValueError('Snapshot manifest identity changed')
    root = Path(manifest['root'])
    writable_links = set()
    for relative, destination in manifest.get('runtime_outputs', {}).items():
        link, target = root / relative, Path(destination)
        if (not link.is_symlink() or not target.is_dir() or
                target.resolve().is_relative_to(root.resolve()) or link.resolve() != target.resolve()):
            raise ValueError('Snapshot runtime-output route changed: ' + relative)
        writable_links.add(link)
    for relative, digest in manifest['artifact_hashes'].items():
        path = root / relative
        if not path.is_file() or sha(path) != digest:
            raise ValueError('Snapshot artifact changed or disappeared: ' + relative)
        if relative in manifest.get('shared_read_only_inputs', []) and path.stat().st_mode & 0o222:
            raise ValueError('Shared snapshot input became writable: '+relative)
    present = {str(path.relative_to(root)) for path in files_under(root, ('__pycache__',), writable_links=writable_links)}
    unexpected = present - manifest['artifact_hashes'].keys()
    if unexpected:
        raise ValueError('Unexpected snapshot input: ' + sorted(unexpected)[0])


def profile(args):
    return {key: getattr(args, key, None) for key in ('config', 'visible', 'display_route', 'display_scale',
                                                     'scale', 'width', 'height', 'fixed_size', 'start_minimized')}


def case_fingerprint(case):
    return fingerprint({key: case.get(key) for key in ('name', 'command', 'properties', 'sources', 'visibility', 'output_arguments')})


def runtime_command(case, directory):
    command = list(case.get('command', []))
    for index, relative in case.get('output_arguments', {}).items():
        path = Path(directory) / 'outputs' / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        command[int(index)] = str(path)
    return command


def reusable(result, case, snapshot_id, execution_profile):
    """Require the recorded execution identity and untouched result evidence."""
    if result.get('status') != 'passed' or result.get('snapshot_id') != snapshot_id:
        return False
    if result.get('execution_profile') != execution_profile or result.get('case_fingerprint') != case_fingerprint(case):
        return False
    for name in ('log', 'junit'):
        path = Path(result.get(name, ''))
        if not path.is_file() or result.get(name + '_sha256') != sha(path):
            return False
    return True
