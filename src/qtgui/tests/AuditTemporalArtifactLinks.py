"""Check immutable artifacts separately from thin aliases to evolving runtimes."""
import hashlib
import json
from pathlib import Path


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def verify_legacy_manifest(repo, path):
    repo, path = Path(repo).resolve(), Path(path).resolve()
    manifest = json.loads(path.read_text())
    images = {}
    for maps in path.parent.glob('*-loaded-images.json'):
        record = json.loads(maps.read_text())
        for target, digest in record.get('loaded_project_images', {}).items():
            images.setdefault(target, set()).add(digest)
    regular, aliases, mapped = 0, [], 0
    for name, digest in manifest['artifact_sha256'].items():
        artifact = repo/name
        if not artifact.is_symlink():
            if sha(artifact) != digest:
                raise ValueError('Immutable artifact changed: '+name)
            regular += 1
            continue
        literal = artifact.readlink()
        target = (artifact.parent/literal).resolve()
        # RunNativeTest's preserved thin-runtime recipe links application/bin
        # entries by the same leaf name. The route is metadata, not ELF bytes.
        if literal != repo/'application/bin'/artifact.name:
            raise ValueError('Unexpected legacy thin-runtime route: '+name)
        historical = digest in images.get(str(target), set())
        aliases.append(dict(path=name, link_text=str(literal), resolved_target=str(target),
            target_hash_recorded_at_legacy_seal=digest,
            hash_matches_recorded_loaded_image=historical,
            use='historical-image-binding-only' if historical else 'fixture-alias-not-execution-evidence'))
        mapped += historical
    return dict(manifest=str(path.relative_to(repo)), manifest_sha256=sha(path),
        regular_artifact_bindings=regular, symbolic_aliases=len(aliases),
        aliases_with_historical_loaded_image_hash=mapped,
        aliases_not_execution_evidence=len(aliases)-mapped, aliases=aliases,
        literal_legacy_manifest_replay_after_rebuild=False,
        limit='The legacy seal dereferenced aliases to mutable runtime files. '
              'Regular artifact bytes are checked normally. Alias routes and historical image hashes '
              'are reported separately; no current target bytes are substituted for old binaries, '
              'no earlier invocation is relabeled with the later seal hash, and no runtime is copied.')
