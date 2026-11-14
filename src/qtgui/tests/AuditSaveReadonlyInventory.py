"""Compare retained read-only native descriptors without inferring persistence roles."""
import argparse
import collections
import hashlib
import json
from pathlib import Path

from AuditSaveResourceEvidence import equal

SHA = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
LOAD = lambda p: json.loads(Path(p).read_text())


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def audit(inventory_path, output):
    inventory = LOAD(inventory_path)
    require(inventory['status'] == 'inventory-only' and inventory['selected_cases'] == 152,
            'Wrong retained Save inventory')
    artifacts = {str(inventory_path): SHA(inventory_path)}
    descriptors = {}
    differences, unavailable, orders = [], [], []
    totals = collections.Counter()

    def read(path):
        path = Path(path)
        require(str(path) in inventory['artifacts'] and SHA(path) == inventory['artifacts'][str(path)],
                'Unbound/changed retained capture: '+str(path))
        artifacts[str(path)] = SHA(path)
        return LOAD(path)

    def walk(a, b, test, object_path, runtime_path, reload_path):
        require(set(a) == set(b) == {'name', 'type', 'fields', 'owned'}
                and a['type'] == b['type'] and a['name'] == b['name'], 'Native identity/schema changed: '+object_path)
        require(len(a['fields']) == len(b['fields']), 'Native descriptor count changed')
        for index, (left, right) in enumerate(zip(a['fields'], b['fields'])):
            require(left['id'] == right['id'] == index
                    and all(left[k] == right[k] for k in ['name', 'type', 'read_only', 'cloaked']),
                    'Native descriptor identity/metadata changed')
            if not left['read_only']:
                continue
            key = (a['type'], index, left['name'], left['type'])
            row = descriptors.setdefault(key, dict(native_type=a['type'], descriptor_id=index,
                    field=left['name'], parameter_type=left['type'], observations=0,
                    equal_value_observations=0, changed_value_observations=0,
                    unavailable_value_observations=0, examples=[]))
            row['observations'] += 1
            totals['readonly_observations'] += 1
            context = dict(test=test, object_path=object_path, native_type=a['type'], descriptor_id=index,
                           field=left['name'], parameter_type=left['type'], runtime=str(runtime_path), reloaded=str(reload_path))
            if 'value' in left and 'value' in right:
                same = equal(left['value'], right['value'])
                category = 'equal_value_observations' if same else 'changed_value_observations'
                row[category] += 1;totals[category] += 1
                if not same:
                    differences.append(dict(context, before=left['value'], after=right['value']))
            else:
                row['unavailable_value_observations'] += 1;totals['unavailable_value_observations'] += 1
                unavailable.append(dict(context, before=left, after=right))
            if len(row['examples']) < 2:
                row['examples'].append(context)
        require(len(a['owned']) == len(b['owned']), 'Native owned count changed')
        children = b['owned']
        if a['type'] in {'ForceModel', 'ODEModel'}:
            identity = lambda child: (child['type'], child['name'])
            aa, bb = [identity(x) for x in a['owned']], [identity(x) for x in b['owned']]
            require(len(set(aa)) == len(aa) and len(set(bb)) == len(bb) and set(aa) == set(bb),
                    'Missing/ambiguous force identity')
            orders.append(dict(test=test, object_path=object_path, before=aa, after=bb, equal=aa == bb))
            indexed = {identity(x): x for x in children}
            children = [indexed[k] for k in aa]
        for index, (left, right) in enumerate(zip(a['owned'], children)):
            walk(left, right, test, object_path+'/owned['+str(index)+']', runtime_path, reload_path)

    runtime_paths = sorted(Path(p) for p in inventory['artifacts'] if Path(p).name == 'runtime-native.json')
    require(len(runtime_paths) == 152, 'Missing/extra selected headless capture')
    for runtime_path in runtime_paths:
        reload_path = runtime_path.with_name('reloaded-native.json')
        a, b = read(runtime_path), read(reload_path)
        require(set(a) == set(b), 'Native root set changed')
        for name in sorted(a):
            walk(a[name], b[name], runtime_path.parent.name, name, runtime_path, reload_path)
    rows = [dict(descriptors[k], persistence_role='unreviewed-owning-source-and-consumer-semantics') for k in sorted(descriptors)]
    require(totals['readonly_observations'] == sum(totals[k] for k in
            ['equal_value_observations', 'changed_value_observations', 'unavailable_value_observations']),
            'Read-only observations were silently excluded')
    result = dict(status='inventory-only-no-persistence-exclusions', selected_cases=152,
        descriptor_identity='native type + native descriptor ID + label + parameter type; duplicate labels remain distinct',
        readonly_descriptor_keys=len(rows), counts=dict(totals),
        changed_descriptor_keys=sum(r['changed_value_observations'] > 0 for r in rows),
        unavailable_descriptor_keys=sum(r['unavailable_value_observations'] > 0 for r in rows),
        descriptors=rows, differences=differences, unavailable=unavailable, force_orders=orders,
        artifacts=artifacts, reader_sha256=SHA(__file__), native_tests_rerun=0,
        fields_excluded_as_transient=0, new_requirement_credit=0, whole_FRC_E132_qualified=False,
        rule='Equal values in retained cases are bounded observations. Differences require source/consumer review; getter absence is unresolved. No field is excluded merely because it is readonly or cloaked.')
    with output.open('x') as stream:json.dump(result,stream,indent=2);stream.write('\n')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args();r = audit(args.inventory.resolve(),args.output.resolve())
    print(json.dumps({k:r[k] for k in ['status','selected_cases','readonly_descriptor_keys','counts',
                                     'changed_descriptor_keys','unavailable_descriptor_keys','native_tests_rerun']}))
