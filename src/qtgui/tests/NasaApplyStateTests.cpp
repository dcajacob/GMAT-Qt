#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Apply state artifact unavailable"); return file.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile file(path); require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Apply state artifact write failed"); }
static QJsonObject entry(const QString &path,const QString &key) { for (const auto &value:QJsonDocument::fromJson(read(path)).object()["cases"].toArray()) if (value.toObject()["key"].toString()==key) return value.toObject(); throw std::runtime_error("Apply state reused case missing"); }
static void reveal(QWidget *widget) {
   require(widget,"Apply state control missing");
   for (auto *parent=widget;parent;parent=parent->parentWidget()) {
      for (auto *tabs:widget->window()->findChildren<QTabWidget *>()) if (tabs->indexOf(parent)>=0) tabs->setCurrentWidget(parent);
      if (auto *scroll=qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(widget);
   }
   events(); require(widget->isVisible(),"Apply state control hidden");
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("NasaApplyState");
   try {
      require(argc==5&&acceptanceWidgetPlatform(),"Supply Apply state startup, reused resource/panel cases and key");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Apply state temporary directory missing");
      const QString key=argv[4],startup=QFileInfo(argv[1]).absoluteFilePath();
      const auto spec=entry(QFileInfo(argv[3]).absoluteFilePath(),key),data=entry(QFileInfo(argv[2]).absoluteFilePath(),spec["resource_key"].toString(key));
      const auto type=data["type"].toString(),field=spec["field"].toString(data["field"].toString()),kind=spec["kind"].toString(data["kind"].toString()),base=spec["base"].toString(data["first"].toString()),valid=spec["valid"].toString(data["second"].toString());
      const auto report=files.filePath("apply result.txt"),saved=files.filePath("Apply state Ω.script"),evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      auto save=[&](const QString &name,const QByteArray &value) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),value); };
      auto literal=[&](const QString &value) { return kind=="list"?"{"+value.split(',').join(", ")+"}":kind=="epoch"?"'"+value+"'":value; };
      QString initial="% NASA Apply state Ω\nCreate Spacecraft Anchor Other;\nCreate Variable Keep OtherValue;\nKeep = 7; % preserve initializer\nOtherValue = 17;\n"+spec["before"].toString()+"Create "+spec["script_type"].toString(type)+" NasaPanel;\n"+spec["setup"].toString()+"NasaPanel."+field+" = "+literal(base)+";\n"+spec["after"].toString();
      const QString reportName=key=="ReportFile"?"NasaPanel":"Check";
      if (key!="ReportFile") initial+="Create ReportFile Check;\nCheck.Precision = 17;\n";
      initial+=reportName+".Filename = '"+report+"';\n"+reportName+".WriteHeaders = false;\nBeginMissionSequence;\nKeep = Keep + 2; % preserve mission\nReport "+reportName+" Keep;\n";
      save("initial.script",initial.toUtf8()); QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window;
      require(window.initialize(startup),"Apply state initialization failed"); window.show(); acceptanceView(window); events();
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace"); require(source&&tree&&area,"Apply state workspace missing");
      source->setPlainText(initial); require(window.buildScript(),"Apply state baseline rejected"); source->document()->setModified(false); const int initialUndo=source->document()->availableUndoSteps();
      auto model=[&] { auto *object=Moderator::Instance()->GetConfiguredObject("NasaPanel"); require(object,"Apply state native resource missing");
         if (kind=="real") return QString::number(object->GetRealParameter(field.toStdString()),'g',17);
         if (kind=="integer") return QString::number(object->GetIntegerParameter(field.toStdString()));
         if (kind=="list") { QStringList items; for (const auto &value:object->GetStringArrayParameter(field.toStdString())) items.append(QString::fromStdString(value)); return items.join(','); }
         return QString::fromStdString(object->GetStringParameter(field.toStdString()));
      };
      auto equal=[&](const QString &a,const QString &b) { return kind=="real"||kind=="integer"||kind=="epoch"?a.toDouble()==b.toDouble():a==b; };
      require(equal(model(),base),"Apply state baseline native value differs");
      auto open=[&](const QString &name) { const auto found=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(found.size()==1,"Apply state resource missing/ambiguous"); tree->scrollToItem(found.front()); tree->setCurrentItem(found.front()); acceptanceView(window); tree->itemDoubleClicked(found.front(),0); events(); auto *child=area->activeSubWindow(); require(child&&dynamic_cast<ResourceEditor *>(child->widget()),"Apply state resource editor missing"); child->showNormal(); child->move(0,0); child->resize(area->viewport()->size()); acceptanceView(*child); return child; };
      QPointer<QMdiSubWindow> sibling=open("Keep"),child=open("NasaPanel");
      auto panel=[&] { auto *value=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr; require(value,"Apply state owner missing"); return value; };
      auto button=[&](QDialogButtonBox::StandardButton action) { auto *box=panel()->findChild<QDialogButtonBox *>("resourceButtons"); require(box,"Apply state buttons missing"); auto *value=box->button(action); reveal(value); return value; };
      QJsonArray states;
      auto state=[&](const QString &phase,bool enabled) {
         auto *apply=button(QDialogButtonBox::Apply); const auto input=panel()->findChild<QLineEdit *>(spec["control"].toString()); require(input&&!input->isReadOnly(),"Apply state editable input missing");
         QJsonObject value{{"phase",phase},{"apply_enabled",apply->isEnabled()},{"pending",panel()->hasChanges()},{"input",input->text()},{"model",model()},{"undo_steps",source->document()->availableUndoSteps()},{"source_sha256",QString::fromLatin1(QCryptographicHash::hash(source->toPlainText().toUtf8(),QCryptographicHash::Sha256).toHex())}};
         states.append(value); save("states.json",QJsonDocument(states).toJson()); acceptanceView(*child);
         require(apply->isEnabled()==enabled,"NASA Apply button clean/dirty state differs");
      };
      auto draft=[&](const QString &value) { auto *input=panel()->findChild<QLineEdit *>(spec["control"].toString()); reveal(input); input->setFocus(); input->selectAll(); if (value.isEmpty()) QTest::keyClick(input,Qt::Key_Backspace); else QTest::keyClicks(input,value); events(); if (input->text()!=value) std::cerr<<"Typed actual=["<<input->text().toStdString()<<"] expected=["<<value.toStdString()<<"]"<<std::endl; require(input->text()==value,"Apply state typed value lost"); };
      auto pristine=[&] { require(source->toPlainText()==initial&&source->document()->availableUndoSteps()==initialUndo&&!source->document()->isModified()&&equal(model(),base)&&sibling&&sibling->isVisible(),"Apply state draft changed source/history/model/sibling"); };
      state("initial-clean",false); draft(valid); pristine(); state("valid-draft",true);
      if (key=="SpacecraftEpoch") {
         draft(base); pristine(); state("reverted-draft-latched",true);
         QTest::mouseClick(button(QDialogButtonBox::Apply),Qt::LeftButton); events(); pristine(); state("no-op-apply-clean",false);
         draft(valid); pristine(); state("valid-again",true);
      }
      const auto invalid=spec["invalid"].toArray().first().toString(); draft(invalid); pristine(); state("invalid-draft",true);
      QTest::mouseClick(button(QDialogButtonBox::Apply),Qt::LeftButton); events(); pristine(); state("rejected-apply-dirty",true);
      auto *diagnostic=panel()->findChild<QLabel *>("resourceStatus"); require(diagnostic&&!diagnostic->text().isEmpty(),"Apply state rejection diagnostic missing");
      draft(valid); pristine(); state("corrected-draft",true); QTest::mouseClick(button(QDialogButtonBox::Apply),Qt::LeftButton); events();
      const auto applied=source->toPlainText(); save("applied.script",applied.toUtf8()); require(applied!=initial&&equal(model(),valid)&&sibling&&sibling->isVisible(),"Apply state valid commit lost value or sibling"); state("applied-clean",false);
      const int appliedUndo=source->document()->availableUndoSteps(); QTest::mouseClick(button(QDialogButtonBox::Apply),Qt::LeftButton); events();
      require(source->toPlainText()==applied&&source->document()->availableUndoSteps()==appliedUndo&&equal(model(),valid),"Disabled Apply changed source/history/model"); state("disabled-click-clean",false);
      draft(base); state("post-apply-draft",true); QTest::mouseClick(button(QDialogButtonBox::Cancel),Qt::LeftButton); events();
      require(!child&&source->toPlainText()==applied&&source->document()->availableUndoSteps()==appliedUndo&&equal(model(),valid)&&sibling,"Apply state Cancel changed accepted source/model/history/sibling");
      child=open("NasaPanel"); state("cancel-reopened-clean",false); require(child->close(),"Apply state clean panel close failed"); events();
      source->undo(); require(source->toPlainText()==initial&&window.buildScript()&&equal(model(),base),"Apply state Undo not exact"); events();
      source->redo(); require(source->toPlainText()==applied&&window.buildScript()&&equal(model(),valid),"Apply state Redo not exact"); events();
      require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==applied&&equal(model(),valid),"Apply state Unicode save/reopen differs"); events(); child=open("NasaPanel"); state("unicode-reopened-clean",false); require(child->close(),"Apply state reopened clean close failed"); events();
      require(window.runMission()==MainWindow::RunResult::Completed,"Apply state sentinel mission failed"); const auto bytes=read(report); save("values.txt",bytes); require(QString::fromUtf8(bytes).trimmed().toDouble()==9,"Apply state independent sentinel differs");
      const QJsonObject proof{{"checkpoint","apply-state-"+key},{"outcome","passed"},{"family",key},{"field",field},{"model_readback",model()},{"states",states},{"numeric_sentinel",9},{"facets",QJsonArray{"disabled-clean-enabled-valid-and-invalid-drafts","failed-apply-keeps-correctable-draft-enabled","successful-apply-and-reopen-disable-clean-button","disabled-click-and-cancel-source-model-history-isolation","exact-history-unicode-native-value-and-sentinel-result"}},
         {"scope","Selected reused resource/panel field fixtures and first invalid boundary; primary line-edit driven Apply status. Epoch additionally checks reverted draft latch and no-op reset. No all-input-kind/form/platform qualification, every invalid domain or physical mission effect of each field, name-domain/current NASA corpus/native OS input/GPU/stress or full-current-build qualification."}};
      save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl; }
     catch (const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<std::endl; }
   return 1;
}
