"""Generate only missing post-reload consumers; preserve the generic driver byte-for-byte."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
SHA = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()

HELPER = r'''
static QJsonObject readonlySavedField(const QJsonObject &roots,const QJsonObject &request){
 auto node=roots[request["object"].toString()].toObject();
 require(!node.isEmpty(),"Read-only consumer root absent");
 for(const auto &value:request["owned_indices"].toArray()){
  const int index=value.toInt(-1);const auto children=node["owned"].toArray();
  require(index>=0&&index<children.size(),"Read-only consumer owned path absent");node=children[index].toObject();
 }
 require(node["type"]==request["native_type"],"Read-only consumer native owner type changed");
 QJsonObject result;int matches=0;
 for(const auto &entry:node["fields"].toArray()){
  const auto field=entry.toObject();
  if(field["name"]==request["field"]&&field["type"]==request["type"]){result=field;++matches;}
 }
 require(matches==1&&result["read_only"].toBool()&&result.contains("value")&&!result.contains("getter_error")&&!result.contains("not_observed"),"Read-only consumer descriptor/getter absent or ambiguous");
 return result;
}
'''

CHECK = r'''
 for(const auto &entry:spec["readonly_roundtrip_fields"].toArray()){
  const auto request=entry.toObject(),a=readonlySavedField(runtime,request),b=readonlySavedField(probeNative,request);
  for(const auto &name:{"id","name","type","read_only","cloaked"})require(a[name]==b[name],"Read-only consumer descriptor metadata changed");
  readonlyChecks.append(QJsonObject{{"request",request},{"before",a["value"]},{"after",b["value"]}});
  save("readonly-checks.json",QJsonDocument(readonlyChecks).toJson());
  require(same(a["value"],b["value"]),"Read-only value not reconstructed by real reload consumer");
 }
'''

PROOF = r'''
 require(readonlyChecks.size()==spec["readonly_roundtrip_fields"].toArray().size()&&!readonlyChecks.isEmpty(),"Read-only consumer checks incomplete");
 const QJsonObject readonlyProof{{"checkpoint","save-readonly-initialization"},{"outcome","passed"},{"case",key},{"checks",readonlyChecks},{"relative_bound",1e-12},
  {"facets",QJsonArray{"unchanged-original-Save-editor-picker-source-writable-and-reload-assertions","actual-original-and-reloaded-runtime-typed-readonly-getters","real-reload-initialization-or-explicit-native-consumer","same-descriptor-identity-metadata-and-original-numeric-bound"}},
  {"scope","Only explicit derived fields and retained resource definitions; initializer history, session IDs, absent getters and complete FRC!E132 remain separate."}};
 save("readonly-proof.json",QJsonDocument(readonlyProof).toJson());
 std::cout<<"NASA_CHECK "<<QJsonDocument(readonlyProof).toJson(QJsonDocument::Compact).constData()<<std::endl;
'''


def block(name, code):
    return '\n/* BEGIN_SAVE_READONLY_'+name+' */\n'+code+'\n/* END_SAVE_READONLY_'+name+' */\n'


def driver(base):
    result = base
    inserts = [('HELPER', 'int main(int argc,char **argv)', HELPER, False),
               ('LOCAL', ' QDir::setCurrent(QFileInfo(startup).absolutePath());',
                ' QJsonArray readonlyChecks;\n require(!spec["readonly_roundtrip_fields"].toArray().isEmpty()&&!spec.value("reload_probe").toString().isEmpty(),"Read-only consumer specification absent");\n', False),
               ('CHECK', 'save("reload-probe-native.json",QJsonDocument(probeNative).toJson());', CHECK, True),
               ('PROOF', 'std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;', PROOF, True)]
    for name, anchor, code, after in inserts:
        assert result.count(anchor) == 1, (name, 'generic source anchor changed')
        result = result.replace(anchor, anchor+block(name,code) if after else block(name,code)+anchor)
    restored = re.sub(r'\n/\* BEGIN_SAVE_READONLY_(\w+) \*/\n.*?\n/\* END_SAVE_READONLY_\1 \*/\n', '', result, flags=re.S)
    assert restored == base, 'The extension changed original generic driver bytes'
    return result


def node(snapshot, object_path):
    parts = object_path.split('/')
    result = snapshot[parts[0]]
    for part in parts[1:]:
        result = result['owned'][int(part[6:-1])]
    return result


def generate(review_path):
    from AuditSaveResourceEvidence import equal
    review = json.loads(review_path.read_text())
    assert review['selected_cases'] == 152 and review['counts']['changed_value_observations'] == 153
    grouped, reused, pending = {}, [], []
    for entry in review['differences']:
        if (entry['native_type'] in {'Variable','String'} or
                entry['field'] in {'NAIFId','NAIFIdReferenceFrame','BodyNumber'}):
            pending.append(entry)
            continue
        probe = Path(entry['runtime']).with_name('reload-probe-native.json')
        if probe.exists():
            data = node(json.loads(probe.read_text()), entry['object_path'])
            matches = [f for f in data['fields'] if f['id'] == entry['descriptor_id']
                       and f['name'] == entry['field'] and f['type'] == entry['parameter_type']]
            if len(matches) == 1 and equal(entry['before'], matches[0].get('value')):
                reused.append(dict(entry, retained_probe=str(probe), retained_probe_sha256=SHA(probe)))
                continue
        grouped.setdefault(entry['test'], []).append(entry)
    assert len(grouped) == 18 and sum(map(len, grouped.values())) == 65
    assert len(reused) == 66 and len(pending) == 22
    cases = []
    for test, entries in sorted(grouped.items()):
        original_path = Path(entries[0]['runtime']).with_name('case.json')
        original = json.loads(original_path.read_text())
        case = copy.deepcopy(original)
        case['key'] = test.removeprefix('QtGui.NasaSave')
        assert 'ReloadAlive' not in case['setup']
        case['setup'] += 'Create Variable ReloadAlive;\nReloadAlive = 41;\n'
        for name in ['selected','objects']:
            case[name] = list(case[name])+['ReloadAlive']
        case['definitions'] = list(case['definitions'])+['Create Variable ReloadAlive']
        case['filename'] = '_'.join(case['selected'])+'.data'
        case['witnesses'] = list(case['witnesses'])+[dict(object='ReloadAlive',field='Value',type='Real',expected=41)]
        consumer = original.get('reload_probe') or (original.get('before_save','') if original['key']=='DynamicDataAutomatic' else '')
        case['reload_probe'] = consumer+'ReloadAlive = ReloadAlive + 1;\n'
        case['probe_witnesses'] = list(original.get('probe_witnesses',[]))+[dict(object='ReloadAlive',field='Value',type='Real',expected=42)]
        requests = []
        for entry in entries:
            parts = entry['object_path'].split('/')
            requests.append(dict(object=parts[0],owned_indices=[int(p[6:-1]) for p in parts[1:]],
                                 native_type=entry['native_type'],field=entry['field'],type=entry['parameter_type']))
        case['readonly_roundtrip_fields'] = requests
        case['original_case_file'] = str(original_path)
        case['original_case_sha256'] = SHA(original_path)
        case['original_test'] = test
        cases.append(case)
    return dict(schema_version=1,scope='Only missing post-reload derived-field consumers; no complete FRC!E132 credit.',
                original_driver_sha256=SHA(HERE/'NasaSaveResourceTests.cpp'),readonly_review=str(review_path),
                readonly_review_sha256=SHA(review_path),cases=cases,reused_headless_probe_observations=reused,
                pending_source_review_observations=pending,new_native_fields=65,reused_native_fields=66,
                pending_observed_value_changes=22,whole_FRC_E132_qualified=False,new_requirement_credit=0)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--review',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--driver',type=Path,required=True)
    args = parser.parse_args()
    matrix = generate(args.review.resolve())
    with args.output.open('x') as stream:json.dump(matrix,stream,indent=2);stream.write('\n')
    with args.driver.open('x') as stream:stream.write(driver((HERE/'NasaSaveResourceTests.cpp').read_text()))
    print(json.dumps(dict(cases=len(matrix['cases']),new_fields=matrix['new_native_fields'],reused_fields=matrix['reused_native_fields'],pending=matrix['pending_observed_value_changes'])))
