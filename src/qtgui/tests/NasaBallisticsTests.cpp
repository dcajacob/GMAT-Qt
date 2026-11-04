#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDialog>
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
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static const QStringList fields={"DryMass","Cd","Cr","DragArea","SRPArea"};
using Values=std::array<double,5>;
static GmatBase *spacecraft(const QString &name) { auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString()); require(object,"Spacecraft missing"); return object; }
static Values readValues(const QString &name) {
   Values values; for (int i=0;i<5;++i) values[i]=spacecraft(name)->GetRealParameter(fields[i].toStdString()); return values;
}
static void checkValues(const QString &name,const Values &expected) { require(readValues(name)==expected,"Configured ballistic values differ from the entered values"); }
static ResourceEditor *openPanel(MainWindow &window,const QString &name) {
   auto *tree=window.findChild<QTreeWidget *>("Resources"); auto matches=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);
   require(matches.size()==1,"Spacecraft tree entry missing"); tree->itemDoubleClicked(matches.front(),0);
   auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow();
   auto *panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr; require(panel,"Spacecraft editor missing"); return panel;
}
static void closePanel(MainWindow &window) {
   auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); require(child,"Spacecraft window missing"); child->close();
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static void dialog(ResourceEditor *panel,const std::function<void(QDialog *)> &action) {
   std::exception_ptr failure;
   // Let the real modal window paint before performing this authored step.
   const int delay=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible" ? 120 : 0;
   QTimer::singleShot(delay,panel,[&] {
      auto *view=panel->findChild<QDialog *>("ballisticsMassDialog");
      try { require(view,"Ballistics modal missing"); action(view); }
      catch (...) { failure=std::current_exception(); if (view) view->reject(); }
   });
   auto *button=panel->findChild<QPushButton *>("spacecraftBallisticsMass"); require(button,"Ballistics button missing"); button->click();
   if (failure) std::rethrow_exception(failure);
}
static void fill(QDialog *view,const Values &values) {
   for (int i=0;i<5;++i) { auto *edit=view->findChild<QLineEdit *>("ballistics_"+fields[i]); require(edit,"Ballistics input missing"); edit->setText(QString::number(values[i],'g',17)); }
}
static void nasaCheck(const char *requirement,const char *field) {
   QJsonArray facets; for (const auto &value:{"nominal","zero","small-positive","invalid-text-rejection","nonfinite-rejection","cancel","pending-apply","undo-redo","save-reopen","execution-report","default-and-user-spacecraft"}) facets.append(value);
   const QJsonObject record={{"checkpoint",requirement},{"field",field},{"outcome","passed"},{"facets",facets}};
   std::cout<<"NASA_CHECK "<<QJsonDocument(record).toJson(QJsonDocument::Compact).constData()<<std::endl;
}

int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("NasaBallistics");
   try {
      require(argc==2,"Supply the startup file"); TestSettings settings; QTemporaryDir files; require(files.isValid(),"Cannot create test fixtures");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Engine initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Source editor missing");
      const Values nominal={975.5,2.4,1.7,18,14},zero={0,0,0,0,0},small={1e-12,1e-12,1e-12,1e-12,1e-12};
      for (const auto &name:QStringList{"DefaultSC","UserSat"}) {
         const auto reportPath=files.filePath(name+" report.txt");
         const auto source="% NASA retain unrelated source\nCreate Spacecraft "+name+";\nCreate Variable Keep;\nKeep = 7;\nCreate ReportFile Values;\nValues.Filename = '"+reportPath+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nReport Values "+name+".DryMass "+name+".Cd "+name+".Cr "+name+".DragArea "+name+".SRPArea;\nKeep = Keep + 2; % retain mission\n";
         editor->setPlainText(source); require(window.buildScript(),"Ballistics fixture failed to build");
         const auto initial=readValues(name); const auto undo=editor->document()->availableUndoSteps(); auto *panel=openPanel(window,name);
         dialog(panel,[&](QDialog *view) { fill(view,nominal); view->reject(); });
         require(!panel->hasChanges() && editor->toPlainText()==source,"Cancel changed pending source"); checkValues(name,initial);
         dialog(panel,[&](QDialog *view) {
            for (int i=0;i<5;++i) {
               const auto previous=view->findChild<QLineEdit *>("ballistics_"+fields[i])->text();
               QStringList invalid={"","not-a-number","NaN","Inf","1e999"}; if (i==0 || i==3 || i==4) invalid.append("-1");
               for (const auto &value:invalid) {
                  auto *edit=view->findChild<QLineEdit *>("ballistics_"+fields[i]); edit->setText(value);
                  view->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
                  require(view->isVisible() && !view->findChild<QLabel *>("ballisticsError")->text().isEmpty(),"Invalid ballistic input closed the modal or lacked an error");
                  require(editor->toPlainText()==source && editor->document()->availableUndoSteps()==undo && !panel->hasChanges(),"Invalid input changed source, Undo or pending values"); checkValues(name,initial);
                  edit->setText(previous);
               }
            }
            view->reject();
         });
         closePanel(window);
         for (const auto &expected:{nominal,zero,small}) {
            const auto before=editor->toPlainText(); const auto oldValues=readValues(name); panel=openPanel(window,name);
            dialog(panel,[&](QDialog *view) { fill(view,expected); view->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); });
            require(panel->hasChanges() && editor->toPlainText()==before,"OK failed to keep ballistic settings pending"); checkValues(name,oldValues);
            panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
            const auto after=editor->toPlainText(); require(after!=before && after.contains("% NASA retain unrelated source") && after.contains("Keep = Keep + 2; % retain mission"),"Apply lost unrelated source"); checkValues(name,expected);
            panel=openPanel(window,name); require(!panel->hasChanges(),"Applied form reopened dirty");
            const auto steps=editor->document()->availableUndoSteps(); panel->requestApply(); require(editor->toPlainText()==after && editor->document()->availableUndoSteps()==steps,"Repeated Apply changed source or Undo"); closePanel(window);
            editor->undo(); require(editor->toPlainText()==before && window.buildScript(),"Undo was not exact"); checkValues(name,oldValues);
            editor->redo(); require(editor->toPlainText()==after && window.buildScript(),"Redo was not exact"); checkValues(name,expected);
            const auto saved=files.filePath(name+" α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==after,"Unicode save/reopen changed source"); checkValues(name,expected);
            require(window.runMission()==MainWindow::RunResult::Completed,"Report execution failed");
            QFile report(reportPath); require(report.open(QIODevice::ReadOnly),"Ballistics report missing"); const auto values=QString::fromUtf8(report.readAll()).trimmed().split(','); require(values.size()==5,"Report lacks five ballistic fields");
            for (int i=0;i<5;++i) { bool ok=false; const auto actual=values[i].trimmed().toDouble(&ok); require(ok && std::isfinite(actual) && actual==expected[i],"Report differs from the known entered ballistic value"); }
         }
      }
      nasaCheck("FRR-4.1", "DryMass");
      nasaCheck("FRR-4.4.1", "Cd");
      nasaCheck("FRR-4.4.2", "Cr");
      nasaCheck("FRR-4.4.3", "DragArea");
      nasaCheck("FRR-4.4.4", "SRPArea");
      std::cout<<"PASS: NASA ballistic property settings, finite input checks, zero/small-positive boundaries, two spacecraft, pending/Cancel/repeated Apply, exact Undo/Redo/Unicode reopen and five-field execution reports\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
