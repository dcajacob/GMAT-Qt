"""Exercise admission failures using one real orbit fixture and isolated records."""
import copy
import json
from pathlib import Path
import unittest
from AuditSaveOrbitEvidence import independent_conic, validate_cartesian


class CartesianEvidenceContracts(unittest.TestCase):
    def setUp(self):
        matrix=json.loads(Path(__file__).with_name('NasaSaveOrbitCases.json').read_text())
        self.case=copy.deepcopy(matrix['cases'][0]);w=self.case['cartesian_witnesses'][0]
        self.records=[dict(object='Subject',coordinate_system='EarthMJ2000Eq',actual=independent_conic(self.case),
                           expected=w['expected'],position_bound_km=w['position_bound_km'],velocity_bound_km_s=w['velocity_bound_km_s'])]

    def rejected(self, change):
        change()
        with self.assertRaises(ValueError):validate_cartesian(self.case,self.records,'isolated-contract')

    def test_valid_original_fixture(self):validate_cartesian(self.case,self.records,'original')
    def test_changed_position(self):self.rejected(lambda:self.records[0]['actual'].__setitem__(0,self.records[0]['actual'][0]+.01))
    def test_changed_velocity(self):self.rejected(lambda:self.records[0]['actual'].__setitem__(3,self.records[0]['actual'][3]+1e-5))
    def test_missing_capture(self):self.rejected(lambda:self.records.clear())
    def test_wrong_frame(self):self.rejected(lambda:self.records[0].update(coordinate_system='EarthFixed'))
    def test_widened_tolerance(self):self.rejected(lambda:self.records[0].update(position_bound_km=.1))
    def test_nonfinite_value(self):self.rejected(lambda:self.records[0]['actual'].__setitem__(0,float('nan')))
    def test_bool_value(self):self.rejected(lambda:self.records[0]['actual'].__setitem__(0,True))
    def test_wrong_object(self):self.rejected(lambda:self.records[0].update(object='Other'))
    def test_short_state(self):self.rejected(lambda:self.records[0]['actual'].pop())
    def test_wrong_conic_oracle(self):self.rejected(lambda:self.case['canonical_keplerian'].__setitem__(0,10000))


if __name__=='__main__':unittest.main()
