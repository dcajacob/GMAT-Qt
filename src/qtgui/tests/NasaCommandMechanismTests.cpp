#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "HelpDialog.hpp"
#include "Moderator.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QAbstractButton>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QStatusBar>
#include <QSlider>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBrowser>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *m) {if (!ok) throw std::runtime_error(m);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static QByteArray read(const QString &p) {QFile f(p);require(f.open(QIODevice::ReadOnly),"Panel mechanism artifact missing");return f.readAll();}
static void write(const QString &p,const QByteArray &bytes) {QFile f(p);require(f.open(QIODevice::WriteOnly) && f.write(bytes)==bytes.size(),"Panel mechanism artifact write failed");}
static QString digest(const QByteArray &bytes) {return QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());}
static void reveal(QWidget *c) {require(c,"Panel mechanism control missing");for (QWidget *p=c;p;p=p->parentWidget()) {for (auto *t:c->window()->findChildren<QTabWidget *>()) if (t->indexOf(p)>=0) t->setCurrentWidget(p);if (auto *s=qobject_cast<QScrollArea *>(p)) s->ensureWidgetVisible(c);}events();require(c->isVisible() && c->isEnabled(),"Panel mechanism control unreachable");}
static QJsonObject entry(const QString &path,const QString &key) {const auto document=QJsonDocument::fromJson(read(path));for (auto c:document.isArray() ? document.array():document.object()["cases"].toArray()) if (c.toObject()["key"].toString()==key) return c.toObject();throw std::runtime_error("Panel mechanism case missing");}
static QString identify(QWidget *w) {if (auto *b=qobject_cast<QAbstractButton *>(w)) return "button:"+QString(b->text()).remove('&');return QString::fromLatin1(w->metaObject()->className())+":"+w->objectName();}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("NasaCommandMechanisms");
   try {
      require(argc==5 && acceptanceWidgetPlatform(),"Expected startup, command recipes, mechanism cases and key");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Command mechanism fixture missing");
      const QString key=argv[4];const auto spec=entry(QFileInfo(argv[3]).absoluteFilePath(),key);
      const auto data=spec.contains("inline_recipe") ? spec["inline_recipe"].toObject():entry(QFileInfo(argv[2]).absoluteFilePath(),spec["recipe_key"].toString());
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto report=files.filePath("Command results Δ.txt"),saved=files.filePath("Command controls Δ.script");
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes) {if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes);};
      const QString prefix="% NASA command mechanism sentinel Δ\nCreate Variable V i;\nV = 0;\ni = 7;\nCreate Spacecraft Sat Other;\nCreate ImpulsiveBurn Burn OtherBurn;\nCreate FiniteBurn FB OtherFB;\nCreate DifferentialCorrector DC OtherDC;\nCreate Yukon Opt OtherOpt;\nCreate Propagator Prop;\nCreate XYPlot Idle OtherIdle;\nIdle.XVariable = V;\nIdle.YVariables = {i};\nIdle.ShowPlot = false;\nOtherIdle.XVariable = V;\nOtherIdle.YVariables = {i};\nOtherIdle.ShowPlot = false;\nCreate ReportFile Truth;\nTruth.Filename = '"+report+"';\nTruth.WriteHeaders = false;\nTruth.FixedWidth = false;\nTruth.Delimiter = ',';\nTruth.Precision = 17;\n"+spec["setup"].toString()+"BeginMissionSequence;\n";
      const auto wrapper=data["wrapper"].toString()+spec["wrapper_prefix"].toString(),footer=spec["footer_prefix"].toString()+data["footer"].toString();
      const QString tail="Report 'NASA anchor' Truth "+spec["report_parameters"].toString()+";\n% Preserve final comment Ω\n";
      const auto initial=prefix+wrapper+data["before"].toString()+"\n"+footer+tail;
      const auto wanted=prefix+wrapper+data["after"].toString()+"\n"+footer+tail;
      save("initial.script",initial.toUtf8());save("expected.script",wanted.toUtf8());
      MainWindow window;require(window.initialize(startup),"Command mechanism initialization failed");window.show();acceptanceView(window);events();
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Mission"),*resources=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(source && messages && tree && resources && area,"Command mechanism workspace missing");
      source->setPlainText(initial);if (!window.buildScript()) {std::cerr<<messages->toPlainText().toStdString();throw std::runtime_error("Command mechanism baseline rejected");}source->document()->setModified(false);const int undo=source->document()->availableUndoSteps();
      auto modelState=[&] {QJsonObject state;for (const auto &name:QStringList{"V","i"}) {auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(object,"Command literal object missing");state[name]=object->GetRealParameter("Value");}auto *sat=Moderator::Instance()->GetConfiguredObject("Sat");require(sat,"Command spacecraft missing");for (const auto &name:QStringList{"X","Y","Z","VX","VY","VZ"}) state["Sat."+name]=sat->GetRealParameter(name.toStdString());return state;};
      auto nodes=[&] {QJsonArray result;for (const auto &node:window.missionSnapshot().nodes) result.append(node.statement);return result;};const auto baseModel=modelState();const auto baseNodes=nodes();
      auto matches=resources->findItems("V",Qt::MatchExactly|Qt::MatchRecursive);require(matches.size()==1,"Command clean sibling resource missing");resources->itemDoubleClicked(matches.front(),0);events();QPointer<QMdiSubWindow> sibling=area->activeSubWindow();require(sibling && dynamic_cast<EditablePanel *>(sibling->widget()),"Command sibling panel missing");acceptanceView(*sibling);events();
      int index=-1;const auto snapshot=window.missionSnapshot();for (int i=0;i<snapshot.nodes.size();++i) if (missionCommandLabel(snapshot.nodes[i].statement).name=="NASA original") {require(index<0,"Duplicate target command label");index=i;}require(index>=0,"Target command label missing");
      QTreeWidgetItem *target=nullptr;for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==index) target=*it;require(target,"Actual Mission tree command missing");
      for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->tabText(i)=="Mission") tabs->setCurrentIndex(i);
      tree->expandAll();tree->setCurrentItem(target);tree->scrollToItem(target);events();acceptanceView(window);tree->itemDoubleClicked(target,0);events();
      const bool opened=QTest::qWaitFor([&] {auto *active=area->activeSubWindow();return active && dynamic_cast<CommandEditor *>(active->widget());},1500);QJsonArray openWindows;for (auto *candidate:area->subWindowList()) openWindows.append(QJsonObject{{"title",candidate->windowTitle()},{"active",candidate==area->activeSubWindow()},{"command",dynamic_cast<CommandEditor *>(candidate->widget())!=nullptr}});save("editor-opening.json",QJsonDocument(QJsonObject{{"opened",opened},{"status",window.statusBar()->currentMessage()},{"windows",openWindows}}).toJson());require(opened,"Native command editor activation did not settle");
      auto *child=area->activeSubWindow();auto *panel=child ? dynamic_cast<CommandEditor *>(child->widget()):nullptr;require(panel,"Actual command editor missing");
      auto *preview=panel->findChild<QPlainTextEdit *>("commandSource");auto *buttons=panel->findChild<QDialogButtonBox *>();require(preview && buttons && preview->toPlainText().trimmed()==data["before"].toString(),"Actual command source/buttons differ from recipe");
      auto field=[&] {auto *value=panel->findChild<QLineEdit *>(spec["control"].toString());require(value && !value->isReadOnly(),"Actual typed command field missing");return value;};
      if (spec["edit"].toString()=="source") preview->setPlainText(data["after"].toString());else field()->setText(spec["valid"].toString());events();
      const auto pending=preview->toPlainText();require(pending.trimmed()==data["after"].toString(),"Actual command edit differs from literal recipe");auto *input=field();
      auto view=[&] {if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);child->showNormal();events();child->move(20,12);child->resize(std::min(960,area->viewport()->width()-40),std::min(700,area->viewport()->height()-24));area->setActiveSubWindow(child);child->raise();events();}acceptanceView(*child);};view();reveal(input);
      auto pristine=[&] {return source->toPlainText()==initial && source->document()->availableUndoSteps()==undo && !source->document()->isModified() && modelState()==baseModel && nodes()==baseNodes && window.missionSnapshot().sourceScript==initial && sibling && sibling->isVisible() && !dynamic_cast<EditablePanel *>(sibling->widget())->hasChanges();};
      auto stable=[&] {require(pristine() && panel->hasChanges() && preview->toPlainText()==pending,"Help/focus/window action changed command draft, configured literals, mission nodes, source/history or sibling");};stable();QJsonArray windowActions,helps;
      auto menu=[&](const QString &wanted) {acceptanceView(window);std::exception_ptr failure;bool visited=false;QTimer::singleShot(0,child,[&] {auto *m=child->systemMenu();try {require(m && m->isVisible(),"Actual MDI system menu missing");QAction *selected=nullptr;for (auto *a:m->actions()) if (QString(a->text()).remove('&').compare(wanted,Qt::CaseInsensitive)==0) selected=a;require(selected && selected->isEnabled(),"MDI system menu action disabled/missing");visited=true;m->setActiveAction(selected);acceptanceView(*m);QTest::keyClick(m,Qt::Key_Return);}catch (...) {failure=std::current_exception();if (m) m->close();}});child->showSystemMenu();events();if (failure) std::rethrow_exception(failure);require(visited,"MDI system menu not inspected");events();stable();require(!window.isFullScreen(),"Panel window operation entered fullscreen");};
      // Finish layout constraints before saving the normal reference geometry.
      // A restore must reproduce that legal geometry exactly.
      auto rectangle=[](const QRect &r) {return QJsonArray{r.x(),r.y(),r.width(),r.height()};};
      const auto beforePreparation=child->geometry();child->ensurePolished();panel->ensurePolished();
      for (auto *widget:panel->findChildren<QWidget *>()) if (widget->isVisible() && widget->layout()) widget->layout()->activate();
      if (panel->layout()) panel->layout()->activate();if (child->layout()) child->layout()->activate();events();
      const auto minimum=child->minimumSizeHint();child->resize(child->size().expandedTo(minimum));events();
      save("normal-preparation.json",QJsonDocument(QJsonObject{{"before",rectangle(beforePreparation)},{"minimum",QJsonArray{minimum.width(),minimum.height()}},{"prepared",rectangle(child->geometry())}}).toJson());
      {
         QJsonArray layoutHints;for (auto *widget:panel->findChildren<QWidget *>()) if (widget->layout() && widget->minimumSizeHint().height()>250) layoutHints.append(QJsonObject{{"class",QString::fromLatin1(widget->metaObject()->className())},{"name",widget->objectName()},{"minimum",QJsonArray{widget->minimumSizeHint().width(),widget->minimumSizeHint().height()}},{"visible",widget->isVisible()}});
         save("layout-diagnostic.json",QJsonDocument(layoutHints).toJson());
         reveal(input);
         require(area->viewport()->rect().contains(child->geometry()),"Command editor normal geometry exceeds the workspace viewport");
         require(input->isVisible() && area->viewport()->rect().contains(input->mapTo(area->viewport(),input->rect().center())),"Selected spacecraft field is clipped outside the workspace");
         for (auto which:{QDialogButtonBox::Apply,QDialogButtonBox::Ok,QDialogButtonBox::Cancel,QDialogButtonBox::Close,QDialogButtonBox::Help}) {
            auto *action=buttons->button(which);require(action && action->isVisible() && area->viewport()->rect().contains(action->mapTo(area->viewport(),action->rect().center())),"Command standard action is clipped outside the workspace");
         }
      }
      const auto normal=child->geometry();const auto viewportBefore=area->viewport()->rect();menu("Maximize");require(QTest::qWaitFor([&] {return child->isMaximized();},1500),"Resource Maximize did not settle");require(child->isMaximized(),"Resource system-menu Maximize did not maximize");windowActions.append("maximized");acceptanceView(window);menu("Restore");const bool restoreSettled=QTest::qWaitFor([&] {return !child->isMaximized() && !child->isMinimized() && child->geometry()==normal;},1500);save("window-diagnostic.json",QJsonDocument(QJsonObject{{"normal",rectangle(normal)},{"restored",rectangle(child->geometry())},{"normal_geometry",rectangle(child->normalGeometry())},{"viewport_before",rectangle(viewportBefore)},{"viewport_after",rectangle(area->viewport()->rect())},{"maximized",child->isMaximized()},{"minimized",child->isMinimized()}}).toJson());if (child->geometry()!=normal) std::cerr<<"WINDOW_RESTORE expected="<<normal.x()<<","<<normal.y()<<","<<normal.width()<<","<<normal.height()<<" actual="<<child->x()<<","<<child->y()<<","<<child->width()<<","<<child->height()<<std::endl;require(restoreSettled,"Resource maximize/restore did not settle");require(!child->isMaximized() && !child->isMinimized() && child->geometry()==normal,"Resource maximize/restore lost normal geometry");windowActions.append("restored-from-maximized");acceptanceView(*child);menu("Minimize");require(QTest::qWaitFor([&] {return child->isMinimized();},1500),"Resource Minimize did not settle");require(child->isMinimized(),"Resource system-menu Minimize did not minimize");windowActions.append("minimized");acceptanceView(window);menu("Restore");require(QTest::qWaitFor([&] {return !child->isMinimized() && !child->isMaximized() && child->geometry()==normal;},1500),"Resource minimize/restore did not settle");require(!child->isMinimized() && !child->isMaximized() && child->geometry()==normal,"Resource minimize/restore lost normal geometry");windowActions.append("restored-from-minimized");acceptanceView(*child);stable();
      const auto helpRoot=QString::fromStdString(FileManager::Instance()->GetFullPathname("HELP_PATH"));const auto expectedPage=QDir(helpRoot).filePath("html/"+spec["page"].toString()+".html");require(QFileInfo(expectedPage).isReadable(),"Panel offline help page not installed");
      for (const auto &route:QStringList{"button","F1"}) {const auto routePage=route=="F1" ? spec["f1_page"].toString(spec["page"].toString()):spec["page"].toString(),routeTitle=route=="F1" ? spec["f1_title"].toString(spec["title"].toString()):spec["title"].toString();const auto routePath=QDir(helpRoot).filePath("html/"+routePage+".html");require(QFileInfo(routePath).isReadable(),"Panel route-specific offline help page not installed");reveal(input);area->setActiveSubWindow(child);child->raise();window.activateWindow();input->setFocus();events();acceptanceView(*child);if (route=="button") {auto *b=buttons->button(QDialogButtonBox::Help);require(b && b->isVisible() && b->isEnabled(),"Actual resource Help button missing");QTest::mouseClick(b,Qt::LeftButton);}else QTest::keyClick(input,Qt::Key_F1);events();QPointer<QDialog> help=nullptr;for (auto *d:window.findChildren<QDialog *>("helpDialog")) if (d->isVisible()) {require(!help,"Multiple visible resource Help dialogs");help=d;}require(help,"Resource Help did not open");auto *browser=help->findChild<QTextBrowser *>("helpBrowser");auto *status=help->findChild<QLabel *>("helpStatus");require(browser && status && browser->isReadOnly() && status->text().isEmpty() && browser->source().toLocalFile()==routePath && browser->toPlainText().contains(routeTitle),"Resource Help route opened wrong/unreadable topic");acceptanceView(*help);helps.append(QJsonObject{{"route",route},{"page",routePage},{"title",routeTitle},{"source",routePath},{"sha256",digest(read(routePath))},{"readonly",browser->isReadOnly()}});stable();QTest::keyClick(help,Qt::Key_Escape);events();require(!help,"Help Escape did not close its owning dialog");stable();}
      require(preview->tabChangesFocus(),"Command source must permit Tab navigation to standard actions");
      reveal(input);area->setActiveSubWindow(child);child->raise();window.activateWindow();input->setFocus();events();require(QApplication::focusWidget()==input,"Panel field could not receive focus");QJsonArray forward,backward;QSet<QWidget *> forwardWidgets,backwardWidgets;
      auto traverse=[&](bool reverse,QJsonArray &names,QSet<QWidget *> &visited) {names.append(identify(input));visited.insert(input);for (int i=0;i<1000;++i) {auto *current=QApplication::focusWidget();require(current && (current==panel || panel->isAncestorOf(current)),"Panel Tab focus escaped its owning editor");QTest::keyClick(current,Qt::Key_Tab,reverse ? Qt::ShiftModifier:Qt::NoModifier);events();auto *next=QApplication::focusWidget();require(next && next->isVisible() && next->isEnabled() && (next==panel || panel->isAncestorOf(next)),"Panel Tab reached hidden/disabled/outside control");if (next==input) return;require(!visited.contains(next),"Panel Tab focus cycled before returning to input");visited.insert(next);names.append(identify(next));}throw std::runtime_error("Panel Tab traversal did not return to its input");};traverse(false,forward,forwardWidgets);traverse(true,backward,backwardWidgets);require(forwardWidgets==backwardWidgets,"Reverse Tab does not cover the forward focus cycle");for (auto which:{QDialogButtonBox::Apply,QDialogButtonBox::Ok,QDialogButtonBox::Cancel,QDialogButtonBox::Close,QDialogButtonBox::Help}) require(buttons->button(which) && forwardWidgets.contains(buttons->button(which)),"Panel Tab cannot reach a standard action");stable();acceptanceView(*child);
      QJsonArray rejected;const auto defaultStatus=panel->findChild<QLabel *>("commandStatus")->text();
      auto reject=[&](const QString &bad,QDialogButtonBox::StandardButton action) {
         auto *control=field();reveal(control);control->setText(bad);events();const auto badSource=preview->toPlainText();QPointer<CommandEditor> originalPanel=panel;acceptanceView(*child);QTest::mouseClick(buttons->button(action),Qt::LeftButton);events();auto *currentPanel=dynamic_cast<CommandEditor *>(child->widget());auto *status=currentPanel ? currentPanel->findChild<QLabel *>("commandStatus"):nullptr;const bool sameOwner=originalPanel && currentPanel==originalPanel;save("rejection-diagnostic.json",QJsonDocument(QJsonObject{{"action",action==QDialogButtonBox::Apply ? "Apply":"OK"},{"value",bad},{"draft",badSource},{"applied_source",source->toPlainText()},{"same_panel",sameOwner},{"source_unchanged",source->toPlainText()==initial},{"diagnostic",status ? status->text():QString()}}).toJson());
         require(sameOwner && pristine() && child->isVisible() && panel->hasChanges() && field()->text()==bad && preview->toPlainText()==badSource && status && !status->text().isEmpty() && status->text()!=defaultStatus,"Invalid typed command Apply/OK changed draft/model/mission/source/history, closed owner or omitted diagnostic");
         rejected.append(QJsonObject{{"action",action==QDialogButtonBox::Apply ? "Apply":"OK"},{"value",bad},{"source",badSource},{"diagnostic",status->text()}});acceptanceView(*child);
      };
      for (const auto &bad:spec["invalid"].toArray()) reject(bad.toString(),QDialogButtonBox::Apply);reject(spec["invalid"].toArray().first().toString(),QDialogButtonBox::Ok);
      preview->setPlainText(pending);events();stable();acceptanceView(*child);QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();panel=dynamic_cast<CommandEditor *>(child->widget());
      require(panel && !panel->hasChanges() && child->isVisible() && sibling && sibling->isVisible() && source->toPlainText()==wanted,"Command recovery Apply did not remain clean/open with exact source and sibling");
      save("applied.script",source->toPlainText().toUtf8());require(child->close(),"Clean command owner did not close");events();source->undo();require(source->toPlainText()==initial && window.buildScript() && modelState()==baseModel,"Command recovery exact Undo/model restore failed");source->redo();require(source->toPlainText()==wanted && window.buildScript(),"Command recovery exact Redo failed");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==wanted,"Command Unicode save/reopen failed");save("mission.script",source->toPlainText().toUtf8());messages->clear();const auto result=window.runMission();events();save("messages.txt",messages->toPlainText().toUtf8());if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"Edited command mission failed");
      const auto raw=read(report);save("values.txt",raw);const auto rows=QString::fromUtf8(raw).trimmed().split('\n');const auto expected=spec["expected"].toArray();require(rows.size()==expected.size(),"Command result row count differs from independent oracle");QJsonArray observed;double maxError=0;const double tolerance=spec["absolute_tolerance"].toDouble();
      for (int r=0;r<rows.size();++r) {const auto values=rows[r].split(',');const auto truth=expected[r].toArray();require(values.size()==truth.size(),"Command result column count differs");QJsonArray numbers;for (int c=0;c<values.size();++c) {bool valid=false;const double value=values[c].trimmed().toDouble(&valid);require(valid && std::isfinite(value),"Command result is not finite numeric data");const double error=std::abs(value-truth[c].toDouble());maxError=std::max(maxError,error);require(error<=tolerance,"Command mission result differs from independent literal oracle");numbers.append(value);}observed.append(numbers);}
      save("rejections.json",QJsonDocument(rejected).toJson());
      const QJsonObject proof{{"checkpoint","command-mechanism-"+key},{"outcome","passed"},{"family",key},{"type",data["template"]},{"control",spec["control"]},{"pending",pending},{"window_actions",windowActions},{"help_routes",helps},{"normal_window",rectangle(normal)},{"workspace_viewport",rectangle(viewportBefore)},{"forward_focus",forward},{"reverse_focus",backward},{"rejections",rejected},{"expected",expected},{"observed",observed},{"max_error",maxError},{"absolute_tolerance",tolerance},{"model_before",baseModel},{"facets",QJsonArray{"actual-mission-tree-command-panel","actual-help-button-f1-escape-and-local-content","actual-mdi-system-menu-max-min-restore","tab-shifttab-including-multiline-source-action-reachability","typed-invalid-apply-and-ok-recovery","pending-configured-literals-mission-source-history-sibling-isolation","exact-history-unicode-and-independent-command-result"}},{"scope","Selected command contexts/fields/literal sets with actual Qt Mission editors. Multiline command source participates in Tab/Shift-Tab navigation, with pending text preserved. Configured literal values and mission-node/source state, not engine object identity, are checked after rejection. Solver cases use RunInitialGuess and do not qualify convergence. Numerical elapsed time/impulse/scalar reports do not establish complete science, native OS input, renderer/hardware/platform/stress/current private NASA corpus or a final full current-build regression."}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch (const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}return 1;
}
