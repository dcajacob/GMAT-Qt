#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMap>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextCursor>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Epoch clone artifact unavailable"); return file.readAll(); }
static void write(const QString &path,const QByteArray &value) { QFile file(path); require(file.open(QIODevice::WriteOnly)&&file.write(value)==value.size(),"Epoch clone artifact write failed"); }
static void reveal(QWidget *widget) {
   require(widget,"Epoch clone input unavailable");
   for (auto *parent=widget;parent;parent=parent->parentWidget()) {
      for (auto *tabs:widget->window()->findChildren<QTabWidget *>()) if (tabs->indexOf(parent)>=0) tabs->setCurrentIndex(tabs->indexOf(parent));
      if (auto *scroll=qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(widget);
   }
   events(); require(widget->isVisible(),"Epoch clone input hidden");
}

int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("NasaEpochClone");
   try {
      require(argc==5&&acceptanceWidgetPlatform(),"Supply startup, original/canonical epoch cases and clone mode");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Epoch clone fixture unavailable");
      const QString mode=argv[4],startup=QFileInfo(argv[1]).absoluteFilePath(); require(mode=="Modify"||mode=="ChangeAll","Unknown epoch clone mode");
      QMap<QString,QJsonObject> specs;
      for (int i=2;i<4;++i) for (const auto &entry:QJsonDocument::fromJson(read(QFileInfo(argv[i]).absoluteFilePath())).object()["cases"].toArray()) specs[entry.toObject()["key"].toString()]=entry.toObject();
      require(specs.contains("TAIModJulian")&&specs.contains("UTCGregorian")&&specs.contains("TDBGregorian")&&specs.contains("TTGregorian"),"Reused independent epoch cases missing");
      const auto original=specs[mode=="Modify"?"TAIModJulian":"UTCGregorian"],cloneEdit=specs["TDBGregorian"],originalEdit=specs["TTGregorian"];
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"); auto save=[&](const QString &name,const QByteArray &value) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),value); };
      const auto report=files.filePath("clone epochs.csv");
      const QString baseline="% NASA epoch clone Ω\nCreate Spacecraft Sat;\nSat.DateFormat = TAIModJulian;\nSat.Epoch = 21545;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DryMass = 913.25;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.5;\nSat.VZ = 1;\nCreate Variable Keep;\nKeep = 7; % untouched initializer\nCreate ReportFile EpochCheck;\nEpochCheck.Filename = '"+report+"';\nEpochCheck.WriteHeaders = false;\nEpochCheck.Precision = 17;\nEpochCheck.FixedWidth = false;\nEpochCheck.Delimiter = ',';\nBeginMissionSequence;\nKeep = Keep + 2; % untouched mission\n";
      save("baseline.script",baseline.toUtf8()); QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window;
      require(window.initialize(startup),"Epoch clone initialization failed"); window.show(); acceptanceView(window); events();
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace");
      require(source&&tree&&area,"Epoch clone workspace unavailable"); source->setPlainText(baseline); require(window.buildScript(),"Epoch clone baseline rejected");
      auto object=[](const QString &name) { return Moderator::Instance()->GetConfiguredObject(name.toStdString()); };
      auto snapshot=[&](const QString &name) {
         auto *sat=dynamic_cast<Spacecraft *>(object(name)); require(sat,"Epoch clone spacecraft unavailable"); require(sat->GetRealParameter("DryMass")==913.25 && sat->GetStringParameter("CoordinateSystem")=="EarthMJ2000Eq","Epoch clone changed unrelated mass/frame"); const auto state=sat->GetState("Cartesian"); QJsonArray values;
         const double wanted[6]={7000,0,0,0,7.5,1}; for (int i=0;i<6;++i) { require(std::abs(state[i]-wanted[i])<1e-12,"Epoch clone operation changed unrelated Cartesian state"); values.append(state[i]); }
         return QJsonObject{{"name",name},{"format",QString::fromStdString(sat->GetStringParameter("DateFormat"))},{"epoch_text",QString::fromStdString(sat->GetStringParameter("Epoch"))},{"a1_mjd",sat->GetRealParameter("A1Epoch")},{"state",values}};
      };
      QJsonArray native,missions;
      auto check=[&](const QString &phase,const QString &name,const QJsonObject &spec) {
         auto state=snapshot(name); const double error=std::abs(state["a1_mjd"].toDouble()-spec["expected"].toObject()["A1"].toDouble())*86400.;
         require(state["format"]==spec["key"]&&error<=spec["epoch_tolerance_seconds"].toDouble(),"Epoch clone format/native time differs from independent oracle");
         state["phase"]=phase; state["expected"]=spec["expected"]; state["tolerance_seconds"]=spec["epoch_tolerance_seconds"]; native.append(state); save("native-states.json",QJsonDocument(native).toJson());
      };
      auto item=[&](const QString &name) { const auto found=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(found.size()==1,"Epoch clone resource tree identity missing/ambiguous"); return found.front(); };
      auto open=[&](const QString &name) {
         auto *entry=item(name); tree->scrollToItem(entry); tree->setCurrentItem(entry); acceptanceView(window); tree->itemDoubleClicked(entry,0); events();
         auto *child=area->activeSubWindow(); require(child&&dynamic_cast<ResourceEditor *>(child->widget())&&child->windowTitle().contains(name),"Epoch clone actual resource editor unavailable");
         child->showNormal(); child->move(0,0); child->resize(area->viewport()->size()); acceptanceView(*child); return child;
      };
      auto outside=[&](QString text,const QString &name) {
         text.remove(QRegularExpression("^"+QRegularExpression::escape(name)+"\\.[A-Za-z][A-Za-z0-9_.]* = [^\\n]*\\n",QRegularExpression::MultilineOption)); return text;
      };
      auto edit=[&](const QString &name,const QJsonObject &spec) {
         const auto before=source->toPlainText(); const int undo=source->document()->availableUndoSteps(); const auto state=snapshot(name);
         QPointer<QMdiSubWindow> child=open(name); auto *panel=dynamic_cast<ResourceEditor *>(child->widget());
         auto *format=panel->findChild<QComboBox *>("resource_DateFormat"); auto *epoch=panel->findChild<QLineEdit *>("resource_Epoch"); auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons"); require(format&&epoch&&buttons,"Epoch clone primary fields unavailable");
         reveal(format); format->setCurrentText(spec["key"].toString()); events(); reveal(epoch); epoch->setFocus(); epoch->selectAll(); QTest::keyClicks(epoch,spec["literal"].toString()); events();
         require(format->currentText()==spec["key"].toString()&&epoch->text()==spec["literal"].toString()&&panel->hasChanges()&&source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&snapshot(name)==state,"Epoch clone draft changed native/source/history or lost input");
         acceptanceView(*child); auto *apply=buttons->button(QDialogButtonBox::Apply); reveal(apply); QTest::mouseClick(apply,Qt::LeftButton); events();
         require(child&&source->toPlainText()!=before&&outside(source->toPlainText(),name)==outside(before,name),"Epoch clone Apply changed another object's or mission's source");
         require(!dynamic_cast<ResourceEditor *>(child->widget())->hasChanges()&&child->close(),"Epoch clone Apply did not leave a clean closeable owner"); events();
      };
      // Exercise the actual tree menu and modal buttons, retaining the Qt-event
      // versus native OS-input distinction used by the existing lifecycle suite.
      auto context=[&](const QString &name,const QString &action,const std::function<void(QDialog *)> &choose) {
         auto *entry=item(name); tree->expandAll(); tree->scrollToItem(entry); tree->setCurrentItem(entry); events(); acceptanceView(window);
         int stage=0; bool visited=false; std::exception_ptr failure; QTimer timer; timer.setInterval(20);
         QObject::connect(&timer,&QTimer::timeout,&window,[&] {
            try {
               if (stage==0) { auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); if (!menu) return;
                  QAction *selected=nullptr; for (auto *candidate:menu->actions()) if (candidate->objectName()==action||candidate->text()==action) selected=candidate;
                  require(selected&&selected->isEnabled(),"Epoch clone context action unavailable"); stage=1; menu->setActiveAction(selected); acceptanceView(*menu); QTest::keyClick(menu,Qt::Key_Return);
               } else if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) { timer.stop(); acceptanceView(*dialog); visited=true; choose(dialog); }
            } catch (...) { timer.stop(); failure=std::current_exception(); if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) dialog->reject(); if (auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget())) menu->close(); }
         });
         timer.start(); tree->customContextMenuRequested(tree->visualItemRect(entry).center()); timer.stop(); events(); if (failure) std::rethrow_exception(failure); require(visited,"Epoch clone context/modal path not exercised");
      };
      auto clone=[&](bool accept) { context("Sat","cloneResource",[&](QDialog *dialog) {
         require(dialog->objectName()=="cloneResourceDialog","Unexpected epoch clone dialog"); auto *name=dialog->findChild<QLineEdit *>("resourceName"); auto *buttons=dialog->findChild<QDialogButtonBox *>("resourceCloneButtons"); require(name&&buttons,"Epoch clone name/actions unavailable");
         name->setFocus(); name->selectAll(); QTest::keyClicks(name,"Twin"); require(name->text()=="Twin","Epoch clone name input differs"); acceptanceView(*dialog);
         auto *button=buttons->button(accept?QDialogButtonBox::Ok:QDialogButtonBox::Cancel); require(button&&button->isVisible()&&button->isEnabled(),"Epoch clone modal action hidden/disabled"); QTest::mouseClick(button,Qt::LeftButton); events(); require(dialog->result()==(accept?QDialog::Accepted:QDialog::Rejected),"Epoch clone modal outcome differs");
      }); };
      auto mission=[&](const QString &phase,const QMap<QString,QJsonObject> &expected) {
         const auto authored=source->toPlainText(); const int undo=source->document()->availableUndoSteps(); QString commands;
         for (auto it=expected.cbegin();it!=expected.cend();++it) commands+="Report EpochCheck "+it.key()+".A1ModJulian "+it.key()+".TAIModJulian "+it.key()+".UTCModJulian "+it.key()+".TTModJulian "+it.key()+".TDBModJulian Keep;\n";
         auto cursor=source->textCursor(); cursor.movePosition(QTextCursor::End); cursor.beginEditBlock(); cursor.insertText(commands); cursor.endEditBlock();
         save(phase+".authored.script",authored.toUtf8()); save(phase+".report.script",source->toPlainText().toUtf8());
         require(window.buildScript(),"Epoch clone reporting mission rejected"); if (QFile::exists(report)) require(QFile::remove(report),"Epoch clone stale report removal failed");
         require(window.runMission()==MainWindow::RunResult::Completed,"Epoch clone numerical mission failed"); events(); const auto bytes=read(report); save(phase+".csv",bytes);
         const auto rows=QString::fromUtf8(bytes).trimmed().split('\n'); require(rows.size()==expected.size(),"Epoch clone report row count differs"); int index=0; QJsonArray checked;
         for (auto it=expected.cbegin();it!=expected.cend();++it,++index) {
            const auto values=rows[index].split(','); require(values.size()==6&&values[5].trimmed().toDouble()==9,"Epoch clone report sentinel/width differs"); const QStringList systems{"A1","TAI","UTC","TT","TDB"}; QJsonArray numbers;
            for (int i=0;i<5;++i) { bool ok=false; const double value=values[i].trimmed().toDouble(&ok),limit=(it.value()["system"].toString()=="TDB"||systems[i]=="TDB")?.00005:.000002;
               require(ok&&std::isfinite(value)&&std::abs(value-it.value()["expected"].toObject()[systems[i]].toDouble())*86400.<=limit,"Epoch clone mission differs from independent five-system truth"); numbers.append(value);
            }
            checked.append(QJsonObject{{"name",it.key()},{"case",it.value()["key"]},{"values",numbers},{"sentinel",9}});
         }
         source->undo(); require(source->toPlainText()==authored&&source->document()->availableUndoSteps()==undo&&window.buildScript(),"Epoch clone temporary reporting did not restore exact source/history/model"); events();
         for (auto it=expected.cbegin();it!=expected.cend();++it) check(phase,it.key(),it.value());
         missions.append(QJsonObject{{"phase",phase},{"rows",checked}}); save("missions.json",QJsonDocument(missions).toJson());
      };
      edit("Sat",original); check("modified-original","Sat",original); const auto modified=source->toPlainText();
      clone(false); require(source->toPlainText()==modified&&!object("Twin"),"Cancelled epoch clone changed source or registered a copy"); check("clone-cancel","Sat",original);
      clone(true); require(object("Sat")&&object("Twin")&&object("Sat")!=object("Twin"),"Epoch clone objects absent or aliased"); const auto cloned=source->toPlainText();
      check("cloned","Sat",original); check("cloned","Twin",original); require(snapshot("Sat")["epoch_text"]==snapshot("Twin")["epoch_text"],"Unedited epoch clone literal differs from original");
      source->undo(); require(source->toPlainText()==modified&&window.buildScript()&&!object("Twin"),"Epoch clone Undo not exact"); check("clone-undo","Sat",original);
      source->redo(); require(source->toPlainText()==cloned&&window.buildScript(),"Epoch clone Redo not exact"); mission("cloned",{{"Sat",original},{"Twin",original}});
      QMap<QString,QJsonObject> finalObjects{{"Sat",original},{"Twin",original}};
      if (mode=="ChangeAll") {
         edit("Twin",cloneEdit); const auto changedClone=source->toPlainText(); check("clone-edited","Sat",original); check("clone-edited","Twin",cloneEdit);
         source->undo(); require(source->toPlainText()==cloned&&window.buildScript(),"Epoch clone independent edit Undo not exact"); check("clone-edit-undo","Sat",original); check("clone-edit-undo","Twin",original);
         source->redo(); require(source->toPlainText()==changedClone&&window.buildScript(),"Epoch clone independent edit Redo not exact"); mission("clone-edited",{{"Sat",original},{"Twin",cloneEdit}});
         source->undo(); require(source->toPlainText()==cloned&&window.buildScript(),"Epoch clone-edit branch did not restore common cloned baseline"); check("clone-branch-reset","Sat",original); check("clone-branch-reset","Twin",original);
         edit("Sat",originalEdit); const auto changedOriginal=source->toPlainText(); check("original-edited","Sat",originalEdit); check("original-edited","Twin",original);
         source->undo(); require(source->toPlainText()==cloned&&window.buildScript(),"Epoch original independent edit Undo not exact"); check("original-edit-undo","Sat",original); check("original-edit-undo","Twin",original);
         source->redo(); require(source->toPlainText()==changedOriginal&&window.buildScript(),"Epoch original independent edit Redo not exact"); mission("original-edited",{{"Sat",originalEdit},{"Twin",original}});
         source->undo(); require(source->toPlainText()==cloned&&window.buildScript(),"Epoch original-edit branch did not restore common cloned baseline"); check("original-branch-reset","Sat",original); check("original-branch-reset","Twin",original);
         auto remove=[&](const QString &name) { const auto before=source->toPlainText(); const int undo=source->document()->availableUndoSteps();
            for (bool accept:{false,true}) { context(name,"Delete resource…",[&](QDialog *dialog) { auto *box=qobject_cast<QMessageBox *>(dialog); require(box,"Epoch delete confirmation unavailable"); auto *button=box->button(accept?QMessageBox::Yes:QMessageBox::Cancel); require(button,"Epoch delete choice unavailable"); QTest::mouseClick(button,Qt::LeftButton); });
               if (!accept) require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&object(name),"Cancelled epoch deletion changed source/model/history");
            }
            require(!object(name)&&tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive).isEmpty()&&!source->toPlainText().contains(QRegularExpression("^"+name+"\\.",QRegularExpression::MultilineOption)),"Epoch delete left native/tree/source references");
         };
         remove("Twin"); mission("clone-deleted",{{"Sat",original}});
         source->undo(); require(source->toPlainText()==cloned&&window.buildScript(),"Epoch clone delete Undo not exact"); check("delete-undo","Twin",original); check("delete-undo","Sat",original);
         remove("Sat"); mission("original-deleted",{{"Twin",original}}); finalObjects={{"Twin",original}};
      }
      const auto accepted=source->toPlainText(),saved=files.filePath("Cloned epoch Ω.script");
      require(window.saveScriptTo(saved)&&read(saved)==accepted.toUtf8()&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==accepted,"Epoch clone Unicode save/reopen changed accepted source");
      for (auto it=finalObjects.cbegin();it!=finalObjects.cend();++it) check("unicode-reopened",it.key(),it.value());
      require((mode=="Modify"&&object("Sat")&&object("Twin"))||(mode=="ChangeAll"&&!object("Sat")&&object("Twin")),"Epoch clone reopen restored wrong surviving objects"); save("accepted.script",accepted.toUtf8());
      QJsonArray facets{"actual-epoch-edit-and-context-clone","independent-native-epochs-and-copy-identity","clone-cancel-and-exact-history-unicode-reopen","five-system-mission-values-and-orbit-sentinel-isolation"};
      facets.append(mode=="Modify"?"epoch-only-modify-and-clone":"both-epoch-fields-change-clone-edit-and-delete-independence");
      const QJsonObject proof{{"checkpoint","epoch-clone-"+mode},{"outcome","passed"},{"family",mode},{"missions",missions},{"native_states",native},{"facets",facets},
         {"scope","Selected2000 epoch cases reused from independent time-system fixtures. Modify edits Epoch only; ChangeAll edits both documented epoch fields DateFormat/Epoch, then copies, independently edits original/clone, deletes each unused object and checks surviving mission truth. Qt context/menu/modal input, exact history and Unicode reopen. No all-spacecraft-property cloning, arbitrary dates/leap seconds/precision, name-domain/button-state, native OS input, GPU/platform/stress or complete current-build qualification."}};
      save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl; }
     catch (const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<std::endl; }
   return 1;
}
