"""Conjoin all original FRR-38.3 property clauses with complete typed evidence.

These two finite list parents do not inherit FRR-38.1's any-body or FRR-38.2's
any-frame obligations. Every original child/setter clause remains mandatory;
scalar-only constraint evidence cannot replace the Gregorian or mutability proof.
"""
import argparse
import json
from pathlib import Path

from AuditCapiAcceptance import load,require,sha
import NasaGuiCoverage as nasa
import AuditConstraintScalarAcceptance as scalars
from BuildNasaCoverage95 import baseline

KEYS=['FRR!E549','FRR!E550']


def qualify(repo,path,inventory):
    q=load(path)
    require(q['contract_kind']=='source-versioned-complete-calculation-family' and q['qualified_keys']==KEYS,'Partial/extra calculation-family credit')
    require(all(q.get(k) is False for k in ('technical_core_approval','full_current_build_qualified','full_NASA_qualified')),'Calculation family exceeds qualification scope')
    require(q['authority_sha256']==inventory['sources'],'Calculation-family authority differs')
    for name,digest in q['artifact_sha256'].items():require(sha(name)==digest,'Changed calculation-family artifact: '+name)
    known={r['key']:r for r in inventory['requirements']};review=load(q['review'])
    require([r['key'] for r in review['reviews']]==KEYS,'Missing original family parent review')
    for r in review['reviews']:require(r['fingerprint']==known[r['key']]['fingerprint'] and r['source_text']==known[r['key']]['text'] and r['rationale'],'Changed original family parent')
    # Use declared workbook hierarchy rather than guessing a universal scope
    # from a generic earlier applicability rationale. Include every source row.
    source={r['key']:r for r in inventory['requirements'] if r['declared_id'].startswith('FRR-38.3.')}
    require(set(source)=={f'FRR!E{n}' for n in range(549,589)} and source['FRR!E549']['declared_id']=='FRR-38.3.0' and source['FRR!E550']['declared_id']=='FRR-38.3.1.0','Original complete declared hierarchy changed')
    require(review['source_hierarchy']==[{k:r[k] for k in ('key','declared_id','fingerprint','text')} for r in source.values()],'Source hierarchy missing/altered')
    children=set(source)-set(KEYS)
    require(set(review['inherited_complete_child_keys'])==children and len(review['inherited_complete_child_keys'])==38,'Original required child omitted')
    seal,original,keys=baseline(repo,Path(q['baseline_seal']))
    require(original['sources']==inventory['sources'] and children<=keys['requirements'],'Unqualified original child cannot supply parent credit')
    require(set(review['unqualified_neighbor_parent_keys'])=={'FRR!E501','FRR!E502','FRR!E525','FRR!E526'},'Any-body/any-frame neighbors silently credited')
    scalarq=load(q['scalar_family_qualification']);fixture=load(scalarq['fixture']);properties=[r for r in fixture['source_rows'] if r['key'] in children]
    scalar_rows={r['key'] for r in properties if r['scalar_types']};other_rows={r['key'] for r in properties if r['non_scalar_types']}
    require(scalar_rows|other_rows==children-{'FRR!E575','FRR!E582'} and other_rows=={'FRR!E564','FRR!E566','FRR!E568','FRR!E570'},'Missing scalar/Gregorian property partition')
    require(review['non_scalar_child_keys']==sorted(other_rows) and review['retained_mutability_and_format_children']==sorted(children),'Gregorian, read/write or readonly clause omitted')
    # Native typed browser, raw independent values, twelve Euler sequences and
    # both modes are checked in the full scalar admission. Existing child rows
    # additionally retain all of their setter, clock-format, readonly-rejection,
    # source/history/native-result and both-cohort obligations at sealed versions.
    scalars.qualify(repo,Path(q['scalar_family_qualification']),inventory)
    return q,KEYS


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);a=p.parse_args();_,keys=qualify(a.repo.resolve(),a.qualification,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,GUI_or_engine_executions=0)))
