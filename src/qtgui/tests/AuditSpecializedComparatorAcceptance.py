"""Qualify the original SPK/TDM harness capabilities, never a GUI/science pass."""
import argparse
from decimal import Decimal
import hashlib
import json
import math
from pathlib import Path

import NasaGuiCoverage as nasa
from RunHarnessAcceptance import passed_junit
from AuditCapiAcceptance import load, require, sha, near

KEYS = {'All Others!E306', 'All Others!E308'}
TEST = 'QtAcceptance.SpecializedComparatorContracts'
TOKENS = ["self.assertEqual(report['compared_fields'], 8)", "self.assertEqual((cell['delta'], cell['allowed']), ('1E-8','9E-9'))",
          "self.assertEqual(report['compared_fields'], 72)", "self.assertAlmostEqual(float(cell['actual']), expected[j], delta=2e-10)",
          "self.assertAlmostEqual(float(cell['truth']), expected[j], delta=2e-10)", "self.assertEqual(self.t.read_bytes(),before)",
          "self.assertEqual(self.spk(options={'observer':0})['status'],'error')", "self.assertEqual(self.spk(options={'frame':'UNKNOWN_FRAME'})['status'],'error')",
          "self.assertEqual(self.spk(options={'max_samples':1})['status'],'error')", "self.assertIn('not a continuous-error bound',page.read_text())"]


def polynomial(t, body, perturb=False):
    k = .0001 if perturb else 0
    return [7000+(100 if body == -1002 else 0)+2*t+.01*t*t+k*t*(30-t),
            500-3*t+.02*t*t, -400+t-.03*t*t, 2+.02*t+k*(30-2*t), -3+.04*t, 1-.06*t]


def native_reports(artifacts, external):
    require(len(artifacts) == 12, 'Incomplete retained comparator examples')
    for p, digest in artifacts.items(): require(sha(p) == digest, 'Changed actual comparator artifact: '+p)
    by_name = {Path(p).name:Path(p) for p in artifacts}
    tdm = load(by_name['tdm-kvn-xml.json'])
    require(tdm['status'] == 'passed' and tdm['kind'] == 'tdm' and tdm['compared_fields'] == 8
            and 'No unit/time conversion' in tdm['format_scope'], 'Incomplete TDM example')
    expected = dict(RANGE=['7000','7001'], DOPPLER_INTEGRATED=['-1.25','-1.5'], ANGLE_1=['45.5'], ANGLE_2=['30'],
                    RECEIVE_FREQ_1=['8400000000'], TRANSMIT_PHASE_CT_1=['9007199254740993.00000001'])
    require(len(tdm['comparisons']) == 8 and not tdm['differences'] and not tdm['errors'], 'TDM example omitted fields')
    for field, values in expected.items():
        rows = [c for c in tdm['comparisons'] if c['field'] == field]
        require([c['truth'] for c in rows] == values and all(not c['exceeded'] and c['delta'] == '0' and Decimal(c['actual']) == Decimal(c['truth']) for c in rows), 'TDM independent literal value failed')
    for name, drift in [('spk-polynomial-types8-13',False), ('spk-interior-drift',True)]:
        report = load(by_name[name+'.json'])
        require(report['status'] == ('failed' if drift else 'passed') and report['kind'] == 'spk'
                and report['compared_fields'] == len(report['comparisons']) == 72 and not report['errors'] and not report['differences'], 'Incomplete SPK numeric comparison')
        policy = report['spk_policy']
        require(policy['observer'] == 399 and policy['frame'] == 'J2000' and policy['time_system'] == 'TDB'
                and policy['step'] == 7 and policy['position_unit'] == 'km' and policy['velocity_unit'] == 'km/s'
                and 'not a continuous-error bound' in policy['comparison'], 'SPK comparison interpretation changed')
        require(report['loaded_dependencies'] == external, 'Native CSPICE library/dependencies not bound')
        samples = [dict(object=body,tdb_seconds=t) for body in [-1002,-1001] for t in [0,7,14,21,28,30]]
        require(report['spk_samples'] == samples, 'SPK samples incomplete/out of order')
        require(report['spk_coverage'] == dict(actual={'-1001':[[0.0,30.0]],'-1002':[[0.0,30.0]]},truth={'-1001':[[0.0,30.0]],'-1002':[[0.0,30.0]]}), 'SPK object/coverage identity changed')
        require(report['inputs']['actual']['sha256'] != report['inputs']['truth']['sha256'], 'SPK witness is only byte identity')
        for i, sample in enumerate(samples):
            actual = polynomial(sample['tdb_seconds'],sample['object'],drift); truth = polynomial(sample['tdb_seconds'],sample['object'])
            for j, c in enumerate(report['comparisons'][i*6:i*6+6]):
                require(c['field'] == ['X','Y','Z','VX','VY','VZ'][j] and c['unit'] == ('km' if j < 3 else 'km/s')
                        and near(float(c['actual']),actual[j],1e-9) and near(float(c['truth']),truth[j],1e-9)
                        and near(float(c['delta']),actual[j]-truth[j],1e-9), 'SPK independent polynomial state/delta failed')
        page = by_name[name+'.html'].read_text()
        require('J2000' in page and '399' in page and 'not a continuous-error bound' in page, 'HTML omits SPK interpretation')
    for name in ['tdm-kvn-xml','spk-polynomial-types8-13','spk-interior-drift']:
        report=load(by_name[name+'.json'])
        for role in ('actual','truth'):require(sha(by_name[name+'-'+role+'.dat']) == report['inputs'][role]['sha256'], 'Comparator example input differs from execution')


def qualify(repo, path, inventory):
    q = load(path)
    require(q['contract_kind'] == 'source-versioned-harness-specialized-comparators' and len(q['qualified_keys']) == 2 and set(q['qualified_keys']) == KEYS, 'Partial/extra/duplicate specialized credit')
    require(all(q.get(k) is False for k in ('full_current_build_qualified','full_NASA_qualified','technical_core_approval')), 'Specialized proof exceeds qualification scope')
    for p, d in q['artifact_sha256'].items(): require(sha(p) == d, 'Changed specialized artifact: '+p)
    require(q['authority_sha256'] == inventory['sources'], 'NASA source authority differs')
    original = load(q['original_qualification']); receipt = load(original['receipt']); frozen = Path(original['source_root'])
    require(original['source_inputs_unchanged'] is True and receipt['source_inputs_unchanged'] is True and receipt['mode'] == 'harness-current-source', 'Missing actual source-bound harness execution')
    for field in ('receipt','report','review'): require(sha(original[field]) == original[field+'_sha256'], 'Original harness '+field+' changed')
    copied, authority = original['preserved_source_sha256'], original['authority_inputs_not_copied']
    require(authority == inventory['sources'] and not set(copied)&set(authority) and {**copied,**authority} == receipt['source_hashes'], 'Incomplete harness source preservation')
    for source, digest in copied.items():
        require(not Path(source).is_absolute() and '..' not in Path(source).parts, 'Unsafe source archive member')
        data=(frozen/source).read_bytes();require(hashlib.sha256(data).hexdigest() == digest and not data.startswith(b'\x7fELF'), 'Changed/non-source frozen input')
    require(copied['src/qtgui/tests/NasaGuiCoverage.py'] == sha(nasa.__file__), 'Original coverage interpreter changed')
    review = nasa.load_review(frozen, inventory, manifest=original['review'])
    require(len(review['reviews']) == 2 and {r['key'] for r in review['reviews']} == KEYS and all(r['applicability'] == 'harness' for r in review['reviews']), 'Wrong original specialized requirement scope')
    results = receipt['results']; require(len(results) == 1 and results[0]['name'] == TEST, 'Extra/missing specialized registration')
    result=results[0];require(result['status'] == 'passed' and passed_junit(Path(result['junit']),TEST,result['returncode']), 'Specialized registration did not pass')
    for field in ('log','junit'):require(sha(result[field]) == result[field+'_sha256'], 'Changed actual '+field)
    code=(frozen/'src/qtgui/tests/AcceptanceSpecializedCompareTests.py').read_text()
    for token in TOKENS:require(token in code, 'Missing original independent assertion: '+token)
    records=nasa.proof_records(result)
    require(len(records) == 2 and {p['checkpoint'] for p in records} == {'specialized-comparator-306','specialized-comparator-308'}
            and all(p['contracts'] == 10 and p['GUI_executions'] == p['runtime_copies'] == 0 for p in records), 'Incomplete actual specialized checkpoints')
    require(records[0]['artifact_sha256'] == records[1]['artifact_sha256'], 'Native examples differ between row proofs')
    external=load(q['external_inputs_before']);require(external == load(q['external_inputs_after']) and len(external) == 2, 'CSPICE dependencies changed during execution')
    native_reports(records[0]['artifact_sha256'], external)
    checked=nasa.evaluate(frozen,inventory,review,receipt)
    keys={r['key'] for kind in ('requirements','historical_cases') for r in checked[kind] if r['status'] == 'verified'}
    require(keys == KEYS == set(original['qualified_keys']), 'Original specialized clauses did not qualify')
    return q, sorted(KEYS)


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);a=p.parse_args()
    _,keys=qualify(a.repo.resolve(),a.qualification,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,new_GUI_or_engine_executions=0)))
