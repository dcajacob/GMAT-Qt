"""Reference restoration must not change any executed acceptance assertion."""
import copy,unittest
import AuditRemainingRecovery as review

class RemainingReferenceContracts(unittest.TestCase):
    def claim(self):return dict(source='src/qtgui/tests/Exact.cpp',source_sha256='a'*64,anchor='actual == 9',expected={'value':9},facets=['exact'],scope='Full original scope',reference_sources={'src/qtgui/MainWindow.cpp':'1'*64})
    def test_production_version_restoration_preserves_every_assertion_field(self):
        original=self.claim();before=copy.deepcopy(original);actual=review.rebind_claim(original,{'src/qtgui/MainWindow.cpp':'2'*64})
        self.assertEqual(original,before)
        self.assertEqual({k:v for k,v in actual.items() if k!='reference_sources'},{k:v for k,v in original.items() if k!='reference_sources'})
    def test_reference_path_guards_cannot_be_dropped_or_added(self):
        for refs in [{},{'src/qtgui/MainWindow.cpp':'2'*64,'src/extra.cpp':'3'*64}]:
            with self.subTest(refs=refs),self.assertRaises(ValueError):review.rebind_claim(self.claim(),refs)
    def test_assertion_helper_data_and_normative_versions_are_not_production(self):
        for path in ['src/qtgui/tests/Helper.cpp','src/qtgui/tests/Expected.json','doc/help/src/Resource.xml','application/data/table.txt']:
            c=self.claim();c['reference_sources']={path:'1'*64}
            with self.subTest(path=path),self.assertRaises(ValueError):review.rebind_claim(c,{path:'2'*64})
    def test_only_unexecuted_report_cases_can_differ(self):
        a={'scope':'retained','cases':[{'key':'SubscriberToggle','expected':[1,3]},{'key':'Other','expected':1}]};b=copy.deepcopy(a);b['cases'][1]['expected']=2
        self.assertTrue(review.selected_report_case_equal(a,b,'SubscriberToggle'))
    def test_changed_selected_results_metadata_duplicates_and_unknown_case_reject(self):
        a={'scope':'retained','cases':[{'key':'SubscriberToggle','expected':[1,3]}]}
        for mode in ['expected','metadata','duplicate','unknown']:
            b=copy.deepcopy(a);key='SubscriberToggle'
            if mode=='expected':b['cases'][0]['expected']=[1,4]
            elif mode=='metadata':b['scope']='changed'
            elif mode=='duplicate':b['cases'].append(copy.deepcopy(b['cases'][0]))
            else:key='Other'
            with self.subTest(mode=mode),self.assertRaises(ValueError):review.selected_report_case_equal(a,b,key)
