"""Join every FRR!E52 evidence component without changing the campaign ledger."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

import NasaGuiCoverage as nasa
import AuditAttitudeDeclaredFrameEvidence  # frozen input-reader dependency
from AuditAttitudeOffsetScopeEvidence import replay as frame_replay,require
from AuditAttitudeOffsetBoundaryEvidence import replay as boundary_replay

REPO=Path(__file__).resolve().parents[3]
KEY='FRR!E52'
FINGERPRINT='3041f4f0af0564670cc6227dfdb1a5d7d45c70b07832be99b9cf6e1f12143333'
SHA=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
LOAD=lambda p:json.loads(Path(p).read_text())


def replay(stage,output):
    artifacts={}
    def pin(p):
        p=Path(p);artifacts[str(p)]=SHA(p);return p
    repo=stage.parents[2]
    inventory=nasa.inventory(repo);known={r['key']:r for r in inventory['requirements']}
    old_scope=LOAD(pin(stage/'row-scope-review-v1.json'))
    require(old_scope['key']==KEY and old_scope['fingerprint']==known[KEY]['fingerprint']==FINGERPRINT
            and old_scope['source_text']==known[KEY]['text'], 'Original exact requirement changed')
    require([r['key']for r in old_scope['required_origins']]==['FRR!E'+str(i)for i in range(428,433)]
            and [r['key']for r in old_scope['required_axes']]==['FRR!E'+str(i)for i in range(434,450)],
            'Original any-coordinate-system clause narrowed')
    with tempfile.TemporaryDirectory(prefix='fixed-offset-component-replay-')as temporary:
        temporary=Path(temporary)
        frames=frame_replay(stage,temporary/'frames.json')
        boundaries=boundary_replay(stage,temporary/'boundaries.json')
        command=[sys.executable,str(pin(stage/'verify-composition-v1.py')),'--output',str(temporary/'aligned.json')]
        child=subprocess.run(command,capture_output=True,text=True)
        require(child.returncode==0,'Retained original aligned/nonzero witness replay failed: '+child.stderr)
        aligned=LOAD(temporary/'aligned.json')
    require(frames['passed_profile_cells']==136 and boundaries['passed_profile_cases']==18
            and boundaries['actual_GUI_rejections']==40,'Whole component/profile conjunction incomplete')
    # Pin the recorded audits as well as independently replaying their raw inputs.
    for name in ['full-scope-replay-v1.json','boundary-replay-v1.json','composition-replay-v1.json',
                 'compiler-dependency-review-v2.json','incremental-core-review-v1.json','abi-v1.json',
                 'core-change-inventory-v2/inventory.json','core-change-inventory-v2/core.patch']:
        pin(stage/name)
    parent=LOAD(pin(stage/'scope-build-receipt-v6-attempt2.json'))
    frozen=Path(parent['frozen_source_root'])
    native=stage/'native-support-v1'
    producer=LOAD(pin(native/'build-receipt-v1.json'));execution=LOAD(pin(native/'execution-v2.json'))
    require(producer['status']=='passed' and producer['returncode']==0 and producer['inputs_unchanged']
            and execution['returncode']==0 and execution['inputs_unchanged']
            and execution['producer_receipt_sha256']==SHA(native/'build-receipt-v1.json')
            and execution['runtime_binary_copies']==0 and execution['GUI_contexts']==0,
            'Native metadata/copy/normalization producer or execution failed')
    for name,digest in execution['source_sha256'].items():require(SHA(frozen/name)==digest,'Native support source closure differs')
    for name,digest in execution['binary_sha256'].items():require(SHA(name)==digest,'Native support image changed')
    require(len(execution['processes'])==1 and len(execution['loaded_project_images'])==3,'Actual native support maps omitted')
    log=pin(native/'native-v2.log').read_text();results=[]
    for line in log.splitlines():
        if '{"status":"passed"'in line:results.append(json.loads(line[line.index('{'):]))
    require(len(results)==1 and results[0]['assertions']==163 and results[0]['sizeof_CSFixed']==2008,
            'Native metadata/copy/index/normalization/serialization assertions incomplete')
    for name in ['execution-v1.json','native-v1.log','junit-v1.xml','junit-v2.xml']:pin(native/name)
    require(execution['prior_loader_failure_sha256']==SHA(native/'execution-v1.json'),'Prior loader failure lost')
    review=LOAD(stage/'incremental-core-review-v1.json')
    require(review['status']=='exact-two-file-increment-retained' and review['technical_core_approval']is False
            and review['scientific_approval']is False and review['total_pending_core_paths']==286,
            'Exact core change/approval scope changed')
    require(SHA(pin(stage/'incremental-core-v1.patch'))==review['patch_sha256'],'Exact core patch changed')
    require({r['path']for r in review['files']}=={'src/base/attitude/CSFixed.cpp','src/base/attitude/CSFixed.hpp'},'Extra core change in this component')
    for row in review['files']:
        require(SHA(pin(stage/'preimages-v1'/row['path']))==row['before_sha256']
                and SHA(frozen/row['path'])==row['after_sha256'],'Core preimage/compiled increment changed')
    abi=LOAD(stage/'abi-v1.json')
    require(abi['class_layout_changed']and abi['new_virtual_slots']==0 and abi['technical_core_approval']is False
            and [r['class_size']for r in abi['records']]==[1984,2008]
            and [r['vtable_entries']for r in abi['records']]==[278,278],'ABI layout limitation hidden')
    for record in abi['records']:
        name='abi-'+record['version']
        require(SHA(pin(stage/(name+'.classes')))==record['class_dump_sha256'],'Actual compiler class-layout dump changed')
    code=(frozen/'src/base/attitude/CSFixed.cpp').read_text()
    for token in ['refCS->FromBaseSystem(A1Mjd(atTime), bogus, true, true)',
                  'dcm              = offset * RiI;',
                  'Rmatrix33 wxIBB  = - (offset * RiIDot * (dcm.Transpose()));',
                  'const Rvector normalized = NormalizedFixedOffset(value);',
                  'offsetQuaternion(att.offsetQuaternion)', 'offsetQuaternion = att.offsetQuaternion;',
                  'ComputeCosineMatrixAndAngularVelocity(atTime.GetMjd());']:
        require(token in code,'Generic frame/constant-offset/copy dispatch guard omitted: '+token)
    require(SHA(frozen/'src/qtgui/tests/NasaAttitudeFixedFrameTests.cpp')==old_scope['original_driver_sha256']
            and SHA(frozen/'src/qtgui/tests/AttitudeFixedFrameCases.json')==old_scope['original_catalog_sha256'],
            'Original aligned driver/assertions/catalog changed')
    guide=pin(frozen/'doc/help/src/Resource_SpacecraftAttitude.xml')
    ET.fromstring(guide.read_bytes())
    require('OffsetQuaternion'in guide.read_text() and 'CoordinateSystemFixed'in guide.read_text(), 'User guide lacks implemented optional offset')
    guide_contract=LOAD(frozen/'src/qtgui/tests/AttitudeFixedOffsetCases.json')
    require(guide_contract['guide_source_sha256']==SHA(guide),'Guide contract is unbound')
    # This argument is about composition of any supplied frame basis, not an
    # assertion that every physical astronomical frame model is universally true.
    generic_argument=dict(
        reference_basis='R maps inertial vectors into the selected frame; frame origin translation is suppressed by coincident=true.',
        offset='C is the normalized constant frame-to-body quaternion matrix, default I.',
        orientation='B = C R; the same code dispatches every configured FRR-35 coordinate system without axis/origin-specific offset branches.',
        derivative='Bdot = C Rdot because C is constant; -Bdot B^T = C (-Rdot R^T) C^T.',
        angular_velocity='The skew-vector identity gives omega_body = C omega_frame, including rotating/body/orbit/date/station frames.',
        domain='Finite four-component offsets of magnitude at least the native minimum; invalid inputs reject atomically. Source dispatch and boundary witnesses cover the representation contract.',
        limits='Frame physical models retain their original independent oracle/convention bounds. This join does not grant broad astronomical/scientific, external-ABI or full-current-build approval.')
    # Every old failure remains a separate immutable record and grants no credit.
    failures=['build-receipt-v2.json','support-preflight-refusal-v4.json',
              'scope-build-receipt-v5.json','scope-build-preflight-refusal-v5-attempt2.json',
              'scope-build-receipt-v6.json','scope-visible-preflight-refusal-v6.json',
              'offset-boundary-normalized-headless-v1/results.json',
              'offset-boundary-normalized-headless-v1-loaded-images.json',
              'boundary-error-surface-review-v1.json','boundary-reader-observer-contract-review-v1.json']
    for name in failures:pin(stage/name)
    result=dict(status='complete-original-fixed-offset-evidence-conjunction',requirement=KEY,fingerprint=FINGERPRINT,
                original_requirement=known[KEY],original_origins=old_scope['required_origins'],original_axes=old_scope['required_axes'],
                applicability=dict(original=80,required=68,original_explicit_exclusions=12,implementation_exclusions=0),
                frame_component=frames,boundary_component=boundaries,original_aligned_component=aligned,
                native_support_assertions=163,generic_argument=generic_argument,artifacts=artifacts,
                failures_retained=failures,original_assertions_changed=0,runtime_binary_copies=0,native_reruns=0,
                whole_row_evidence_complete=True,canonical_ledger_admitted=False,new_requirement_credit=0,
                scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,
                full_NASA_qualified=False,reader_sha256=SHA(__file__))
    with output.open('x')as stream:json.dump(result,stream,indent=2);stream.write('\n')
    return result


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,default=REPO/'build/nasa-coverage/coverage95-20261104-attitude-fixed-offset');p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    r=replay(a.stage.resolve(),a.output.resolve());print(json.dumps({k:r[k]for k in ['status','requirement','whole_row_evidence_complete','new_requirement_credit','native_reruns']}))
