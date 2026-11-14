"""Inventory retained Save descriptors/order without inferring persistence from metadata."""
import argparse
import collections
import hashlib
import json
from pathlib import Path

SHA = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
LOAD = lambda p: json.loads(Path(p).read_text())


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def inventory(component_path, output):
    component = LOAD(component_path)
    require(component['status'] == 'passed' and component['cumulative_cases'] == 152
            and not component['whole_FRC_E132_qualified'], 'Missing bounded Save component audit')
    artifacts = {str(component_path): SHA(component_path)}

    def read(path, expected=None):
        path = Path(path).resolve()
        digest = SHA(path)
        require(expected is None or digest == expected, 'Changed retained input: '+str(path))
        artifacts[str(path)] = digest
        return LOAD(path)

    entries, seen_evidence = [], set()
    path = component_path.parent.parent/'coverage95-20261101/save-orbit-modes-evidence.json'
    while path:
        require(str(path) not in seen_evidence, 'Cyclic evidence chain')
        seen_evidence.add(str(path))
        evidence = read(path)
        for selected in evidence['selected']['headless']:
            work = path.parent/selected['run']/selected['test']
            captures = []
            for phase in ['runtime-native.json', 'reloaded-native.json']:
                raw = work/phase
                require(str(raw) in evidence['artifact_sha256'], 'Selected snapshot absent from evidence bindings')
                captures.append(read(raw, evidence['artifact_sha256'][str(raw)]))
            entries.append((selected['test'], work, captures))
        parent = evidence.get('parent_evidence') or evidence.get('parent_conjoined_evidence')
        path = Path(parent) if parent else None
    for selected in component['witnesses']:
        if selected['profile'] != 'headless':
            continue
        work = Path(selected['proof']).parent
        captures = [read(work/name, component['artifacts'][str(work/name)])
                    for name in ['runtime-native.json', 'reloaded-native.json']]
        entries.append(('QtGui.NasaSaveFixedOffset'+selected['case'], work, captures))
    require(len(entries) == 152 and len({test for test, _, _ in entries}) == 152,
            'Missing/duplicate selected Save case')
    fields, orders, counts = {}, [], collections.Counter()

    def walk(before, after, test, object_path, runtime_path, reload_path):
        typename = before['type']
        require(typename == after['type'] and before['name'] == after['name'], 'Changed native object identity')
        counts[typename] += 1
        for field in before['fields']:
            key = (typename, field['name'], field['type'])
            row = fields.setdefault(key, dict(native_type=typename, field=field['name'], parameter_type=field['type'],
                    readonly_observations=0, writable_observations=0, cloaked_observations=0,
                    runtime_getter_failures=0, examples=[]))
            row['readonly_observations' if field['read_only'] else 'writable_observations'] += 1
            row['cloaked_observations'] += bool(field['cloaked'])
            row['runtime_getter_failures'] += 'getter_error' in field
            if len(row['examples']) < 2:
                row['examples'].append(dict(test=test, object_path=object_path, runtime=str(runtime_path),
                                            reloaded=str(reload_path), descriptor_id=field['id'],
                                            getter_error=field.get('getter_error')))
        if typename in {'ODEModel', 'ForceModel'}:
            a = [(o['type'], o['name']) for o in before['owned']]
            b = [(o['type'], o['name']) for o in after['owned']]
            orders.append(dict(test=test, object_path=object_path, runtime=a, reloaded=b, equal=a == b))
        # Match unique(type,name) only to descend, while preserving the complete
        # native order observations above. Do not call unequal order harmless.
        aa, bb = before['owned'], after['owned']
        require(len(aa) == len(bb), 'Owned object cardinality changed')
        if typename in {'ODEModel', 'ForceModel'}:
            require(len({(o['type'], o['name']) for o in aa}) == len(aa), 'Ambiguous owned-force identity')
            by_key = {(o['type'], o['name']): o for o in bb}
            require(set(by_key) == {(o['type'], o['name']) for o in aa}, 'Owned-force identities changed')
            bb = [by_key[o['type'], o['name']] for o in aa]
        for index, (a, b) in enumerate(zip(aa, bb)):
            walk(a, b, test, object_path+'/owned['+str(index)+']', runtime_path, reload_path)

    for test, work, (before, after) in entries:
        require(set(before) == set(after), 'Native root set changed')
        for name in before:
            walk(before[name], after[name], test, name, work/'runtime-native.json', work/'reloaded-native.json')
    descriptors = [dict(fields[key], persistence_classification='unreviewed-owning-source-semantics') for key in sorted(fields)]
    result = dict(status='inventory-only', selected_cases=152, native_types=len(counts),
                  native_type_observations=dict(sorted(counts.items())), descriptor_keys=len(fields),
                  readonly_descriptor_keys=sum(bool(r['readonly_observations']) for r in descriptors),
                  mixed_readonly_writable_descriptor_keys=sum(bool(r['readonly_observations'] and r['writable_observations']) for r in descriptors),
                  descriptor_keys_with_runtime_getter_failures=sum(bool(r['runtime_getter_failures']) for r in descriptors),
                  force_order_observations=len(orders), unequal_force_order_observations=sum(not r['equal'] for r in orders),
                  descriptors=descriptors, native_force_order=orders, artifacts=artifacts,
                  rule='Readonly/cloaked/getter failure is metadata, not evidence that state is transient. Classify each remaining field against its owning source and native consumer before excluding it from Save persistence.',
                  native_reruns=0, new_requirement_credit=0, whole_FRC_E132_qualified=False, reader_sha256=SHA(__file__))
    with output.open('x') as stream:
        json.dump(result, stream, indent=2)
        stream.write('\n')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--component', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = inventory(args.component.resolve(), args.output.resolve())
    print(json.dumps({k: result[k] for k in ['status', 'selected_cases', 'native_types', 'descriptor_keys',
                     'readonly_descriptor_keys', 'descriptor_keys_with_runtime_getter_failures',
                     'force_order_observations', 'unequal_force_order_observations']}))
