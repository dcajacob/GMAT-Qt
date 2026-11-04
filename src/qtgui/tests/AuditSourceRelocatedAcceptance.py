"""Reuse an exact prior qualification after freezing a mutable source artifact.

Only artifact locations may change. Original keys, receipts, assertions,
fingerprints and expected results are byte-for-byte the original JSON values.
The original contract auditor still performs all of its checks.
"""
import copy
from pathlib import Path
from AuditCapiAcceptance import load, require, sha


def qualify(repo,path,inventory):
    wrapper=load(path)
    require(wrapper['contract_kind']=='source-versioned-qualification-source-relocation',
            'Wrong source relocation contract')
    for name,digest in wrapper['artifact_sha256'].items():
        require(sha(name)==digest,'Changed relocation artifact: '+name)
    required={wrapper['original_qualification'],wrapper['relocated_qualification']}
    require(required<=set(wrapper['artifact_sha256']),'Unsealed original/relocated qualification')
    original=load(wrapper['original_qualification'])
    require(original['contract_kind']!='source-versioned-qualification-source-relocation',
            'Nested source relocation is not admitted')
    expected=copy.deepcopy(original)
    require(bool(wrapper['source_relocations']),'Empty relocation')
    for old,replacement in wrapper['source_relocations'].items():
        source=Path(old).resolve();frozen=Path(replacement).resolve()
        require(source.is_relative_to(repo/'src') and source.suffix in {'.cpp','.hpp','.py'},
                'Only a source artifact can be relocated')
        require(frozen.is_relative_to(repo/'build/nasa-coverage') and
                replacement in wrapper['artifact_sha256'],'Frozen source location not sealed')
        require(old in expected['artifact_sha256'] and replacement not in expected['artifact_sha256'],
                'Missing/duplicate original source artifact')
        digest=expected['artifact_sha256'].pop(old)
        require(sha(replacement)==digest and not Path(replacement).read_bytes().startswith(b'\x7fELF'),
                'Relocated source differs from the actually qualified version')
        expected['artifact_sha256'][replacement]=digest
    require(load(wrapper['relocated_qualification'])==expected,
            'Relocation changed obligations, tests, assertions or qualification content')
    require(wrapper['qualified_keys']==original['qualified_keys'] and
            all(wrapper.get(k) is False for k in ['technical_core_approval',
                'full_current_build_qualified','full_NASA_qualified']), 'Relocation scope changed')
    from BuildNasaCoverage95 import qualify as admission
    _,keys=admission(repo,Path(wrapper['relocated_qualification']),inventory)
    require(keys==original['qualified_keys'],'Original auditor changed the relocated key set')
    return wrapper,keys
