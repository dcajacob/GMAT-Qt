import unittest
from DocumentationTextEdits import apply_edits
class DocumentationEditTests(unittest.TestCase):
 def test_original_offsets_survive_unordered_length_changes(self):
  source='alpha=old; beta=old; gamma=keep';self.assertEqual(apply_edits(source,[(16,19,'much longer'),(6,9,'X')]),'alpha=X; beta=much longer; gamma=keep')
 def test_surrounding_unicode_bytes_preserved(self):
  self.assertEqual(apply_edits('µ one θ two Ω',[(8,11,'second'),(2,5,'first')]),'µ first θ second Ω')
 def test_overlapping_and_duplicate_edits_rejected(self):
  for edits in [[(0,3,'a'),(2,4,'b')],[(0,2,'a'),(0,2,'b')]]:
   with self.assertRaises(ValueError):apply_edits('abcdef',edits)
 def test_out_of_bounds_rejected(self):
  for span in [(-1,1),(1,8),(3,2)]:
   with self.assertRaises(ValueError):apply_edits('abc',[(*span,'x')])
 def test_empty_and_adjacent_edits(self):
  self.assertEqual(apply_edits('abcdef',[]),'abcdef');self.assertEqual(apply_edits('abcdef',[(0,3,'A'),(3,6,'B')]),'AB')
if __name__=='__main__':unittest.main()
