// Supplementary witness for exact Formation copies and native sharing guards.
#include "FormationAcceptance.hpp"
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <functional>
using namespace formaccept;

int main(int argc,char **argv) {
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("FormationCloneScope");
   try {
      require(argc==3,"Supply startup and sharing policy");
      const bool shared=QString(argv[2])=="Shared";
      require(shared||QString(argv[2])=="Exclusive","Unknown sharing policy");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QString name="Twin",boundary="BeginMissionSequence;\n";
      QString initial=rig.script({"One"},true);
      // Pin per-instance identifiers across configured-model rebuilds.
      for(int i=0;i<4;++i) {
         const auto &body=orbits[i].name;
         initial.replace(body+".CoordinateSystem =",body+".NAIFId = "+QString::number(-20001-i)+";\n"+
            body+".NAIFIdReferenceFrame = "+QString::number(-9002001-i)+";\n"+body+".CoordinateSystem =");
      }
      initial.replace(boundary,"Fleet.AllowSharedMembers = "+QString(shared?"true":"false")+";\n"+boundary);
      const auto path=rig.files.filePath("Formation clone Ω.script");write(path,initial.toUtf8());
      require(rig.window.loadScript(path)&&rig.window.buildScript(),"Formation clone initial Build failed");rig.save("initial.script",initial.toUtf8());
      auto object=[&](const QString &n) {return Moderator::Instance()->GetConfiguredObject(n.toStdString());};
      auto snapshot=[&](const QString &n) {auto *p=object(n);require(p,"Formation clone object absent");return QString::fromStdString(p->GetGeneratingString(Gmat::SHOW_SCRIPT));};
      const auto original=snapshot("Fleet");const auto originalMember=snapshot("One");
      int stableCheck=0;
      auto stable=[&] {
         const auto current=snapshot("Fleet"),member=snapshot("One");
         const auto prefix="preservation-"+QString::number(stableCheck++);
         rig.save(prefix+"-formation-before.txt",original.toUtf8());rig.save(prefix+"-formation-after.txt",current.toUtf8());
         rig.save(prefix+"-member-before.txt",originalMember.toUtf8());rig.save(prefix+"-member-after.txt",member.toUtf8());
         require(current==original&&member==originalMember,"Cloning changed the original Formation or spacecraft");
      };
      auto select=[&] {
         rig.closePanels();
         for(auto *tabs:rig.window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)
            if(tabs->widget(i)==rig.resources||tabs->widget(i)->isAncestorOf(rig.resources))tabs->setCurrentIndex(i);
         const auto rows=rig.resources->findItems("Fleet",Qt::MatchExactly|Qt::MatchRecursive);
         require(rows.size()==1,"Formation clone tree entry absent");rig.resources->expandAll();rig.resources->setCurrentItem(rows[0]);rig.resources->scrollToItem(rows[0]);events();acceptanceView(*rig.resources);return rows[0];
      };
      auto dialog=[&](bool accept) {
         const auto before=rig.source->toPlainText();const int undo=rig.source->document()->availableUndoSteps();
         auto *item=select();bool visited=false,chosen=false;std::exception_ptr failure;
         QTimer modal;modal.setInterval(10);
         QObject::connect(&modal,&QTimer::timeout,&rig.window,[&] {
            auto *d=qobject_cast<QDialog *>(QApplication::activeModalWidget());if(!d)return;modal.stop();
            try {
               require(d->objectName()=="cloneResourceDialog","Wrong clone modal");acceptanceView(*d);
               auto *input=d->findChild<QLineEdit *>("resourceName");auto *buttons=d->findChild<QDialogButtonBox *>("resourceCloneButtons");
               require(input&&buttons,"Formation clone name/actions missing");input->setText(name);
               if(accept) {
                  QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);events();
                  if(shared)require(d->result()==QDialog::Accepted,"Shared exact Formation clone rejected");
                  else {
                     auto *error=d->findChild<QLabel *>("resourceCloneStatus");
                     require(d->isVisible()&&error&&error->text().contains("already belongs to formation"),"Exclusive sharing refusal missing");
                     require(!object(name)&&rig.source->toPlainText()==before&&rig.source->document()->availableUndoSteps()==undo,"Rejected clone partially changed source/model/history");
                     rig.save("exclusive-refusal.txt",error->text().toUtf8());QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
                  }
               } else QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
               chosen=true;
            }catch(...){failure=std::current_exception();d->reject();}
         });
         QTimer::singleShot(0,&rig.window,[&] {
            auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
            try {
               require(menu,"Formation clone context menu missing");QAction *clone=nullptr;
               for(auto *a:menu->actions())if(a->objectName()=="cloneResource")clone=a;
               require(clone&&clone->isEnabled(),"Formation clone action unavailable");acceptanceView(*menu);visited=true;
               menu->setActiveAction(clone);modal.start();QTest::keyClick(menu,Qt::Key_Return);
            }catch(...){failure=std::current_exception();if(menu)menu->close();}
         });
         rig.resources->customContextMenuRequested(rig.resources->visualItemRect(item).center());events();modal.stop();
         if(failure)std::rethrow_exception(failure);require(visited&&chosen,"Formation context/modal not exercised");
         if(!accept||!shared)require(rig.source->toPlainText()==before&&rig.source->document()->availableUndoSteps()==undo&&!object(name),"Cancelled/rejected Formation clone changed state");
         stable();
      };
      dialog(false);dialog(true);QJsonArray runs;
      if(!shared) {
         const auto failure=rig.window.cloneResource("Fleet","ApiTwin",initial,{{"AllowSharedMembers","true"}});
         require(failure.contains("already belongs to formation")&&!object("ApiTwin")&&rig.source->toPlainText()==initial,"One-sided sharing permission bypassed guard");stable();
      } else {
         require(object(name)&&object(name)!=object("Fleet")&&members(object(name))==QStringList{"One"}&&
            object(name)->GetBooleanParameter("AllowSharedMembers"),"Exact Formation member/flag copy differs");
         const auto cloned=rig.source->toPlainText();
         const auto expected=QString(initial).replace(boundary,"Create Formation Twin;\nTwin.Add = {One};\nTwin.AllowSharedMembers = true;\n"+boundary);
         rig.save("expected-cloned.script",expected.toUtf8());rig.save("cloned.script",cloned.toUtf8());
         require(cloned==expected,"Clone changed unrelated source or omitted exact member configuration");
         rig.source->undo();require(rig.source->toPlainText()==initial&&rig.window.buildScript()&&!object(name),"Formation clone Undo failed");
         rig.source->redo();require(rig.source->toPlainText()==cloned&&rig.window.buildScript()&&object(name),"Formation clone Redo failed");stable();
         require(rig.window.saveScriptTo(path)&&rig.window.loadScript(path)&&rig.window.buildScript()&&rig.source->toPlainText()==cloned,"Formation clone Unicode reopen failed");stable();
         const auto rejected=rig.window.cloneResource("Fleet","NoConsent",cloned,{{"AllowSharedMembers","false"}});
         require(rejected.contains("already belongs to formation")&&!object("NoConsent")&&rig.source->toPlainText()==cloned,"Copied Formation bypassed its sharing flag");stable();
         auto run=[&](const QString &phase,const QString &active) {
            const auto source=rig.source->toPlainText();const int undo=rig.source->document()->availableUndoSteps();
            require(rig.window.runMission()==MainWindow::RunResult::Completed,"Cloned Formation propagation failed");events();
            const auto bytes=read(rig.report);rig.save(phase+".csv",bytes);rig.save(phase+".script",source.toUtf8());
            const auto cells=QString::fromUtf8(bytes).trimmed().split(',');require(cells.size()==33,"Formation clone report shape changed");
            QJsonArray raw,expected;
            for(const auto &cell:cells){bool ok=false;double value=cell.toDouble(&ok);require(ok&&std::isfinite(value),"Formation clone result is nonfinite");raw.append(value);}
            for(int i=0;i<4;++i) {
               const auto &orbit=orbits[i];const double seconds=orbit.name==active?duration:0;const auto truth=circular(orbit,seconds);
               expected.append(epoch+seconds/86400);expected.append(seconds);
               require(std::abs(raw[i*8].toDouble()-(epoch+seconds/86400))<1e-10&&std::abs(raw[i*8+1].toDouble()-seconds)<.00003,"Clone propagation changed a member/nonmember clock");
               auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject(orbit.name.toStdString()));require(sat,"Cloned member native object absent");
               for(int j=0;j<6;++j) {
                  expected.append(truth[j]);require(std::abs(raw[i*8+2+j].toDouble()-truth[j])<(j<3?.00002:.00000003),"Cloned Formation differs from independent circle");
                  require(std::abs(sat->GetState()[j]-raw[i*8+2+j].toDouble())<1e-9,"Cloned Formation report/native state differ");
               }
            }
            expected.append(41);require(raw.last().toDouble()==41,"Clone propagation changed unrelated sentinel");
            require(rig.source->toPlainText()==source&&rig.source->document()->availableUndoSteps()==undo,"Clone Run changed source/history");stable();
            runs.append(QJsonObject{{"phase",phase},{"active",active},{"raw",raw},{"independent_expected",expected}});
         };
         rig.source->setPlainText(QString(cloned).replace("P(Fleet)","P(Twin)"));require(rig.window.buildScript(),"Cloned Formation consumer did not build");run("shared-copy","One");
         const auto source=rig.source->toPlainText();
         const auto error=rig.window.applyResourceChanges("Twin",{{"Add","Two"}},source);
         rig.save("member-edit-error.txt",error.toUtf8());
         require(error.isEmpty()&&members(object(name))==QStringList{"Two"}&&members(object("Fleet"))==QStringList{"One"},"Cloned Formation independent member edit failed");stable();
         rig.source->setPlainText(QString(rig.source->toPlainText()).replace("{One.ElapsedSecs = 60}","{Two.ElapsedSecs = 60}"));
         require(rig.window.buildScript(),"Edited Formation consumer did not build");
         run("independent-copy","Two");
         require(rig.window.saveScriptTo(path)&&rig.window.loadScript(path)&&rig.window.buildScript()&&members(object(name))==QStringList{"Two"},"Edited Formation copy did not reopen");stable();
      }
      const QJsonObject proof{{"status","passed"},{"mode",argv[2]},{"sharing_allowed",shared},{"original_preserved",true},
         {"cancel_and_rejection_atomic",true},{"runs",runs},{"native_platform",QApplication::platformName()},{"new_requirement_credit",0},
         {"scope","Exact Formation copy under native pairwise-sharing policy; explicit rejection without consent. No blanket resource-cloning or physics qualification."}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';}return 1;
}
