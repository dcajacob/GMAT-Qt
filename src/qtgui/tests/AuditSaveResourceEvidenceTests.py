"""Negative controls for the independent bounded native Save evidence audit."""
import copy
import json
import hashlib
import tempfile
from pathlib import Path
import math
import unittest
from AuditSaveResourceEvidence import compare, equal, check_witnesses, emitted_case_metadata, materialize_case_inputs
from AuditCentralBodyPropagationAcceptance import passing_visible_projection


def sample():
    return dict(name='Subject', type='ExampleResource', owned=[], fields=[
        dict(id=0, name='Mass', type='Real', read_only=False, cloaked=False, value=12.5),
        dict(id=1, name='Reference', type='Object', read_only=False, cloaked=False, value='Mars'),
        dict(id=2, name='Flags', type='BooleanArray', read_only=False, cloaked=False, value=[False, True]),
        dict(id=3, name='Computed', type='Real', read_only=True, cloaked=False,
             not_observed='Computed output requires initialized attachment')])


class NativeSaveAuditTests(unittest.TestCase):
    def rejects(self, mutate):
        a, b = sample(), sample()
        mutate(a, b)
        with self.assertRaises((ValueError, AssertionError)):
            compare(a, b, 'Subject')

    def test_valid_writable_inventory(self):
        self.assertEqual(compare(sample(), sample()), 3)

    def test_owned_object_is_checked(self):
        a, b = sample(), sample()
        a['owned'] = [sample()];b['owned'] = [sample()]
        self.assertEqual(compare(a, b), 6)
        b['owned'][0]['fields'][0]['value'] = 18
        with self.assertRaises(ValueError): compare(a, b)

    def test_read_only_computed_alias_keeps_both_native_ids(self):
        a, b = sample(), sample()
        for x in (a, b): x['fields'][3]['name'] = 'Mass'
        self.assertEqual(compare(a, b), 3)

    def test_read_only_literal_is_checked_after_reload(self):
        case = dict(witnesses=[dict(object='Subject', field='Computed', type='Real', expected=12.5)])
        state = sample();state['fields'][3].pop('not_observed');state['fields'][3]['value'] = 12.5
        check_witnesses(case, {'Subject': state}, 'runtime')
        state['fields'][3]['value'] = 0
        with self.assertRaises(ValueError): check_witnesses(case, {'Subject': state}, 'reloaded')

    def test_automatic_color_mode_is_preserved(self):
        a, b = sample(), sample()
        for x in (a, b): x['fields'][2].update(type='StringArray', value=[[[0, False]]])
        b['fields'][2]['value'] = [[[0, True]]]
        with self.assertRaises(ValueError): compare(a, b)

    def test_missing_literal_value_is_rejected(self):
        case = dict(witnesses=[dict(object='Subject', field='Mass', type='Real', expected=12.5)])
        state = sample();del state['fields'][0]['value']
        with self.assertRaises((ValueError, KeyError)): check_witnesses(case, {'Subject': state}, 'reloaded')

    def test_mixed_visible_receipt_keeps_its_failure_history(self):
        receipt = dict(mode='visible', counts={'passed': 1, 'failed': 1}, results=[
            dict(name='Good', status='passed'), dict(name='Bad', status='failed'),
            dict(name='Unrun', status='not-run')])
        original = json.dumps(receipt, sort_keys=True)
        projection = passing_visible_projection(receipt)
        self.assertEqual(json.dumps(receipt, sort_keys=True), original)
        self.assertEqual([r['name'] for r in projection['results']], ['Good', 'Unrun'])
        self.assertEqual(projection['counts'], {'passed': 1, 'failed': 1})
        self.assertEqual(projection['visible_validation_scope']['excluded_failed_tests'], ['Bad'])

    def test_all_green_visibility_contract_is_unchanged(self):
        receipt = dict(results=[dict(name='Good', status='passed')])
        self.assertIs(passing_visible_projection(receipt), receipt)

    def test_failed_visible_case_never_enters_passing_projection(self):
        receipt = dict(results=[dict(name='Bad', status='failed')])
        self.assertEqual(passing_visible_projection(receipt)['results'], [])
        self.assertEqual(receipt['results'][0]['status'], 'failed')

    def test_known_missing_optional_lookup_adds_only_metadata_null(self):
        case = dict(key='Example', setup='unchanged')
        expected = emitted_case_metadata(case, 'spec["before_save"].toString()')
        self.assertEqual(expected, dict(key='Example', setup='unchanged', before_save=None))
        self.assertNotIn('before_save', case)

    def test_nonmutating_optional_lookup_keeps_metadata_exact(self):
        case = dict(key='Example', setup='unchanged')
        self.assertEqual(emitted_case_metadata(case, 'spec.value("before_save").toString()'), case)

    def test_nonempty_optional_command_is_never_normalized(self):
        case = dict(key='Example', before_save='UpdateDynamicData Subject;\n')
        self.assertEqual(emitted_case_metadata(case, 'spec["before_save"].toString()'), case)

    def test_missing_declared_literal_descriptor_is_rejected(self):
        case = dict(witnesses=[dict(object='Subject', field='Interval', type='Real', expected=60)])
        with self.assertRaises((ValueError, KeyError)): check_witnesses(case, {'Subject': sample()}, 'reloaded')

    def asset_fixture(self, directory):
        work = Path(directory);root = work/'fixture-inputs';data = 'Δ fixture\n'.encode()
        case = dict(setup="Reader.Filename = '@FILES@/input α.txt';\n",
                    fixture_files={'input α.txt': data.decode()},
                    witnesses=[dict(object='Reader', field='Filename', type='Filename',
                                    expected='@FILES@/input α.txt')])
        artifact = work/'fixture-asset-0.txt';artifact.write_bytes(data)
        manifest = dict(root=str(root), files=[dict(relative='input α.txt',
            path=str(root/'input α.txt'), artifact=artifact.name,
            sha256=hashlib.sha256(data).hexdigest(), bytes=len(data))])
        target = work/'fixture-assets.json';target.write_text(json.dumps(manifest))
        return case, work, {str(target), str(artifact)}, manifest

    def test_declared_asset_bytes_and_expanded_literals(self):
        with tempfile.TemporaryDirectory() as directory:
            case, work, artifacts, manifest = self.asset_fixture(directory)
            original = copy.deepcopy(case)
            actual = materialize_case_inputs(case, work, artifacts)
            self.assertEqual(actual['witnesses'][0]['expected'], manifest['files'][0]['path'])
            self.assertEqual(actual['setup'], "Reader.Filename = '"+manifest['files'][0]['path']+"';\n")
            self.assertEqual(case, original)

    def test_corrupt_asset_bytes_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            case, work, artifacts, manifest = self.asset_fixture(directory)
            (work/'fixture-asset-0.txt').write_text('changed')
            with self.assertRaises(ValueError): materialize_case_inputs(case, work, artifacts)

    def test_unbound_asset_file_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            case, work, artifacts, manifest = self.asset_fixture(directory)
            artifacts.remove(str(work/'fixture-asset-0.txt'))
            with self.assertRaises(ValueError): materialize_case_inputs(case, work, artifacts)

    def test_unbound_asset_manifest_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            case, work, artifacts, manifest = self.asset_fixture(directory)
            artifacts.remove(str(work/'fixture-assets.json'))
            with self.assertRaises(ValueError): materialize_case_inputs(case, work, artifacts)

    def test_wrong_asset_digest_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            case, work, artifacts, manifest = self.asset_fixture(directory)
            manifest['files'][0]['sha256'] = '0'*64
            (work/'fixture-assets.json').write_text(json.dumps(manifest))
            with self.assertRaises(ValueError): materialize_case_inputs(case, work, artifacts)

    def test_wrong_asset_size_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            case, work, artifacts, manifest = self.asset_fixture(directory)
            manifest['files'][0]['bytes'] += 1
            (work/'fixture-assets.json').write_text(json.dumps(manifest))
            with self.assertRaises(ValueError): materialize_case_inputs(case, work, artifacts)

    def test_relative_asset_root_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            case, work, artifacts, manifest = self.asset_fixture(directory)
            manifest['root'] = 'relative/fixture-inputs'
            (work/'fixture-assets.json').write_text(json.dumps(manifest))
            with self.assertRaises(ValueError): materialize_case_inputs(case, work, artifacts)

    def test_missing_asset_record_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            case, work, artifacts, manifest = self.asset_fixture(directory)
            manifest['files'] = []
            (work/'fixture-assets.json').write_text(json.dumps(manifest))
            with self.assertRaises(ValueError): materialize_case_inputs(case, work, artifacts)

    def test_asset_parent_traversal_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            case, work, artifacts, manifest = self.asset_fixture(directory)
            case['fixture_files'] = {'../outside.txt': 'Δ fixture\n'}
            manifest['files'][0]['relative'] = '../outside.txt'
            (work/'fixture-assets.json').write_text(json.dumps(manifest))
            with self.assertRaises(ValueError): materialize_case_inputs(case, work, artifacts)

    def test_output_only_placeholder_has_no_invented_input_files(self):
        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory);root = work/'fixture-inputs';manifest = work/'fixture-assets.json'
            manifest.write_text(json.dumps(dict(root=str(root), files=[])))
            case = dict(setup="Output.Filename = '@FILES@/out.oem';\n", witnesses=[])
            actual = materialize_case_inputs(case, work, {str(manifest)})
            self.assertIn(str(root), actual['setup'])

    def test_unexpected_asset_manifest_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory);(work/'fixture-assets.json').write_text('{}')
            with self.assertRaises(ValueError): materialize_case_inputs(dict(setup='', witnesses=[]), work, set())

    def test_changed_reference(self):
        self.rejects(lambda a, b: b['fields'][1].update(value='Earth'))

    def test_changed_scalar(self):
        self.rejects(lambda a, b: b['fields'][0].update(value=12.6))

    def test_changed_flag_order(self):
        self.rejects(lambda a, b: b['fields'][2].update(value=[True, False]))

    def test_missing_writable_getter(self):
        def mutate(a, b):
            for x in (a, b):
                del x['fields'][0]['value'];x['fields'][0]['getter_error'] = 'unsupported'
        self.rejects(mutate)

    def test_swallowed_getter_error(self):
        self.rejects(lambda a, b: b['fields'][0].update(getter_error='still failed'))

    def test_skipped_writable_getter(self):
        self.rejects(lambda a, b: b['fields'][0].update(not_observed='skip'))

    def test_changed_metadata(self):
        self.rejects(lambda a, b: b['fields'][0].update(read_only=True))

    def test_truncated_inventory(self):
        self.rejects(lambda a, b: b['fields'].pop())

    def test_duplicate_parameter_name(self):
        def mutate(a, b):
            for x in (a, b): x['fields'][1]['name'] = 'Mass'
        self.rejects(mutate)

    def test_noncontiguous_parameter_ids(self):
        self.rejects(lambda a, b: b['fields'][1].update(id=7))

    def test_missing_owned_object(self):
        self.rejects(lambda a, b: a['owned'].append(sample()))

    def test_changed_type(self):
        self.rejects(lambda a, b: b.update(type='OtherResource'))

    def test_changed_name(self):
        self.rejects(lambda a, b: b.update(name='Other'))

    def test_nonfinite_values(self):
        for value in (math.nan, math.inf, -math.inf):
            self.rejects(lambda a, b: b['fields'][0].update(value=value))

    def test_boolean_is_not_numeric(self):
        self.assertFalse(equal(False, 0));self.assertFalse(equal(True, 1))

    def test_array_length_is_not_ignored(self):
        self.assertFalse(equal([1, 2], [1, 2, 3]))

    def test_numeric_roundoff_bound_is_fixed(self):
        self.assertTrue(equal(12.5, 12.5+1e-12));self.assertFalse(equal(12.5, 12.5+1e-8))


class OwnedNativeWitnessTests(unittest.TestCase):
    def tree(self):
        root=sample();child=sample();child.update(name='Earth',type='GravityField')
        child['fields'][0]['value']=3.75;root['owned']=[child]
        return {'Subject':root}

    def witness(self, **changes):
        w=dict(object='Subject',owned_types=['GravityField'],field='Mass',type='Real',expected=3.75)
        w.update(changes);return {'witnesses':[w]}

    def test_owned_value_does_not_substitute_root(self):
        check_witnesses(self.witness(),self.tree(),'runtime')

    def test_nested_owned_path(self):
        tree=self.tree();child=tree['Subject']['owned'][0];inner=sample();inner['type']='Inner';child['owned']=[inner]
        check_witnesses(self.witness(owned_types=['GravityField','Inner'],expected=12.5),tree,'reloaded')

    def test_missing_owned_path_rejected(self):
        with self.assertRaises(ValueError):check_witnesses(self.witness(owned_types=['Missing']),self.tree(),'runtime')

    def test_ambiguous_owned_path_rejected(self):
        tree=self.tree();tree['Subject']['owned'].append(copy.deepcopy(tree['Subject']['owned'][0]))
        with self.assertRaises(ValueError):check_witnesses(self.witness(),tree,'runtime')

    def test_changed_owned_value_rejected(self):
        with self.assertRaises(ValueError):check_witnesses(self.witness(expected=12.5),self.tree(),'runtime')

    def test_invalid_path_shape_rejected(self):
        for path in ['GravityField',[''],[1],None]:
            with self.subTest(path=path),self.assertRaises(ValueError):check_witnesses(self.witness(owned_types=path),self.tree(),'runtime')

    def test_empty_path_stays_on_root(self):
        check_witnesses(self.witness(owned_types=[],expected=12.5),self.tree(),'runtime')


class ForceIdentityComparisonTests(unittest.TestCase):
    def pair(self):
        root=sample();root['type']='ForceModel';one=sample();one.update(name='EarthGravity',type='GravityField');two=sample();two.update(name='EarthDrag',type='DragForce');two['fields'][0]['value']=99
        root['owned']=[one,two];after=copy.deepcopy(root);after['owned'].reverse();return root,after

    def test_force_reordering_preserves_all_fields(self):
        before,after=self.pair();self.assertEqual(compare(before,after),9)

    def test_other_owned_collections_stay_ordered(self):
        before,after=self.pair();before['type']=after['type']='OtherResource'
        with self.assertRaises(ValueError):compare(before,after)

    def test_duplicate_force_identity_rejected(self):
        before,after=self.pair();after['owned']=[copy.deepcopy(before['owned'][0]) for _ in range(2)]
        with self.assertRaises(ValueError):compare(before,after)

    def test_missing_force_identity_rejected(self):
        before,after=self.pair();after['owned'][0]['name']='Different'
        with self.assertRaises(ValueError):compare(before,after)

    def test_reordered_changed_force_value_rejected(self):
        before,after=self.pair();after['owned'][0]['fields'][0]['value']=12.5
        with self.assertRaises(ValueError):compare(before,after)


if __name__ == '__main__': unittest.main()
