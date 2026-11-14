"""Reuse a sealed source-versioned union without claiming current-build qualification.

The original baseline auditor and exact row/fingerprint union are rechecked.
Prior native receipts, source capsules, admissions and their sealed artifact
bindings remain immutable. Current edited source is deliberately not substituted
for the source versions that actually passed those earlier tests.
"""
import hashlib
from pathlib import Path
import tarfile
import NasaGuiCoverage as nasa
import BuildNasaCoverageExtension as common

sha,load,require=common.sha,common.load,common.require
KIND='sealed-95-percent-source-versioned-prefix'


def baseline(repo,path):
    q=load(path)
    require(q['kind']==KIND and q['target_requirements']==1101,'Different sealed prefix kind/target')
    require(all(q[k] is False for k in ['technical_core_approval','full_current_build_qualified','full_NASA_qualified']),
            'Historical prefix claims current-build approval')
    for name,digest in q['artifact_sha256'].items():require(sha(name)==digest,'Changed prefix artifact: '+name)
    stage=Path(q['stage']);ledger=load(q['ledger']);receipt=load(q['receipt']);manifest=load(q['manifest']);seal=load(q['verification'])
    required={q[k] for k in ['ledger','receipt','manifest','verification','overlay','admission_audit']}
    require(required<=set(q['artifact_sha256']),'Unbound prefix closure evidence')
    for name,digest in manifest['artifact_sha256'].items():require(sha(repo/name)==digest,'Changed sealed historical evidence: '+name)
    require(sha(q['manifest'])==receipt['manifest_sha256']==seal['manifest_sha256'] and sha(q['receipt'])==seal['receipt_sha256']
            and sha(q['ledger'])==receipt['canonical_ledger_sha256']==seal['canonical_ledger_sha256'],'Prefix seal chain differs')
    require(ledger['status']=='passed' and ledger['coverage']==receipt['coverage']==q['coverage']
            and ledger['target_requirements']==1101 and ledger['authority_sha256']==q['authority_sha256']==nasa.inventory(repo)['sources'],
            'Prior exact union/authority differs')
    require(seal['requirements_verified']==ledger['coverage']['requirements']['verified'] and seal['requirements_total']==1158
            and seal['remaining_to_target']==ledger['requirements_still_needed']==max(0,1101-seal['requirements_verified']),
            'Historical target arithmetic differs')
    require(all(ledger[k] is False and receipt[k] is False for k in ['technical_core_approval','full_current_build_qualified','full_NASA_qualified']),
            'Historical qualification gate changed')
    ids=manifest['final_source_sha256']
    with tarfile.open(stage/'final-sources-v1.tar.gz') as archive:
        require(set(archive.getnames())==set(ids),'Historical final source partition differs')
        for name,digest in ids.items():require(hashlib.sha256(archive.extractfile(name).read()).hexdigest()==digest,'Historical final source differs')
    require(sha(stage/'final-sources-v1.tar.gz')==receipt['source_archive_sha256'],'Historical source archive chain differs')
    for name in manifest['executed_source_capsules']:
        archive=stage/name;version=name.removeprefix('executed-sources-v').removesuffix('.tar.gz')
        identity=load(stage/f'executed-source-sha256-v{version}.json')
        with tarfile.open(archive) as bundle:
            require(set(bundle.getnames())==set(identity),'Historical executed source partition differs')
            for p,h in identity.items():require(hashlib.sha256(bundle.extractfile(p).read()).hexdigest()==h,'Historical executed source differs')
    audit=load(q['admission_audit']);overlay=load(q['overlay'])
    require(audit['status']=='passed' and audit['qualified_keys']==receipt['qualified_keys']==q['last_admitted_keys']
            and sha(q['admission_audit'])==receipt['row_admission_audit_sha256'],'Historical admission chain differs')
    require(len(overlay['reviewed_source_rows'])==139 and len(overlay['original_hold_keys_retained'])==59
            and overlay['coverage']==ledger['coverage'],'Historical original row/hold records lost')
    # Reconstruct every original key from the preceding audited prefix and every
    # exact addition. This is source-versioned reuse, not a fresh native pass.
    import BuildNasaCoverage95 as builder
    _,inventory,keys=builder.baseline(repo,Path(ledger['baseline']))
    require(sha(ledger['baseline'])==ledger['baseline_sha256'],'Historical baseline identity differs')
    known={kind:{r['key']:r for r in inventory[kind]} for kind in keys}
    for row in ledger['additions']:
        kind,key=row['kind'],row['key']
        require(row['status']=='verified-source-versioned' and row['fingerprint']==known[kind][key]['fingerprint']
                and row['source_text']==known[kind][key]['text'] and sha(row['qualification'])==row['qualification_sha256'],
                'Historical addition fingerprint/qualification differs')
        keys[kind]=common.merge_keys(keys[kind],[key],known[kind])
    for kind in keys:
        require(len(keys[kind])==ledger['coverage'][kind]['verified'] and len(known[kind])==ledger['coverage'][kind]['total'],
                'Historical numerator/denominator differs')
        remaining=ledger['remaining_original_rows'][kind]
        require({r['key'] for r in remaining}==set(known[kind])-keys[kind],'Historical original scope dropped')
        for row in remaining:require(row['fingerprint']==known[kind][row['key']]['fingerprint'],'Historical remaining fingerprint differs')
    require(q['remaining_original_rows']==ledger['remaining_original_rows'],'Prefix remaining records changed')
    return q,inventory,keys
