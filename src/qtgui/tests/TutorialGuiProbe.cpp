// Recorder for tutorial replay, with an explicit reproducible RNG fixture.
// Construction uses GUI input events from the semantic QtTest driver (or the
// explicit legacy X11 driver). Neither sets resources or editor text through
// setters. The only engine fixture is the RNG seed.
#include "MainWindow.hpp"
#include "BaseException.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "FileManager.hpp"
#include "RealUtilities.hpp"
#include "EditablePanel.hpp"
#include "TutorialQtTestDriver.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QAbstractButton>
#include <QComboBox>
#include <QLabel>
#include <QMenu>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QTabBar>
#include <QFormLayout>
#include <QTableWidget>
#include <QListWidget>
#include <QScrollArea>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QSaveFile>
#include <QRegularExpression>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QSet>
#include <QScreen>
#include <cstdio>

static void write(const QString &path,const QByteArray &bytes)
{
   QSaveFile file(path);
   if (!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit())
      throw std::runtime_error(("Cannot record "+path).toStdString());
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   app.setOrganizationName("GMAT"); app.setApplicationName("GMAT Qt 6");
   QCommandLineParser parser;
   parser.addHelpOption();
   parser.addOptions({{"startup","Startup file","file"},
      {"settings-dir","Private settings","directory"},
      {"host-display-info","Print the host Qt screen scale and geometry without starting GMAT"},
      {"run","Execute a frozen comparison mission and exit"},
      {"convert-views","Convert legacy viewers for comparison"}});
   parser.addPositionalArgument("script","Only a previously GUI-created milestone or a post-freeze reference");
   parser.process(app);
   if (parser.isSet("host-display-info")) {
      const auto *screen=app.primaryScreen();
      if (!screen) return 2;
      const auto size=screen->geometry().size(),available=screen->availableGeometry().size();
      const QJsonObject info{{"scale",screen->devicePixelRatio()},
         {"logical_size",QJsonArray{size.width(),size.height()}},
         {"available_logical_size",QJsonArray{available.width(),available.height()}}};
      const auto output=QJsonDocument(info).toJson(QJsonDocument::Compact)+'\n';
      std::fwrite(output.constData(),1,output.size(),stdout);
      return 0;
   }
   const auto directory=qEnvironmentVariable("GMAT_QT_TUTORIAL_STATE");
   if (directory.isEmpty() || !QDir().mkpath(directory)) return 2;
   if (parser.isSet("settings-dir")) {
      const auto settings=QFileInfo(parser.value("settings-dir")).absoluteFilePath();
      QSettings::setDefaultFormat(QSettings::IniFormat);
      QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings);
      QSettings::setPath(QSettings::IniFormat,QSettings::SystemScope,settings+"/system");
   }
   const auto startup=QFileInfo(parser.value("startup")).absoluteFilePath();
   QDir::setCurrent(qEnvironmentVariable("GMAT_QT_TUTORIAL_WORKDIR",QCoreApplication::applicationDirPath()));
   MainWindow window; window.show();
   TutorialQtTestDriver input(window,directory);
   const auto controls=[&] {
      QJsonArray entries;
      auto rect=[](QWidget *widget,const QRect &local) {
         const auto origin=widget->mapToGlobal(local.topLeft());
         return QJsonArray{origin.x(),origin.y(),local.width(),local.height()};
      };
      auto add=[&](QWidget *widget,const QString &role,const QString &text,const QRect &local,QWidget *buddy=nullptr,int checked=-1) {
         if (!widget->isVisible() || !widget->isEnabled() || local.isEmpty()) return;
         auto *modal=QApplication::activeModalWidget();
         if (modal && widget!=modal && !modal->isAncestorOf(widget)) return;
         const auto point=widget->mapToGlobal(local.center());
         auto *hit=QApplication::widgetAt(point);
         QScrollArea *scroll=nullptr;
         if (hit!=widget && (!hit || !widget->isAncestorOf(hit))) {
            for (auto *parent=widget->parentWidget();parent;parent=parent->parentWidget())
               if ((scroll=qobject_cast<QScrollArea *>(parent))) break;
            if (!scroll || !scroll->isVisible() || scroll->viewport()->rect().contains(scroll->viewport()->mapFromGlobal(point))) return;
         }
         QJsonObject item{{"role",role},{"name",widget->objectName()},{"text",text},{"rect",rect(widget,local)},
            {"focused",widget==QApplication::focusWidget() || widget->isAncestorOf(QApplication::focusWidget())}};
         QMdiSubWindow *panel=nullptr;
         for (auto *parent=widget;parent;parent=parent->parentWidget())
            if ((panel=qobject_cast<QMdiSubWindow *>(parent))) break;
         auto *area=window.findChild<QMdiArea *>();
         item["active_panel"]=!panel || (area && area->activeSubWindow()==panel);
         if (auto *button=qobject_cast<QAbstractButton *>(widget);button && button->isCheckable()) item["checked"]=button->isChecked();
         if (checked>=0) item["checked"]=checked==Qt::Checked;
         if (buddy && buddy->isVisible() && buddy->isEnabled()) item["buddy"]=rect(buddy,buddy->rect());
         if (auto *combo=qobject_cast<QComboBox *>(widget)) {
            QJsonArray choices;
            for (int i=0;i<combo->count();++i) choices.append(combo->itemText(i));
            item["options"]=choices; item["current"]=combo->currentText();
         }
         if (scroll) {
            item["offscreen"]=true; item["scroll_rect"]=rect(scroll->viewport(),scroll->viewport()->rect());
            item["scroll_direction"]=scroll->viewport()->mapFromGlobal(point).y()<0 ? -1 : 1;
         }
         entries.append(item);
      };
      for (auto *widget:window.findChildren<QWidget *>()) {
         if (auto *button=qobject_cast<QAbstractButton *>(widget)) add(widget,"button",button->text(),widget->rect());
         else if (auto *label=qobject_cast<QLabel *>(widget)) {
            auto *buddy=label->buddy();
            if (!buddy && label->parentWidget()) if (auto *form=qobject_cast<QFormLayout *>(label->parentWidget()->layout())) {
               int row=-1; QFormLayout::ItemRole role;
               form->getWidgetPosition(label,&row,&role);
               if (row>=0) if (auto *item=form->itemAt(row,QFormLayout::FieldRole)) buddy=item->widget();
            }
            add(widget,"label",label->text(),widget->rect(),buddy);
         }
         else if (auto *combo=qobject_cast<QComboBox *>(widget)) add(widget,"combo",combo->currentText(),widget->rect());
         else if (auto *tabs=qobject_cast<QTabBar *>(widget)) {
            for (int i=0;i<tabs->count();++i) add(widget,"tab",tabs->tabText(i),tabs->tabRect(i));
         }
         if (auto *tree=qobject_cast<QTreeWidget *>(widget)) {
            for (QTreeWidgetItemIterator i(tree);*i;++i) {
               const auto area=tree->visualItemRect(*i).intersected(tree->viewport()->rect());
               if (!area.isEmpty()) add(tree->viewport(),"tree",(*i)->text(0),area);
            }
         }
         if (auto *list=qobject_cast<QListWidget *>(widget))
            for (int i=0;i<list->count();++i) {
               const auto area=list->visualItemRect(list->item(i)).intersected(list->viewport()->rect());
               if (!area.isEmpty()) add(list->viewport(),"list",list->item(i)->text(),area,nullptr,
                  list->item(i)->flags().testFlag(Qt::ItemIsUserCheckable) ? int(list->item(i)->checkState()) : -1);
            }
         if (auto *table=qobject_cast<QTableWidget *>(widget))
            for (int row=0;row<table->rowCount();++row) if (table->columnCount()>1 && table->item(row,0) && !table->isRowHidden(row)) {
               const auto label=table->visualItemRect(table->item(row,0));
               const auto value=table->visualRect(table->model()->index(row,1));
               auto *cell=table->cellWidget(row,1);
               auto *hit=QApplication::widgetAt(table->viewport()->mapToGlobal(value.center()));
               if (!value.isEmpty() && table->viewport()->rect().contains(value.center()) &&
                   (hit==table->viewport() || (cell && (hit==cell || cell->isAncestorOf(hit))))) {
                  QMdiSubWindow *panel=nullptr;
                  for (auto *parent=table->parentWidget();parent;parent=parent->parentWidget())
                     if ((panel=qobject_cast<QMdiSubWindow *>(parent))) break;
                  auto *area=window.findChild<QMdiArea *>();
                  entries.append(QJsonObject{{"role","label"},{"text",table->item(row,0)->text()},
                     {"rect",rect(table->viewport(),label)},{"buddy",rect(table->viewport(),value)},
                     {"active_panel",!panel || (area && area->activeSubWindow()==panel)}});
               }
            }
         if (!widget->objectName().isEmpty()) add(widget,"widget",QString(),widget->rect());
      }
      // QMenu popups are top-level widgets and may not belong to MainWindow's
      // QObject subtree, so inventory their visible actions separately.
      if (auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()))
         for (auto *action:menu->actions()) if (action->isEnabled() && !action->isSeparator())
            add(menu,"menu",action->text(),menu->actionGeometry(action));
      if (auto *dialog=qobject_cast<QFileDialog *>(QApplication::activeModalWidget()))
         entries.append(QJsonObject{{"role","file-dialog"},{"directory",dialog->directory().absolutePath()}});
      if (auto *popup=QApplication::activePopupWidget();popup && !qobject_cast<QMenu *>(popup)) {
         for (auto *parent=popup->parentWidget();parent;parent=parent->parentWidget())
            if (auto *combo=qobject_cast<QComboBox *>(parent)) {
               entries.append(QJsonObject{{"role","popup"},{"kind","combo"},{"owner",combo->objectName()}});
               break;
            }
      }
      write(directory+"/controls.json",QJsonDocument(entries).toJson(QJsonDocument::Compact));
   };
   int runs=0;
   bool recording=false;
   bool seededRun=false;
   bool observingRun=false;
   QMap<QString,QPair<qint64,qint64>> previousFiles;
   auto outputPaths=[&] {
      QSet<QString> paths;
      const auto output=QString::fromStdString(FileManager::Instance()->GetAbsPathname("OUTPUT_PATH"));
      for (const auto &entry:QDir(output).entryInfoList(QDir::Files)) paths.insert(entry.canonicalFilePath());
      for (auto *tree:window.findChildren<QTreeWidget *>())
         for (QTreeWidgetItemIterator i(tree);*i;++i) {
            const auto kind=(*i)->data(0,Qt::UserRole+1).toString();
            const QFileInfo entry((*i)->data(0,Qt::UserRole).toString());
            if ((kind=="report" || kind=="ephemeris") && entry.isFile()) paths.insert(entry.canonicalFilePath());
         }
      return paths;
   };
   QString lastStatus;
   auto capture=[&](const QString &status) {
      if (recording) return;
      recording=true;
      try {
         if (status=="Running mission…" && !observingRun) {
            previousFiles.clear();
            for (const auto &path:outputPaths()) {
               const QFileInfo info(path);
               previousFiles[path]=qMakePair(info.lastModified().toMSecsSinceEpoch(),info.size());
            }
            observingRun=true;
         }
         const auto seedText=qEnvironmentVariable("GMAT_QT_TUTORIAL_SEED");
         if (status=="Running mission…" && !seededRun && !seedText.isEmpty()) {
            bool valid=false; const auto seed=seedText.toUInt(&valid);
            if (!valid) throw std::runtime_error("Invalid tutorial random seed");
            GmatMathUtil::SetSeed(seed);
            seededRun=true;
         }
         if (!window.isRunning()) { seededRun=false; observingRun=false; }
         const auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
         const auto source=editor ? editor->toPlainText().toUtf8() : QByteArray();
         write(directory+"/current.script",source);
         QJsonObject state{{"status",status},{"running",window.isRunning()},
            {"source_sha256",QString::fromLatin1(QCryptographicHash::hash(source,QCryptographicHash::Sha256).toHex())},
            {"command_count",window.missionSnapshot().nodes.size()}};
         state["window_size"]=QJsonArray{window.width(),window.height()};
         state["window_maximized"]=window.isMaximized();
         state["device_pixel_ratio"]=window.devicePixelRatioF();
         if (window.screen()) state["screen_size"]=QJsonArray{window.screen()->geometry().width(),window.screen()->geometry().height()};
         if (!seedText.isEmpty()) state["random_seed"]=seedText;
         if (auto *area=window.findChild<QMdiArea *>()) if (auto *panel=area->activeSubWindow())
            if (auto *selected=qobject_cast<QPlainTextEdit *>(panel->widget());selected &&
                (selected->objectName()=="scriptEditor" || selected->objectName()=="inactiveScriptEditor"))
               state["selected_document_active"]=selected->objectName()=="scriptEditor";
         if (auto *messages=window.findChild<QPlainTextEdit *>("messageWindow"))
            write(directory+"/messages.txt",messages->toPlainText().toUtf8());
         if (status!=lastStatus && (status=="Mission completed" || status=="Mission stopped" || status.startsWith("Mission failed"))) {
            const auto prefix=QString("/run-%1").arg(++runs,3,10,QLatin1Char('0'));
            state["run"]=runs;
            write(directory+prefix+".script",source);
            if (status=="Mission completed") {
               QJsonObject outputs;
               const auto output=QString::fromStdString(FileManager::Instance()->GetAbsPathname("OUTPUT_PATH"));
               const auto snapshot=directory+prefix+"-output";
               QDir().mkpath(snapshot);
               // Help chapters can explicitly choose an absolute report path
               // in their own work directory. Read the frontend's report and
               // ephemeris registrations as well as the default output folder.
               QJsonArray unchanged;
               for (const auto &path:outputPaths()) {
                  const QFileInfo entry(path);
                  if (entry.fileName()=="GmatLog.txt") continue;
                  if (previousFiles.contains(path) && previousFiles.value(path)==qMakePair(entry.lastModified().toMSecsSinceEpoch(),entry.size())) {
                     unchanged.append(path); continue;
                  }
                  if (outputs.contains(entry.fileName())) throw std::runtime_error("Output basename collision");
                  QFile file(entry.absoluteFilePath());
                  if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot read a mission output");
                  QCryptographicHash hash(QCryptographicHash::Sha256);
                  if (!hash.addData(&file)) throw std::runtime_error("Cannot hash a mission output");
                  if (!QFile::copy(entry.absoluteFilePath(),snapshot+"/"+entry.fileName()))
                     throw std::runtime_error("Cannot preserve a mission output");
                  QJsonObject identity{{"bytes",entry.size()},{"sha256",QString::fromLatin1(hash.result().toHex())},{"source_path",path}};
                  if (entry.suffix()=="txt" || entry.suffix()=="data" || entry.suffix()=="gmd") {
                     file.seek(0);
                     auto text=QString::fromUtf8(file.readAll());
                     // Estimator report header metadata, never an observation
                     // epoch or a computed value. Raw bytes are retained too.
                     text.replace(QRegularExpression("(?m)(Run Date[ \\t]*:[ \\t]*)[^\\r\\n]*"),"\\1<wall-clock>");
                     identity["content_sha256"]=QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(),QCryptographicHash::Sha256).toHex());
                     identity["whitespace_sha256"]=QString::fromLatin1(QCryptographicHash::hash(text.simplified().toUtf8(),QCryptographicHash::Sha256).toHex());
                  }
                  outputs[entry.fileName()]=identity;
               }
               state["output_files"]=outputs;
               state["preexisting_unchanged_files"]=unchanged;
               QJsonObject spacecraft;
               for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SPACECRAFT)) {
                  auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject(name));
                  if (!sat) continue;
                  const auto epoch=sat->GetEpoch();
                  const auto cartesian=sat->GetMJ2000State(epoch);
                  QJsonArray values;
                  for (int i=0;i<6;++i) values.append(cartesian[i]);
                  spacecraft[QString::fromStdString(name)]=QJsonObject{
                     {"epoch_a1mjd",epoch},{"j2000_body",QString::fromStdString(sat->GetJ2000BodyName())},
                     {"cartesian_km_km_s",values},{"total_mass_kg",sat->GetRealParameter("TotalMass")}};
               }
               state["spacecraft"]=spacecraft;
               QJsonObject summaries;
               for (const auto &frame:window.summaryFrames()) {
                  try { summaries[frame]=window.summaryText(-1,frame,true); }
                  catch (BaseException &error) { summaries[frame]=QJsonObject{{"error",QString::fromStdString(error.GetFullMessage())}}; }
                  catch (const std::exception &error) { summaries[frame]=QJsonObject{{"error",QString::fromUtf8(error.what())}}; }
               }
               state["summaries"]=summaries;
            }
            write(directory+prefix+".json",QJsonDocument(state).toJson());
         }
         write(directory+"/state.json",QJsonDocument(state).toJson());
         controls();
         lastStatus=status;
      } catch (const std::exception &error) {
         std::fprintf(stderr,"Tutorial observation failed: %s\n",error.what()); app.exit(3);
      }
      recording=false;
   };
   QObject::connect(window.statusBar(),&QStatusBar::messageChanged,&window,capture);
   const bool ready=window.initialize(startup);
   bool loaded=ready;
   if (!parser.positionalArguments().isEmpty()) {
      loaded=ready && window.loadScript(QFileInfo(parser.positionalArguments().first()).absoluteFilePath());
      if (loaded) loaded=(!parser.isSet("convert-views") || window.convertOpenFramesScript()) && window.buildScript();
   }
   QTimer timer;
   QObject::connect(&timer,&QTimer::timeout,&window,[&] { capture(window.statusBar()->currentMessage()); });
   timer.start(200);
   // External input is asynchronous. A zero-timer acknowledgement lets the
   // replay wait for Qt to consume the input before sending the next action.
   // This observes the event loop; it never performs a construction operation.
   QTimer barrierTimer;
   QByteArray acknowledged;
   QObject::connect(&barrierTimer,&QTimer::timeout,&window,[&] {
      QFile request(directory+"/barrier.request");
      if (!request.open(QIODevice::ReadOnly)) return;
      const auto value=request.readAll();
      if (value.isEmpty() || value==acknowledged) return;
      acknowledged=value;
      QTimer::singleShot(0,&window,[&,value] { controls(); write(directory+"/barrier.response",value); });
   });
   barrierTimer.start(30);
   QTimer::singleShot(0,&window,[&] {
      if (!loaded) { capture("Initialization or build failed"); app.exit(2); return; }
      if (parser.isSet("run")) {
         const auto result=window.runMission();
         capture(window.statusBar()->currentMessage());
         app.exit(result==MainWindow::RunResult::Completed ? 0 : 1);
      } else if (parser.positionalArguments().isEmpty()) window.showWelcome(true);
   });
   return app.exec();
}
