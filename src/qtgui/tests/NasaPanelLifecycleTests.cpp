#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include <QDialogButtonBox>
#include <QEventLoop>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStyleOptionTitleBar>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
using namespace formaccept;

static QJsonObject entry(const QString &path,const QString &key)
{
   for (const auto &v:QJsonDocument::fromJson(read(path)).object()["cases"].toArray())
      if (v.toObject()["key"]==key) return v.toObject();
   throw std::runtime_error("Panel lifecycle reused case absent");
}
static void reveal(QWidget *widget)
{
   require(widget,"Panel lifecycle widget absent");
   for (QWidget *parent=widget;parent;parent=parent->parentWidget()) {
      for (auto *tabs:widget->window()->findChildren<QTabWidget *>())
         if (tabs->indexOf(parent)>=0) tabs->setCurrentWidget(parent);
      if (auto *scroll=qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(widget);
   }
   events();require(widget->isVisible()&&widget->isEnabled(),"Panel lifecycle widget unreachable");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
   app.setOrganizationName("GMATTests");app.setApplicationName("NasaPanelLifecycle");
   try {
      require(argc==6,"Supply startup, resource/panel/lifecycle cases and key");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();const auto spec=entry(QFileInfo(argv[4]).absoluteFilePath(),argv[5]);
      const auto key=spec["key"].toString();const bool actions=spec["actions"].toBool();
      const auto panelSpec=entry(QFileInfo(argv[3]).absoluteFilePath(),key),data=entry(QFileInfo(argv[2]).absoluteFilePath(),panelSpec["resource_key"].toString(key));
      const auto type=data["type"].toString(),field=panelSpec["field"].toString(data["field"].toString()),kind=panelSpec["kind"].toString(data["kind"].toString());
      const auto base=panelSpec["base"].toString(data["first"].toString()),valid=panelSpec["valid"].toString(data["second"].toString());
      require(kind=="real"||kind=="string","Unsupported lifecycle field kind");Rig rig(startup);
      const auto pathA=rig.files.filePath("Panel lifecycle Ω.script"),pathB=rig.files.filePath("Loaded panel Δ.script"),report=rig.files.filePath("panel results.csv");
      auto recipe=[&](const QString &value,int keep,int other) {
         return "% Panel lifecycle witness Ω\nCreate Spacecraft Anchor Other;\nCreate Variable Keep OtherValue;\nKeep = "+QString::number(keep)+";\nOtherValue = "+QString::number(other)+";\n"+panelSpec["before"].toString()+"Create "+type+" NasaPanel;\n"+panelSpec["setup"].toString()+"NasaPanel."+field+" = "+value+";\n"+panelSpec["after"].toString()+"Create ReportFile Check;\nCheck.Filename = '"+report+"';\nCheck.WriteHeaders = false;\nCheck.FixedWidth = false;\nCheck.Delimiter = ',';\nCheck.Precision = 17;\nBeginMissionSequence;\nKeep = Keep + 1;\nOtherValue = OtherValue + 2;\nReport Check Keep OtherValue;\n% Panel lifecycle tail Δ\n";
      };
      const auto initial=recipe(base,41,17),loaded=actions?recipe(spec["loaded"].toString(),73,29):QString();
      write(pathA,initial.toUtf8());if (actions) write(pathB,loaded.toUtf8());
      rig.save("initial.script",initial.toUtf8());if (actions) rig.save("loaded.script",loaded.toUtf8());rig.save("case.json",QJsonDocument(spec).toJson());
      require(rig.window.loadScript(pathA)&&rig.window.buildScript(),"Panel lifecycle initial mission failed");QString wanted=initial,current=base;QJsonArray operations,commits,prompts,runs,rectangles;
      QPointer<QMdiSubWindow> owner,sibling;QPointer<QLineEdit> siblingValue;
      auto target=[&](bool runtime=false) {
         auto *object=runtime?Moderator::Instance()->GetInternalObject("NasaPanel"):Moderator::Instance()->GetConfiguredObject("NasaPanel");
         require(object&&object->GetTypeName()==type.toStdString(),"Panel lifecycle native resource identity/type differs");return object;
      };
      auto value=[&](bool runtime=false) {return kind=="real"?number(target(runtime)->GetRealParameter(field.toStdString())):QString::fromStdString(target(runtime)->GetStringParameter(field.toStdString()));};
      auto equal=[&](const QString &a,const QString &b) {return kind=="real"?std::abs(a.toDouble()-b.toDouble())<1e-12:a==b;};
      auto configured=[&](const QString &v,int keep=41,int other=17) {
         require(equal(value(),v)&&Moderator::Instance()->GetConfiguredObject("Keep")->GetRealParameter("Value")==keep&&Moderator::Instance()->GetConfiguredObject("OtherValue")->GetRealParameter("Value")==other,"Panel lifecycle configured field/sentinels differ");
      };configured(base);
      auto open=[&](const QString &name) {
         for (auto *tabs:rig.window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->tabText(i)=="Resources") tabs->setCurrentIndex(i);
         auto entries=rig.resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(entries.size()==1,"Panel lifecycle resource identity absent/ambiguous");
         rig.resources->expandAll();rig.resources->setCurrentItem(entries[0]);rig.resources->scrollToItem(entries[0]);events();acceptanceView(*rig.resources);
         rig.resources->itemDoubleClicked(entries[0],0);events();auto *child=rig.area->activeSubWindow();require(child&&child->property("resourceName")==name&&dynamic_cast<ResourceEditor *>(child->widget()),"Panel lifecycle opened wrong actual resource editor");
         child->showNormal();child->move(0,0);child->resize(rig.area->viewport()->size());rig.area->setActiveSubWindow(child);events();acceptanceView(*child);return QPointer<QMdiSubWindow>(child);
      };
      auto panel=[&] {auto *p=owner?dynamic_cast<ResourceEditor *>(owner->widget()):nullptr;require(p,"Panel lifecycle target editor absent");return p;};
      auto input=[&] {auto *p=panel()->findChild<QLineEdit *>(panelSpec["control"].toString());require(p&&!p->isReadOnly(),"Panel lifecycle editable field absent");reveal(p);return p;};
      auto siblingOpen=[&] {
         sibling=open("Keep");siblingValue=sibling->findChild<QLineEdit *>("parameterValue");require(siblingValue,"Panel lifecycle sibling value absent");siblingValue->setText("99");require(dynamic_cast<EditablePanel *>(sibling->widget())->hasChanges(),"Panel lifecycle sibling draft absent");
      };
      auto siblingStable=[&] {require(sibling&&siblingValue&&siblingValue->text()=="99"&&dynamic_cast<EditablePanel *>(sibling->widget())->hasChanges(),"Panel lifecycle lost unrelated pending sibling");};
      auto siblingCancel=[&] {auto *button=sibling?sibling->findChild<QPushButton *>("cancelPanel"):nullptr;require(button,"Panel lifecycle sibling cleanup unavailable");QTest::mouseClick(button,Qt::LeftButton);events();require(!sibling,"Panel lifecycle sibling cleanup retained owner");};
      auto draft=[&](const QString &v) {
         auto *fieldInput=input();fieldInput->setFocus();fieldInput->selectAll();QTest::keyClicks(fieldInput,v);events();
         require(fieldInput->text()==v&&panel()->hasChanges()&&rig.source->toPlainText()==wanted,"Panel lifecycle typed draft escaped source or changed text");acceptanceView(*owner);
      };
      auto click=[&](QDialogButtonBox::StandardButton which) {auto *box=panel()->findChild<QDialogButtonBox *>("resourceButtons");auto *button=box?box->button(which):nullptr;reveal(button);require(button->isEnabled(),"Panel lifecycle action disabled");acceptanceView(*owner);QTest::mouseClick(button,Qt::LeftButton);events();};
      QJsonArray historySources{initial},historyValues{base};
      if (actions) {
         siblingOpen();owner=open("NasaPanel");
         for (int phase=0;phase<3;++phase) {
            const auto v=phase==0?valid:phase==1?spec["third"].toString():spec["fourth"].toString();const auto before=wanted;const int undo=rig.source->document()->availableUndoSteps();
            draft(v);configured(current);siblingStable();require(read(pathA)==initial.toUtf8(),"Panel commit saved file before Save");
            QString next=wanted;next.remove(QRegularExpression("(?m)^NasaPanel\\."+QRegularExpression::escape(field)+" = [^\\n]*\\n"));
            require(next.count("BeginMissionSequence;\n")==1,"Panel lifecycle mission delimiter ambiguous");next.replace("BeginMissionSequence;\n","NasaPanel."+field+" = "+v+";\nBeginMissionSequence;\n");
            QPointer<ResourceEditor> old=panel();QPointer<QMdiSubWindow> original=owner;
            if (phase==2) {const auto callback=old->onApplied;old->onApplied=[&,callback] {callback();if (sibling) rig.area->setActiveSubWindow(sibling);};}
            click(phase<2?QDialogButtonBox::Apply:QDialogButtonBox::Ok);
            require(!old&&rig.source->toPlainText()==next&&rig.source->document()->availableUndoSteps()>undo,"Distinct panel commit changed wrong source/history or retained stale editor");
            if (phase<2) require(owner&&owner==original&&!panel()->hasChanges()&&equal(input()->text(),v),"Positive repeated Apply lost clean owner or current field");
            else require(!owner&&!original,"Positive panel OK failed to close exactly its original owner after callback focus change");
            current=v;wanted=next;configured(current);siblingStable();require(read(pathA)==initial.toUtf8(),"Positive panel commit wrote saved source prematurely");
            const auto name=phase==0?QString("apply-first"):phase==1?QString("apply-second"):QString("positive-ok");
            rig.save(name+".script",wanted.toUtf8());commits.append(QJsonObject{{"phase",name},{"input",v},{"native",value()},{"old_editor_retired",true},{"owner_closed",phase==2},{"sibling_preserved",true}});historySources.append(wanted);historyValues.append(current);
         }
         siblingCancel();
         for (int i=2;i>=0;--i) {rig.source->undo();require(rig.source->toPlainText()==historySources[i].toString()&&rig.window.buildScript(),"Panel lifecycle exact multi-step Undo failed");configured(historyValues[i].toString());}
         for (int i=1;i<4;++i) {rig.source->redo();require(rig.source->toPlainText()==historySources[i].toString()&&rig.window.buildScript(),"Panel lifecycle exact multi-step Redo failed");configured(historyValues[i].toString());}
         require(rig.window.saveScriptTo(pathA)&&read(pathA)==wanted.toUtf8()&&rig.window.loadScript(pathA)&&rig.window.buildScript()&&rig.source->toPlainText()==wanted,"Panel lifecycle Unicode Save/reload changed accepted source");configured(current);
         operations.append("two-distinct-positive-apply-commits");operations.append("positive-ok-original-owner-and-pending-sibling");operations.append("exact-three-step-history-unicode-save-reload");
      }
      siblingOpen();const int closeUndo=rig.source->document()->availableUndoSteps();
      auto stable=[&] {require(rig.source->toPlainText()==wanted&&!rig.source->document()->isModified()&&rig.source->document()->availableUndoSteps()==closeUndo&&read(pathA)==wanted.toUtf8(),"Title-bar Close changed source/history/modified state/file");configured(current);siblingStable();};
      auto titleClose=[&](bool pending,QMessageBox::StandardButton answer=QMessageBox::NoButton) {
         bool visited=false;std::exception_ptr failure;QTimer timer;timer.setInterval(20);
         QObject::connect(&timer,&QTimer::timeout,&rig.window,[&] {
            auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());if (!box) return;timer.stop();
            try {require(pending&&box->windowTitle()=="Unapplied changes"&&owner&&panel()->hasChanges(),"Unexpected title-bar Close prompt");stable();auto *button=box->button(answer);require(button&&button->isVisible()&&button->isEnabled(),"Title-bar Close decision unavailable");acceptanceView(*box);prompts.append(QJsonObject{{"answer",answer==QMessageBox::Cancel?"Cancel":"Discard"},{"draft",input()->text()}});visited=true;QTest::mouseClick(button,Qt::LeftButton);} catch (...) {failure=std::current_exception();box->reject();}
         });
         require(owner,"Title-bar Close owner absent");QStyleOptionTitleBar option;option.initFrom(owner);option.titleBarFlags=owner->windowFlags();option.titleBarState=owner->windowState();option.rect=QRect(0,0,owner->width(),owner->style()->pixelMetric(QStyle::PM_TitleBarHeight,&option,owner));
         const auto rect=owner->style()->subControlRect(QStyle::CC_TitleBar,&option,QStyle::SC_TitleBarCloseButton,owner);
         require(rect.isValid()&&!rect.isEmpty()&&owner->rect().contains(rect),"Actual title-bar X has no reachable style rectangle");rectangles.append(QJsonArray{rect.x(),rect.y(),rect.width(),rect.height()});acceptanceView(*owner);
         QPointer<QMdiSubWindow> guarded=owner;const QPointF local=rect.center(),global=owner->mapToGlobal(rect.center());
         QMouseEvent move(QEvent::MouseMove,local,global,Qt::NoButton,Qt::NoButton,Qt::NoModifier),press(QEvent::MouseButtonPress,local,global,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier),release(QEvent::MouseButtonRelease,local,global,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
         timer.start();QApplication::sendEvent(guarded,&move);QApplication::sendEvent(guarded,&press);QEventLoop settle;QTimer::singleShot(50,&settle,&QEventLoop::quit);settle.exec();if (guarded) QApplication::sendEvent(guarded,&release);timer.stop();events();if (failure) std::rethrow_exception(failure);require(visited==pending,"Title-bar X omitted or unexpectedly showed its decision prompt");
      };
      owner=open("NasaPanel");require(!panel()->hasChanges()&&equal(input()->text(),current),"Reopened lifecycle panel has stale field or dirty state");titleClose(false);require(!owner,"Clean actual title-bar X retained owner");stable();operations.append("clean-actual-titlebar-x");
      owner=open("NasaPanel");draft(valid);QPointer<ResourceEditor> pending=panel();titleClose(true,QMessageBox::Cancel);
      require(owner&&pending&&panel()==pending&&input()->text()==valid,"Title-bar X Cancel lost editor/owner/draft");stable();operations.append("pending-titlebar-x-cancel");
      titleClose(true,QMessageBox::Discard);require(!owner&&!pending,"Title-bar X Discard retained old editor/owner");stable();operations.append("pending-titlebar-x-discard");siblingCancel();
      auto run=[&](const QString &phase,const QString &v,int keep,int other) {
         const auto accepted=rig.source->toPlainText();const int undo=rig.source->document()->availableUndoSteps();write(report,"unexecuted sentinel\n");acceptanceView(rig.window);const auto result=rig.window.runMission();events();rig.save(phase+"-messages.txt",rig.messages->toPlainText().toUtf8());require(result==MainWindow::RunResult::Completed,"Panel lifecycle mission failed");
         const auto raw=read(report);rig.save(phase+".csv",raw);const auto rows=QString::fromUtf8(raw).trimmed().split('\n');require(rows.size()==1,"Panel lifecycle report row count differs");const auto columns=rows[0].split(',');require(columns.size()==2&&columns[0].toDouble()==keep+1&&columns[1].toDouble()==other+2,"Panel lifecycle report differs from independent continuation witnesses");
         require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==keep+1&&Moderator::Instance()->GetInternalObject("OtherValue")->GetRealParameter("Value")==other+2&&equal(value(true),v),"Panel lifecycle native execution values/target field differ");configured(v,keep,other);
         require(rig.source->toPlainText()==accepted&&rig.source->document()->availableUndoSteps()==undo&&read(phase=="loaded"?pathB:pathA)==accepted.toUtf8(),"Panel lifecycle Run changed source/history/file");runs.append(QJsonObject{{"phase",phase},{"report",QJsonArray{keep+1,other+2}},{"native_field",value(true)},{"configured_field",value()}});
      };
      run("committed",current,41,17);
      if (actions) {
         owner=open("NasaPanel");require(!panel()->hasChanges(),"Panel-open Load requires a clean actual target");QPointer<ResourceEditor> old=panel();QMenu *file=nullptr;QAction *load=nullptr;
         for (auto *a:rig.window.menuBar()->actions()) if (a->menu()&&QString(a->text()).remove('&')=="File") file=a->menu();require(file,"Lifecycle File menu absent");
         for (auto *a:file->actions()) if (a->objectName()=="openMissionScript") load=a;require(load&&load->isEnabled(),"Lifecycle File/Open action absent");bool selected=false;std::exception_ptr failure;QTimer timer;timer.setInterval(20);
         QObject::connect(&timer,&QTimer::timeout,&rig.window,[&] {
            auto *dialog=qobject_cast<QFileDialog *>(QApplication::activeModalWidget());if (!dialog) return;timer.stop();
            try {require(owner&&old&&dialog->windowTitle()=="Open GMAT script","Panel-open File/Open lost old owner before selection");dialog->selectFile(pathB);auto *filename=dialog->findChild<QLineEdit *>("fileNameEdit");require(filename&&filename->isVisible(),"Lifecycle actual filename widget absent");filename->setText(pathB);require(QTest::qWaitFor([&] {return dialog->selectedFiles()==QStringList{pathB};},1500),"Lifecycle File/Open selected wrong Unicode path");auto *box=dialog->findChild<QDialogButtonBox *>();auto *button=box?box->button(QDialogButtonBox::Open):nullptr;require(button&&button->isVisible()&&button->isEnabled(),"Lifecycle actual Open button unavailable");acceptanceView(*dialog);selected=true;QTest::mouseClick(button,Qt::LeftButton);} catch (...) {failure=std::current_exception();dialog->reject();}
         });
         timer.start();auto *bar=rig.window.menuBar();QTest::mouseClick(bar,Qt::LeftButton,Qt::NoModifier,bar->actionGeometry(file->menuAction()).center());require(QTest::qWaitFor([&] {return file->isVisible();},1500),"Lifecycle actual File popup absent");file->setActiveAction(load);acceptanceView(*file);QTest::keyClick(file,Qt::Key_Return);timer.stop();events();if (failure) std::rethrow_exception(failure);
         require(selected&&!owner&&!old&&rig.source->toPlainText()==loaded&&!rig.source->document()->isModified()&&read(pathA)==wanted.toUtf8()&&read(pathB)==loaded.toUtf8(),"Panel-open File/Open retained stale editor or changed wrong source/file");configured(spec["loaded"].toString(),73,29);
         owner=open("NasaPanel");require(!panel()->hasChanges()&&equal(input()->text(),spec["loaded"].toString()),"Loaded lifecycle panel retained stale field");auto *button=panel()->findChild<QPushButton *>("closePanel");require(button,"Loaded lifecycle clean Close absent");QTest::mouseClick(button,Qt::LeftButton);events();require(!owner,"Loaded lifecycle clean Close retained owner");operations.append("actual-file-open-clean-panel-retired");operations.append("loaded-panel-current-value-reopened");run("loaded",spec["loaded"].toString(),73,29);
      }
      rig.save("commits.json",QJsonDocument(commits).toJson());rig.save("operations.json",QJsonDocument(operations).toJson());rig.save("prompts.json",QJsonDocument(prompts).toJson());rig.save("title-x-rectangles.json",QJsonDocument(rectangles).toJson());rig.save("runs.json",QJsonDocument(runs).toJson());rig.save("accepted.script",wanted.toUtf8());rig.save("final.script",rig.source->toPlainText().toUtf8());
      require(prompts.size()==2&&rectangles.size()==3&&commits.size()==(actions?3:0)&&runs.size()==(actions?2:1),"Panel lifecycle operation counts differ");
      QJsonArray facets{"actual-titlebar-x-clean-and-pending-current-decisions","unrelated-pending-sibling-source-history-model-file-isolation","configured-and-native-field-and-independent-continuation-report"};if (actions) {facets.append("two-distinct-positive-apply-and-owning-editor-ok");facets.append("exact-three-step-history-unicode-persistence");facets.append("actual-file-open-with-clean-panel-and-old-editor-retirement");}
      const QJsonObject proof{{"checkpoint","panel-lifecycle-"+key},{"outcome","passed"},{"case",key},{"field",field},{"kind",kind},{"actions",actions},{"positive_apply",actions?2:0},{"positive_ok",actions?1:0},{"titlebar_x",3},{"prompts",2},{"file_open",actions?1:0},{"missions",runs.size()},{"raw_report_values",2*runs.size()},{"facets",facets},{"scope","Five reused resource/page fields. Actual MDI title-bar X clean/pending Cancel/Discard with owner/editor/draft and unrelated sibling/source/history/native/file isolation. Attitude and Visual additionally make two distinct positive Apply commits and positive original-owner OK, exact three-step Undo/Redo/Unicode persistence and actual File/Open with clean target panel still present. Two-variable continuation report and native target-field configuration witnesses; no maneuver/attitude dynamics/visual rendering claim. No original ExitNo/Yes/private keyword/all-field/input/native OS/GPU/hardware/platform/stress/current NASA or full newest-build qualification."}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   } catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
     catch (std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}
   return 1;
}
