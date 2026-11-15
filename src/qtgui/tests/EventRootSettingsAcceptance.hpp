#pragma once
#include <memory>
// Included after the unchanged result client's widget helpers. These additions
// configure and observe Brent; all original fixture/oracle assertions remain.
namespace eventrootaccept {
struct Witness {
   QString spelling=qEnvironmentVariable("GMAT_EVENT_ROOT_TOLERANCE","1e-6");
   double tolerance=0;
   int drafts=0,invalids=0;
   Witness(){bool ok=false;tolerance=spelling.toDouble(&ok);require(ok&&(tolerance==1e-4||tolerance==1e-6),"Supply declared Brent tolerance profile");}
   template<class Choice,class Pristine>
   void configure(ResourceEditor *panel,QDialog *form,bool cancel,Choice combo,Pristine pristine){
      auto *value=form->findChild<QLineEdit *>("event_RootTolerance");auto *method=form->findChild<QComboBox *>("event_RootFindingMethod");
      require(value&&method&&method->currentText()=="SPICE"&&!value->isEnabled(),"Initial SPICE/root tolerance dependency differs");
      // The visible platform can still be closing its native popup after the
      // previous keyboard selection. Settle events before reusing that popup;
      // retain the original mouse-opening and visible-popup assertions.
      auto selectRoot=[&](const QString &name){QTest::qWait(QApplication::doubleClickInterval()+20);combo("RootFindingMethod",name);};
      selectRoot("Brent");require(value->isEnabled(),"Brent did not enable tolerance");
      reveal(value);value->setFocus();QTest::keyClick(value,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(value,spelling);QTest::keyClick(value,Qt::Key_Tab);events();
      require(value->text()==spelling,"Typed root tolerance changed");selectRoot("SPICE");require(!value->isEnabled()&&value->text()==spelling,"SPICE switch lost tolerance draft");
      selectRoot("Brent");require(value->isEnabled()&&value->text()==spelling,"Brent switch lost tolerance draft");++drafts;
      if(!cancel){
         auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");
         for(const auto &bad:QStringList{"0","-1","nan"}){
            value->setText(bad);events();click(buttons,QDialogButtonBox::Apply);
            auto *status=panel->findChild<QLabel *>("resourceStatus");
            require(pristine()&&panel->hasChanges()&&status&&!status->text().isEmpty(),"Invalid Brent tolerance changed source/native/history or lost diagnostic");++invalids;
         }
         value->setText(spelling);events();
      }
   }
   QJsonObject observe()const{
      auto *configured=Moderator::Instance()->GetConfiguredObject("Events");auto *runtime=Moderator::Instance()->GetInternalObject("Events");
      require(configured&&runtime,"Configured/runtime event locator absent");
      for(auto *object:{configured,runtime})require(object->GetStringParameter("RootFindingMethod")=="Brent"&&object->GetRealParameter("RootTolerance")==tolerance,"Brent method/tolerance lost across persistence or runtime clone");
      std::unique_ptr<GmatBase> copy(configured->Clone());require(copy->GetStringParameter("RootFindingMethod")=="Brent"&&copy->GetRealParameter("RootTolerance")==tolerance,"Brent clone lost method/tolerance");
      require(drafts==2&&invalids==3,"Brent draft/invalid setting partition incomplete");
      return {{"status","observed"},{"method","Brent"},{"tolerance_seconds",tolerance},{"draft_round_trips",drafts},{"invalid_atomic_rejections",invalids},
         {"configured_runtime_clone_and_persistence",true},{"original_oracle_assertions_changed",0},{"new_requirement_credit",0}};
   }
};
}
