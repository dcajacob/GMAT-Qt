"""Inactive Resource-branch source checks; synthetic tests earn no row credit."""
import unittest
from AuditGraphicsRecovery import viewer_source_equal


class GraphicsSourceContracts(unittest.TestCase):
    def test_viewer_can_reuse_only_an_unexecuted_resource_branch_change(self):
        left='if(mode=="Resource"){QString x="{literal}";require(oldResource());}else{require(viewerCorrect());}'
        right='if(mode=="Resource"){if(nested){require(newResource());}}else{require(viewerCorrect());}'
        self.assertTrue(viewer_source_equal(left,right,'Viewer'))

    def test_changed_viewer_assertion_is_rejected(self):
        left='if(mode=="Resource"){oldResource();}else{require(viewerCorrect());}'
        right='if(mode=="Resource"){newResource();}else{require(viewerWrong());}'
        with self.assertRaisesRegex(ValueError,'Viewer code or assertion changed'):
            viewer_source_equal(left,right,'Viewer')

    def test_resource_and_unknown_arguments_cannot_borrow_viewer_equivalence(self):
        source='if(mode=="Resource"){resource();}else{viewer();}'
        for mode in ('Resource','Unknown'):
            with self.subTest(mode=mode),self.assertRaises(ValueError):
                viewer_source_equal(source,source,mode)


if __name__=='__main__':unittest.main()
