"""Offline scientific reference contracts; no GUI or native mission replay."""
from pathlib import Path
import copy, hashlib, json, tempfile, unittest
import numpy as np
import BatchOneStepOracle as oracle

HERE=Path(__file__).resolve().parent
DATA=HERE/'batch-one-step'

class ReferenceContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.result=oracle.analyze(DATA/'observations.gmd',DATA/'station-inputs.csv',DATA/'native-report.txt')
        cls.fixture=json.loads((HERE/'NasaBatchIterationCases.json').read_text())['one_step_reference']

    def test_shared_input_hashes(self):
        root=HERE.parents[2]
        for path,digest in self.fixture['inputs'].items():
            self.assertEqual(hashlib.sha256((root/path).read_bytes()).hexdigest(),digest)

    def test_universal_propagation_matches_closed_circle_both_directions(self):
        times=np.linspace(-28800,28800,401)
        actual=oracle.propagate(oracle.circle([0])[0],times)
        error=np.max(np.abs(actual-oracle.circle(times)),axis=0)
        self.assertLess(max(error[:3]),2e-7);self.assertLess(max(error[3:]),2e-10)

    def test_biased_orbits_preserve_energy_and_angular_momentum(self):
        truth=oracle.circle([0])[0];bias=np.array([1,-.5,.25,.0005,-.0003,.0002])
        for sign in [-1,1]:
            initial=truth+sign*bias;states=oracle.propagate(initial,np.linspace(-28800,28800,241))
            energy=.5*np.sum(states[:,3:]**2,axis=1)-oracle.MU/np.linalg.norm(states[:,:3],axis=1)
            wanted=.5*np.dot(initial[3:],initial[3:])-oracle.MU/np.linalg.norm(initial[:3])
            self.assertLess(np.max(np.abs(energy-wanted)),1e-9)
            moment=np.cross(states[:,:3],states[:,3:]);self.assertLess(np.max(np.abs(moment-np.cross(initial[:3],initial[3:]))),1e-7)

    def test_every_native_range_residual_and_edit_decision(self):
        self.assertTrue(oracle.validate_reference(self.result))
        self.assertEqual(self.result['edit_flags'],{'':58,'B1':1,'IRMS':299})

    def test_step_size_stability_and_reproducible_reference(self):
        actual=np.array(self.result['native_mask_steps'][1]['state']);expected=np.array(self.fixture['expected_state'])
        self.assertLess(max(abs(actual[:3]-expected[:3])),5e-6)
        self.assertLess(max(abs(actual[3:]-expected[3:])),5e-9)
        truth=np.array(self.result['truth'])
        self.assertGreater(max(abs(actual[:3]-truth[:3])),.2)

    def test_wrong_range_or_selection_is_rejected(self):
        for key,value in [('truth_observation_max_error_km',1.),('native_computed_max_error_km',1.),('native_residual_max_error_km',1.),('predicted_acceptance_matches_native',False),('accepted_records',57)]:
            changed=copy.deepcopy(self.result);changed[key]=value
            with self.assertRaises(AssertionError):oracle.validate_reference(changed)

    def test_misaligned_geometry_epoch_is_rejected(self):
        with tempfile.TemporaryDirectory()as folder:
            target=Path(folder)/'stations.csv';lines=(DATA/'station-inputs.csv').read_text().splitlines();parts=lines[1].split(',');parts[0]='26000';lines[1]=','.join(parts);target.write_text('\n'.join(lines)+'\n')
            with self.assertRaises(AssertionError):oracle.load_inputs(DATA/'observations.gmd',target,DATA/'native-report.txt')

if __name__=='__main__':unittest.main()
