"""Source-equivalence guard tests; synthetic strings never earn NASA credit."""
import unittest

from AuditVisualRecovery import applicable_source_equal, FAMILIES


class VisualSourceContracts(unittest.TestCase):
    def fixtures(self):
        picker='d->selectFile(asset);selected=QMetaObject::invokeMethod(d,"accept",Qt::DirectConnection);'
        common='void witness(){acceptanceView(*d);'+picker+'require(rotation[0]==25);}'
        repair='if(key=="Load3DS"){QInputMethodEvent input;click(QDialogButtonBox::Open);}else{'+picker+'}'
        diagnostic='save("picker-result.json",QJsonDocument(QJsonObject{{"expected",asset},{"actual",field("ModelFile")->text()},{"accepted",selected}}).toJson());'
        later='#include <QInputMethodEvent>\n'+common.replace(picker,repair+diagnostic)
        return common,later

    def test_only_six_non_load_families_keep_identical_executed_code(self):
        earlier,later=self.fixtures()
        for family in FAMILIES:
            with self.subTest(family=family):
                self.assertTrue(applicable_source_equal(earlier,later,family)['applicable_source_byte_equal'])

    def test_load_branch_and_unknown_family_cannot_borrow_the_equivalence(self):
        earlier,later=self.fixtures()
        for family in ('Load3DS','Unknown'):
            with self.subTest(family=family),self.assertRaises(ValueError):
                applicable_source_equal(earlier,later,family)

    def test_modified_common_assertion_is_rejected(self):
        earlier,later=self.fixtures()
        for changed in (later.replace('rotation[0]==25','rotation[0]==0'),
                        later.replace('require(rotation[0]==25);',''),
                        later+'extra_transform();'):
            with self.assertRaisesRegex(ValueError,'assertion changed'):
                applicable_source_equal(earlier,changed,'Compose')

    def test_different_picker_repair_cannot_be_normalized(self):
        earlier,later=self.fixtures()
        for changed in (later.replace('QInputMethodEvent input','OtherInput input'),
                        later.replace('QDialogButtonBox::Open','QDialogButtonBox::Cancel'),
                        later.replace('#include <QInputMethodEvent>\n','')):
            with self.assertRaises(ValueError):
                applicable_source_equal(earlier,changed,'Translate')


if __name__=='__main__':unittest.main()
