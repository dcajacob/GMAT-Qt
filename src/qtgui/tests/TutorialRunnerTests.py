"""Contracts protecting tutorial construction/reference separation and results."""
import copy
import argparse
import importlib.util
import json
from pathlib import Path
from types import SimpleNamespace
import sys
import tempfile
import unittest
from unittest import mock
import xml.etree.ElementTree as ET

sys.path.insert(0, str(Path(__file__).parent))
from RunTutorials import RECIPES, REPO, TutorialSession, compare_outputs, compare_states, normalized_source, relocate, control_target, semantic_action, reference_milestone, configure_display, scale_value
from IsolatedX11Session import IsolatedX11Session
from AnalyzeTutorialGoals import analyze


class TutorialContracts(unittest.TestCase):
    def test_visible_scaling_uses_host_and_keeps_explicit_pixel_sizes(self):
        host={'scale':3.0,'logical_size':[1536,960],'available_logical_size':[1469,928]}
        args=SimpleNamespace(visible=True,driver='qttest',scale=None,width=None,height=None)
        with mock.patch('RunTutorials.host_display_info',return_value=host):
            configure_display(args,Path('/unused/probe'))
        self.assertEqual((args.scale,args.scale_source),(3.0,'host display'))
        self.assertGreater(args.width,2560)
        self.assertLessEqual(args.width,1469*3)
        self.assertLessEqual(args.height,928*3)
        args=SimpleNamespace(visible=True,driver='qttest',scale=1.5,width=1920,height=1080)
        with mock.patch('RunTutorials.host_display_info',return_value=host):
            configure_display(args,Path('/unused/probe'))
        self.assertEqual((args.scale,args.width,args.height),(1.5,1920,1080))

    def test_headless_scaling_stays_deterministic_and_environment_uses_override(self):
        args=SimpleNamespace(visible=False,driver='qttest',scale=None,width=None,height=None)
        with mock.patch('RunTutorials.host_display_info') as query:
            configure_display(args,Path('/unused/probe'))
            query.assert_not_called()
        self.assertEqual((args.scale,args.width,args.height),(1.0,1600,1200))
        session=object.__new__(TutorialSession)
        session.directory=Path('/tmp/own-session')
        session.work=Path('/tmp/own-case');session.private=Path('/tmp/private');session.seed=1;session.ui_scale=1.75
        with mock.patch.object(IsolatedX11Session,'_environment',return_value={'QT_SCALE_FACTOR':'99'}):
            self.assertEqual(session._environment()['QT_SCALE_FACTOR'],'1.75')
        self.assertIsNone(scale_value('auto'))
        for invalid in ('nan','inf','0','5','larger'):
            with self.assertRaises(argparse.ArgumentTypeError):scale_value(invalid)

    def test_reference_milestone_merges_only_explicit_completed_components(self):
        a={'reference_spacecraft':['SimSat'],'run':{'status':'Mission completed','spacecraft':{'SimSat':1},'output_files':{'obs':{'sha256':'a'}}}}
        b={'reference_spacecraft':['EstSat'],'reference_milestone':True,'run':{'status':'Mission completed','spacecraft':{'EstSat':2},'output_files':{'report':{'sha256':'b'}}}}
        later={'run':{'status':'Mission completed','spacecraft':{'EstSat':3},'output_files':{}}}
        records=[a,b,later];before=copy.deepcopy(records)
        result=reference_milestone(records)['reference_milestone_run']
        self.assertEqual(result['spacecraft'],{'SimSat':1,'EstSat':2})
        self.assertEqual(set(result['output_files']),{'obs','report'})
        self.assertEqual(records,before)
        self.assertEqual(reference_milestone([a,later]),{})
        b['run']['status']='Mission failed'
        with self.assertRaisesRegex(RuntimeError,'no completed GUI run'):reference_milestone([a,b])
        b['run']['status']='Mission completed';b['run']['output_files']['obs']={'sha256':'different'}
        with self.assertRaisesRegex(RuntimeError,'output collision'):reference_milestone([a,b])

    def test_long_viewing_pauses_keep_shared_desktop_waits_bounded(self):
        session=object.__new__(TutorialSession)
        with mock.patch.object(IsolatedX11Session,'wait') as wait:
            session.wait(4500)
            self.assertEqual([call.args[0] for call in wait.call_args_list],[2000,2000,500])

    def test_target_convergence_does_not_replace_goal_tolerance_checks(self):
        row='Sat.Earth.RMAG Desired: 7000 Achieved: 7000.01 Variance: -0.01\n'
        self.assertFalse(analyze(row,{'Sat.Earth.RMAG':.1})['all_accepted_goals_within_tolerance'])
        self.assertTrue(analyze(row+'*** The Targeter converged!',{'Sat.Earth.RMAG':.1})['all_accepted_goals_within_tolerance'])
        self.assertFalse(analyze(row+'*** The Targeter converged!',{'Sat.Earth.RMAG':.001})['all_accepted_goals_within_tolerance'])
        self.assertFalse(analyze(row+'*** The Targeter converged!',{})['all_accepted_goals_within_tolerance'])

    def test_ace_coordinate_stop_explicitly_replaces_template_value(self):
        recipe=json.loads(next(RECIPES.glob('17-*.json')).read_text())
        actions=recipe['stages'][0]['actions']
        index=next(i for i,a in enumerate(actions) if a.get('text')=='ACE.EarthSunL1CS.Y')
        apply=next(i for i,a in enumerate(actions[index+1:],index+1) if a.get('target_text')=='Apply')
        self.assertTrue(any(a.get('target_name')=='propagationDuration' and a.get('text')=='0' for a in actions[index:apply]))

    def test_qttest_rejects_coordinate_fallback_and_strips_recorded_positions(self):
        with self.assertRaisesRegex(RuntimeError, 'coordinate replay is disabled'):
            semantic_action({'action': 'click', 'x': 100, 'y': 200}, Path('/tmp/own-case'))
        action=semantic_action({'action': 'click', 'x': 100, 'y': 200, 'target_name': 'saveButton'}, Path('/tmp/own-case'))
        self.assertEqual(action, {'action': 'click', 'target_name': 'saveButton'})

    def test_published_chapters_have_distinct_recordings(self):
        ns = {'xi': 'http://www.w3.org/2001/XInclude'}
        chapters = ET.parse(REPO / 'doc/help/src/Part_Tutorials.xml').findall('xi:include', ns)
        recipes = [json.loads(p.read_text()) for p in sorted(RECIPES.glob('[0-9][0-9]-*.json'))]
        self.assertEqual(len(recipes), len(chapters))
        self.assertEqual([Path(r['guide']).name for r in recipes], [c.attrib['href'] for c in chapters])
        self.assertEqual(len({r['id'] for r in recipes}), len(recipes))
        for recipe in recipes:
            first = recipe
            if recipe.get('prerequisites'):
                first = json.loads((RECIPES / (recipe['prerequisites'][0]+'.json')).read_text())
            self.assertIsNone(first['stages'][0]['input'], recipe['id'])
            self.assertTrue(recipe['guide_sha256'])
            self.assertTrue(recipe['stages'][0]['actions'])
            if recipe['reference']:
                self.assertTrue((REPO / recipe['reference']).is_file(), recipe['id'])

    def test_every_published_recipe_resolves_without_coordinate_fallback(self):
        for path in RECIPES.glob('[0-9][0-9]-*.json'):
            for stage in json.loads(path.read_text())['stages']:
                for index, action in enumerate(stage['actions']):
                    if action['action'] in {'screenshot','wait'}:
                        continue
                    with self.subTest(recipe=path.name, action=index+1):
                        value=semantic_action(action,Path('/tmp/own-case'))
                        self.assertNotIn('x',value)
                        self.assertNotIn('y',value)
                        self.assertNotIn('anchor',value)

    def test_no_reference_mission_or_historical_output_is_typed_during_construction(self):
        for path in RECIPES.glob('[0-9][0-9]-*.json'):
            recipe = json.loads(path.read_text())
            for stage in recipe['stages']:
                for action in stage['actions']:
                    if action['action'] != 'text':
                        continue
                    text = action['text']
                    self.assertNotIn('/application/samples/', text, path.name)
                    self.assertNotIn('/application/docs/help/files/scripts/', text, path.name)
                    self.assertNotIn('/home/dan/', text, path.name)
                    self.assertNotIn('/tmp/', text, path.name)
                    self.assertNotIn('build/example-qualification/', text, path.name)

    def test_continuations_name_only_preceding_gui_file_operations(self):
        for path in RECIPES.glob('[0-9][0-9]-*.json'):
            recipe = json.loads(path.read_text())
            own_files = set()
            stages = []
            for dependency in recipe.get('prerequisites', []):
                stages += json.loads((RECIPES/(dependency+'.json')).read_text())['stages']
            for stage in stages+recipe['stages']:
                if stage.get('input'):
                    self.assertIn(stage['input'], own_files, (path.name, stage['name']))
                for action in stage['actions']:
                    text = action.get('text', '')
                    if text.startswith('${WORK}/') and text.endswith('.script') and '\n' not in text:
                        own_files.add(text.removeprefix('${WORK}/'))

    def test_comments_do_not_hide_executable_differences(self):
        self.assertEqual(normalized_source("% date\nGMAT A = '50%'; % comment\n"), normalized_source("A='50%';"))
        self.assertEqual(normalized_source('A = [1 2];'), normalized_source('A=[ 1   2 ];'))
        self.assertNotEqual(normalized_source('A = [1 2];'), normalized_source('A = [12];'))
        self.assertNotEqual(normalized_source("A='some text';"), normalized_source("A='sometext';"))
        self.assertNotEqual(normalized_source('A = 1;'), normalized_source('A = 2;'))
        self.assertNotEqual(normalized_source('A = 1;\nB = 2;'), normalized_source('B = 2;\nA = 1;'))

    def state(self):
        return {'spacecraft': {'Sat': {'epoch_a1mjd': 21545.0, 'j2000_body': 'Earth',
                'cartesian_km_km_s': [7000., 0., 0., 0., 7.5, 0.], 'total_mass_kg': 850.}}}

    def test_endpoint_comparison_requires_observed_objects_finite_values_and_matching_frames(self):
        a = self.state()
        self.assertTrue(compare_states(a, copy.deepcopy(a))['matched'])
        self.assertFalse(compare_states({}, {})['matched'])
        b = copy.deepcopy(a)
        b['spacecraft']['Sat']['cartesian_km_km_s'][0] += .001
        self.assertFalse(compare_states(a, b)['matched'])
        b['spacecraft']['Sat']['cartesian_km_km_s'][0] = float('nan')
        self.assertFalse(compare_states(a, b)['matched'])
        b = copy.deepcopy(a)
        b['spacecraft']['Sat']['j2000_body'] = 'Mars'
        self.assertFalse(compare_states(a, b)['matched'])
        b = copy.deepcopy(a)
        b['spacecraft']['Extra'] = b['spacecraft']['Sat']
        self.assertFalse(compare_states(a, b)['matched'])

    def test_complete_output_comparison_does_not_accept_missing_snapshots_or_missing_files(self):
        self.assertFalse(compare_outputs({}, {})['matched'])
        self.assertTrue(compare_outputs({'output_files': {}}, {'output_files': {}})['matched'])
        a = {'output_files': {'report.txt': {'bytes': 3, 'sha256': 'abc'}}}
        self.assertTrue(compare_outputs(a, copy.deepcopy(a))['matched'])
        self.assertFalse(compare_outputs(a, {'output_files': {}})['matched'])
        b = copy.deepcopy(a)
        b['output_files']['report.txt']['sha256'] = 'different'
        self.assertFalse(compare_outputs(a, b)['matched'])

    def test_paths_are_relocated_without_reading_an_old_authored_mission(self):
        with tempfile.TemporaryDirectory() as directory:
            self.assertEqual(relocate('${WORK}/mine.script', Path(directory)), directory + '/mine.script')

    def test_active_panel_wins_after_window_positions_change(self):
        controls = [{'role': 'button', 'name': '', 'text': 'Apply', 'rect': [1400, 850, 80, 25], 'active_panel': False},
                    {'role': 'button', 'name': '', 'text': 'Apply', 'rect': [750, 980, 80, 25], 'active_panel': True}]
        point = control_target({'x': 1460, 'y': 987, 'hint': 'Apply'}, controls)
        self.assertEqual(point, (790, 992))

    def test_tree_target_preserves_context_menu_horizontal_origin(self):
        controls = [{'role': 'tree', 'text': 'Burns', 'rect': [43, 303, 224, 17], 'active_panel': True}]
        self.assertEqual(control_target({'x': 63, 'y': 296, 'target_text': 'Burns', 'target_role': 'tree'}, controls), (63, 311))

    def test_named_field_requires_actual_scrolling_in_its_active_panel(self):
        controls = [{'role': 'widget', 'name': 'propagationDuration', 'rect': [400, 300, 300, 25], 'active_panel': False},
                    {'role': 'widget', 'name': 'propagationDuration', 'rect': [1000, 540, 400, 25],
                     'active_panel': True, 'offscreen': True, 'scroll_rect': [914, 356, 658, 168], 'scroll_direction': 1}]
        self.assertEqual(control_target({'x': 1232, 'y': 553, 'target_name': 'propagationDuration'}, controls),
                         {'scroll': [914, 356, 658, 168], 'direction': 1})
        with self.assertRaises(RuntimeError):
            control_target({'x': 1232, 'y': 553, 'target_name': 'missing'}, controls)

    def test_recorded_field_label_does_not_include_its_old_value(self):
        controls = [{'role': 'label', 'text': 'Name (optional)', 'rect': [900, 200, 100, 25],
                     'buddy': [1010, 200, 300, 25], 'active_panel': True}]
        self.assertEqual(control_target({'x': 1200, 'y': 186, 'field_hint': 'Name (optional) | Old command name'}, controls),
                         (1160, 212))


if __name__ == '__main__':
    unittest.main()
