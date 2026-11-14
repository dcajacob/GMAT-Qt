#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "FileManager.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QDialogButtonBox>
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
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <exception>
#include <stdexcept>

static void require(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }
static QMap<QString, QString> properties()
{
   auto *model = Moderator::Instance()->GetConfiguredObject("Gravity"); require(model, "Gravity model missing");
   QMap<QString, QString> values;
   for (const auto &property : resourceProperties(*model)) values.insert(property.name, property.value);
   return values;
}
static void gravityDomainChecks(MainWindow &window,QPlainTextEdit &editor,QTreeWidget &tree,QMdiArea &area,const QString &source)
{
   const auto potential=QString::fromStdString(FileManager::Instance()->GetFullPathname("JGM2_FILE"));
   QFile modelFile(potential); require(modelFile.open(QIODevice::ReadOnly),"JGM2 degree-limit fixture missing");
   const auto supplied=modelFile.readAll(); require(supplied.contains("POTFIELD 70 70"),"JGM2 fixture no longer has the reviewed 70-by-70 boundary");
   QTemporaryDir files; require(files.isValid(),"Gravity file fixture directory unavailable");
   const auto lowOrder=files.filePath("Order two Δ.cof"); QByteArray rectangular;
   for (auto line:supplied.split('\n')) {
      if (line.startsWith("POTFIELD")) line.replace(11,3,"  2");
      if (line.startsWith("RECOEF") && line.mid(11,3).trimmed().toInt()>2) continue;
      rectangular+=line+'\n';
   }
   { QFile output(lowOrder); require(output.open(QIODevice::WriteOnly) && output.write(rectangular)==rectangular.size(),"Could not write bounded-order potential fixture"); }
   struct Case { QString name; QMap<QString,QString> fields; bool valid; };
   const QList<Case> cases={
      {"degree-over-file",{{"Degree","71"},{"Order","0"}},false},
      {"degree-order-at-file",{{"Degree","70"},{"Order","70"}},true},
      {"degree-order-over-file",{{"Degree","71"},{"Order","71"}},false},
      {"order-at-file",{{"PotentialFile",lowOrder},{"Degree","4"},{"Order","2"}},true},
      {"order-over-file",{{"PotentialFile",lowOrder},{"Degree","4"},{"Order","3"}},false},
      {"stm-zero",{{"StmLimit","0"}},true},{"stm-one",{{"StmLimit","1"}},true},
      {"stm-default",{{"StmLimit","100"}},true},{"stm-int-max",{{"StmLimit","2147483647"}},true},
      {"stm-negative",{{"StmLimit","-1"}},false},{"stm-fraction",{{"StmLimit","1.5"}},false},
      {"stm-nan",{{"StmLimit","nan"}},false},{"stm-infinity",{{"StmLimit","inf"}},false},
      {"stm-empty",{{"StmLimit",""}},false},{"stm-text",{{"StmLimit","bad"}},false},
      {"stm-int-overflow",{{"StmLimit","2147483648"}},false}};
   for (const auto &test:cases) {
      editor.setPlainText(source); require(window.buildScript(),"Gravity domain fixture failed");
      const auto before=properties(); const auto undo=editor.document()->availableUndoSteps();
      const auto items=tree.findItems("Gravity",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Gravity domain resource missing");
      tree.itemDoubleClicked(items.front(),0); events(); auto *child=area.currentSubWindow();
      auto *panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr; require(panel,"Gravity domain panel missing"); acceptanceView(window);
      auto activate=[&](QWidget *control) { for (auto *page=control;page;page=page->parentWidget()) for (auto *tabs:window.findChildren<QTabWidget *>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page); events(); };
      auto field=[&](const QString &name) {
         auto *grid=panel->findChild<QTableWidget *>("resourceProperties"); require(grid,"Gravity domain grid missing"); activate(grid);
         auto *sections=panel->findChild<QTabBar *>("propertySections"); require(sections,"Gravity domain sections missing");
         for (int i=0;i<sections->count();++i) if (sections->tabText(i)=="All Properties") sections->setCurrentIndex(i);
         for (int row=0;row<grid->rowCount();++row) if (grid->item(row,0)->text()=="GravityField.Earth."+name) {
            auto *value=grid->item(row,1); grid->scrollToItem(value); events(); require(value && !grid->isRowHidden(row) && value->flags().testFlag(Qt::ItemIsEditable),"Gravity domain input is not visible/editable"); return value;
         }
         throw std::runtime_error("Gravity domain field missing");
      };
      for (auto it=test.fields.begin();it!=test.fields.end();++it) field(it.key())->setText(it.value());
      auto *mode=panel->findChild<QComboBox *>("resource_ErrorControl"); require(mode,"Domain ErrorControl missing"); activate(mode); mode->setCurrentText("RSSState");
      require(panel->hasChanges() && properties()==before && editor.toPlainText()==source,"Gravity domain edits escaped pending isolation");
      panel->requestApply(); events();
      if (!test.valid) {
         if (child->widget()!=panel || editor.toPlainText()!=source) std::cerr<<"Invalid gravity domain accepted: "<<test.name.toStdString()<<std::endl;
         require(child->widget()==panel && panel->hasChanges() && properties()==before && editor.toPlainText()==source && editor.document()->availableUndoSteps()==undo,"Invalid gravity domain did not reject atomically");
         auto *status=panel->findChild<QLabel *>("resourceStatus"); require(status && !status->text().isEmpty(),"Invalid gravity domain omitted diagnostic");
         for (auto it=test.fields.begin();it!=test.fields.end();++it) require(field(it.key())->text()==it.value(),"Rejected gravity domain lost pending text");
         field("Degree")->setText("6"); field("Order")->setText("2"); field("StmLimit")->setText("5"); panel->requestApply(); events();
      }
      panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel && !panel->hasChanges(),"Valid/corrected gravity domain failed to apply");
      const auto after=editor.toPlainText(); const auto values=properties();
      auto expected=test.valid ? test.fields : QMap<QString,QString>{{"Degree","6"},{"Order","2"},{"StmLimit","5"}};
      if (test.fields.contains("PotentialFile")) expected["PotentialFile"]=lowOrder;
      for (auto it=expected.begin();it!=expected.end();++it) {
         const auto actual=values.value("GravityField.Earth."+it.key());
         if (it.key()=="PotentialFile") require(QFileInfo(actual).canonicalFilePath()==QFileInfo(it.value()).canonicalFilePath(),"Applied gravity potential file differs");
         else require(actual.toLongLong()==it.value().toLongLong(),"Applied gravity domain value differs");
      }
      require(values.value("ErrorControl")=="RSSState","Mixed domain Apply lost ErrorControl");
      require(child->close(),"Clean gravity domain panel failed to close"); events();
      editor.undo(); require(editor.toPlainText()==source && window.buildScript() && properties()==before,"Gravity domain Undo differs");
      editor.redo(); require(editor.toPlainText()==after && window.buildScript() && properties()==values,"Gravity domain Redo differs");
      const QJsonObject proof{{"checkpoint","gravity-domain."+test.name},{"case",test.name},{"valid",test.valid},{"outcome","passed"},
         {"facets",QJsonArray{"visible-field","file-boundary","integer-domain","pending-isolation","atomic-apply","rejection-retains-input","exact-undo-redo"}}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
   }
}
static void tideDomainChecks(MainWindow &window,QPlainTextEdit &editor,QTreeWidget &tree,QMdiArea &area,const QString &baseSource)
{
   QTemporaryDir files; require(files.isValid(),"Tide fixture directory missing");
   auto file=[&](const QString &name,const QByteArray &data) { const auto path=files.filePath(name); QFile output(path); require(output.open(QIODevice::WriteOnly) && output.write(data)==data.size(),"Tide fixture write failed"); return path; };
   const QByteArray love="k 2 0 0.30190\nk 2 1 0.29830\nk 2 2 0.30102\n";
   const auto valid=file("Earth love Δ.tide",love),upper=file("Upper.TIDE",love),wrong=file("Wrong.txt",love),empty=file("Empty.tide",{}),comments=file("Comments.tide","% No coefficients\n"),bad=file("Bad.tide","not a tide file\n"),shortK=file("ShortK.tide","k 2 0\n"),shortPlus=file("ShortPlus.tide","kplus 0\n"),nan=file("Nan.tide","k 2 0 nan\n"),inf=file("Inf.tide","k 2 0 inf\n"),degree=file("Degree.tide","k 4 0 0.1\n"),order=file("Order.tide","k 2 4 0.1\n"),negative=file("Negative.tide","k -1 0 0.1\n");
   const auto directory=files.filePath("Directory.tide"); require(QDir().mkdir(directory),"Tide directory fixture failed");
   const auto lunar=QDir::current().absoluteFilePath("../data/gravity/luna/grgm900c.tide"); require(QFileInfo(lunar).isFile(),"NASA lunar Love-number fixture missing");
   struct Case { QString name,body,file,model; bool valid; };
   const QList<Case> cases={{"builtin-none","Earth","","None",true},{"builtin-solid","Earth","","Solid",true},{"builtin-pole","Earth","","SolidAndPole",true},
      {"file-solid","Earth",valid,"Solid",true},{"file-pole","Earth",valid,"SolidAndPole",true},{"uppercase-extension","Earth",upper,"Solid",true},{"lunar-solid","Luna",lunar,"Solid",true},
      {"lunar-no-data","Luna","","Solid",false},{"lunar-pole","Luna",lunar,"SolidAndPole",false},{"wrong-extension","Earth",wrong,"Solid",false},{"missing-file","Earth",files.filePath("Missing.tide"),"Solid",false},
      {"directory","Earth",directory,"Solid",false},{"empty","Earth",empty,"Solid",false},{"comments-only","Earth",comments,"Solid",false},{"malformed","Earth",bad,"Solid",false},
      {"incomplete-k","Earth",shortK,"Solid",false},{"incomplete-kplus","Earth",shortPlus,"Solid",false},{"nan-coefficient","Earth",nan,"Solid",false},{"infinite-coefficient","Earth",inf,"Solid",false},
      {"degree-range","Earth",degree,"Solid",false},{"order-range","Earth",order,"Solid",false},{"negative-index","Earth",negative,"Solid",false},{"unknown-model","Earth","","NotATide",false}};
   for (const auto &test:cases) {
      QString source=baseSource; source.replace("Earth",test.body); source.insert(source.indexOf("Gravity.PrimaryBodies"),"Gravity.CentralBody = "+test.body+";\n");
      editor.setPlainText(source); require(window.buildScript(),"Tide fixture failed to build"); const auto before=properties(); const auto undo=editor.document()->availableUndoSteps();
      const auto items=tree.findItems("Gravity",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Tide force resource missing"); tree.itemDoubleClicked(items.front(),0); events();
      auto *child=area.currentSubWindow(); auto *panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr; require(panel,"Tide editor missing"); acceptanceView(window);
      auto activate=[&](QWidget *control) { for (auto *page=control;page;page=page->parentWidget()) for (auto *tabs:window.findChildren<QTabWidget *>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page); events(); };
      const auto prefix="GravityField."+test.body+".";
      auto field=[&](const QString &name) {
         auto *grid=panel->findChild<QTableWidget *>("resourceProperties"); require(grid,"Tide property grid missing"); activate(grid); auto *sections=panel->findChild<QTabBar *>("propertySections"); require(sections,"Tide sections missing");
         for (int i=0;i<sections->count();++i) if (sections->tabText(i)=="All Properties") sections->setCurrentIndex(i);
         for (int row=0;row<grid->rowCount();++row) if (grid->item(row,0)->text()==prefix+name) { auto *value=grid->item(row,1); grid->scrollToItem(value); events(); require(value && !grid->isRowHidden(row) && value->flags().testFlag(Qt::ItemIsEditable),"Tide input is not visible/editable"); return value; }
         throw std::runtime_error("Tide field missing");
      };
      if (test.name=="file-solid") {
         const auto initial=field("TideFile")->text(); auto *browse=panel->findChild<QPushButton *>("chooseProperty_"+prefix+"TideFile"); require(browse && browse->isVisible(),"Visible tide file picker missing");
         for (bool accept:{false,true}) {
            std::exception_ptr failure; bool inspected=false;
            QTimer::singleShot(0,panel,[&] {
               auto *dialog=panel->findChild<QFileDialog *>("resourceFileDialog");
               try {
                  require(dialog,"Tide file dialog did not open"); acceptanceView(*dialog); inspected=true;
                  if (accept) {
                     // Populate the displayed full-path entry and use Open.
                     // selectFile followed immediately by accept can race the
                     // native QFileSystemModel directory population.
                     dialog->selectFile(valid);
                     auto *entry=dialog->findChild<QLineEdit *>("fileNameEdit"); require(entry && entry->isVisible(),"Tide filename entry missing"); entry->setText(valid); events();
                     auto *buttons=dialog->findChild<QDialogButtonBox *>(); auto *open=buttons ? buttons->button(QDialogButtonBox::Open) : nullptr;
                     require(open && open->isEnabled(),"Tide Open action is not enabled for the valid file"); open->click();
                     require(dialog->result()==QDialog::Accepted,"Tide file picker did not accept the selected path");
                  } else dialog->reject();
               } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
            });
            browse->click(); if (failure) std::rethrow_exception(failure); require(inspected,"Tide file picker was not inspected");
            require(field("TideFile")->text()==(accept ? valid : initial) && editor.toPlainText()==source,"Tide picker Cancel/selection changed the wrong state");
         }
      }
      field("TideFile")->setText(test.file); field("TideModel")->setText(test.model);
      auto *mode=panel->findChild<QComboBox *>("resource_ErrorControl"); require(mode,"Tide mixed selector missing"); activate(mode); mode->setCurrentText("RSSState");
      require(panel->hasChanges() && properties()==before && editor.toPlainText()==source,"Tide edits escaped pending isolation"); panel->requestApply(); events();
      if (!test.valid) {
         if (child->widget()!=panel || editor.toPlainText()!=source) std::cerr<<"Invalid tide accepted: "<<test.name.toStdString()<<std::endl;
         require(child->widget()==panel && panel->hasChanges() && properties()==before && editor.toPlainText()==source && editor.document()->availableUndoSteps()==undo,"Invalid tide did not reject atomically");
         require(!panel->findChild<QLabel *>("resourceStatus")->text().isEmpty() && field("TideFile")->text()==test.file && field("TideModel")->text()==test.model,"Tide rejection lost diagnostic or pending input");
         field("TideFile")->setText(""); field("TideModel")->setText("None"); panel->requestApply(); events();
      }
      panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel && !panel->hasChanges(),"Valid/corrected tide did not apply"); const auto after=editor.toPlainText(); const auto values=properties();
      require(values.value(prefix+"TideModel")==(test.valid ? test.model : "None") && values.value("ErrorControl")=="RSSState","Applied tide model differs");
      const auto expectedFile=test.valid ? test.file : QString(); const auto actualFile=values.value(prefix+"TideFile");
      require(expectedFile.isEmpty() ? actualFile.isEmpty() : QFileInfo(actualFile).canonicalFilePath()==QFileInfo(expectedFile).canonicalFilePath(),"Applied tide file differs");
      require(after.contains("Keep = 13; % preserve unrelated source") && after.endsWith("BeginMissionSequence;\n"),"Tide Apply rewrote unrelated source");
      require(child->close(),"Clean tide editor did not close"); events(); editor.undo(); require(editor.toPlainText()==source && window.buildScript() && properties()==before,"Tide Undo was not exact"); editor.redo(); require(editor.toPlainText()==after && window.buildScript() && properties()==values,"Tide Redo was not exact");
      const QJsonObject proof{{"checkpoint","tide-domain."+test.name},{"case",test.name},{"body",test.body},{"valid",test.valid},{"outcome","passed"},{"facets",QJsonArray{"visible-field","tide-file-model","atomic-apply","rejection-retains-input","correct-and-apply","exact-undo-redo"}}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
   }
}
int main(int argc, char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc, argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("NasaGravityValidation");
   try {
      require(argc == 2 && acceptanceWidgetPlatform(), "Expected startup and supported GUI platform");
      TestSettings settings; const auto startup = QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup), "Runtime initialization failed"); window.show();
      auto *editor = window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree = window.findChild<QTreeWidget *>("Resources"); auto *area = window.findChild<QMdiArea *>("workspace");
      require(editor && tree && area, "Workspace missing");
      const QString source = "% Gravity coupled-bound validation\nCreate ForceModel Gravity;\nGravity.PrimaryBodies = {Earth};\nGravity.GravityField.Earth.Degree = 4;\nGravity.GravityField.Earth.Order = 4;\nGravity.ErrorControl = RSSStep;\nCreate Variable Keep;\nKeep = 13; % preserve unrelated source\nBeginMissionSequence;\n";
      const QList<QStringList> cases = {{"0","0","valid"},{"2","1","valid"},{"4","4","valid"},{"8","6","valid"},
         {"3","4","invalid"},{"0","1","invalid"},{"-1","0","invalid"},{"0","-1","invalid"},
         {"1.5","0","invalid"},{"nan","0","invalid"},{"0","inf","invalid"},{"2","3","invalid"}};
      QJsonArray completed;
      for (const auto &test : cases) {
         editor->setPlainText(source); require(window.buildScript(), "Gravity fixture did not build");
         const auto before = properties(); const auto undo = editor->document()->availableUndoSteps();
         const auto items = tree->findItems("Gravity", Qt::MatchExactly | Qt::MatchRecursive); require(items.size() == 1, "Gravity tree entry missing");
         tree->itemDoubleClicked(items.front(), 0); events(); auto *child = area->currentSubWindow();
         auto *panel = child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr; require(panel, "Gravity editor missing"); acceptanceView(window);
         auto activate = [&](QWidget *control) {
            for (auto *page = control; page; page = page->parentWidget()) for (auto *tabs : window.findChildren<QTabWidget *>()) if (tabs->indexOf(page) >= 0) tabs->setCurrentWidget(page);
            events();
         };
         auto field = [&](const QString &name) {
            auto *table = panel->findChild<QTableWidget *>("resourceProperties"); require(table, "Gravity table missing"); activate(table);
            if (auto *sections = panel->findChild<QTabBar *>("propertySections")) for (int i=0;i<sections->count();++i) if (sections->tabText(i)=="All Properties") sections->setCurrentIndex(i);
            for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==name) {
               auto *value=table->item(row,1); table->scrollToItem(value); events();
               require(!table->isRowHidden(row) && value && value->flags().testFlag(Qt::ItemIsEditable), "Gravity bound is not editable"); return value;
            }
            throw std::runtime_error("Gravity degree/order field missing");
         };
         field("GravityField.Earth.Degree")->setText(test[0]); field("GravityField.Earth.Order")->setText(test[1]);
         auto *mode=panel->findChild<QComboBox *>("resource_ErrorControl"); require(mode,"ErrorControl selector missing"); activate(mode); mode->setCurrentText("RSSState");
         require(panel->hasChanges() && properties()==before && editor->toPlainText()==source, "Gravity changes escaped pending isolation");
         const auto oldStatus=panel->findChild<QLabel *>("resourceStatus")->text(); panel->requestApply(); events();
         if (test[2]=="invalid") {
            const bool rejected=child->widget()==panel && panel->hasChanges() && editor->toPlainText()==source && properties()==before && editor->document()->availableUndoSteps()==undo;
            if (!rejected) std::cerr << "Invalid pair accepted: degree=" << test[0].toStdString() << " order=" << test[1].toStdString() << "; configured " << properties().value("GravityField.Earth.Degree").toStdString() << '/' << properties().value("GravityField.Earth.Order").toStdString() << '\n';
            require(rejected, "Invalid gravity pair was not rejected atomically");
            auto *status=panel->findChild<QLabel *>("resourceStatus"); require(status && !status->text().isEmpty() && status->text()!=oldStatus, "Invalid gravity values lack a diagnostic");
            require(field("GravityField.Earth.Degree")->text()==test[0] && field("GravityField.Earth.Order")->text()==test[1], "Invalid Apply lost pending gravity input");
            field("GravityField.Earth.Degree")->setText("6"); field("GravityField.Earth.Order")->setText("2"); panel->requestApply(); events();
         }
         panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel && !panel->hasChanges(), "Valid gravity pair or corrected input failed to apply");
         const auto after=editor->toPlainText(); const auto values=properties();
         require(values.value("GravityField.Earth.Degree").toInt()==(test[2]=="invalid" ? 6 : test[0].toInt()) && values.value("GravityField.Earth.Order").toInt()==(test[2]=="invalid" ? 2 : test[1].toInt()) && values.value("ErrorControl")=="RSSState", "Applied coupled gravity values differ");
         require(child->close(), "Clean gravity editor did not close"); events();
         editor->undo(); require(editor->toPlainText()==source && window.buildScript() && properties()==before, "Gravity Apply Undo was not exact");
         editor->redo(); require(editor->toPlainText()==after && window.buildScript() && properties()==values, "Gravity Apply Redo was not exact");
         completed.append(test[2]+"-"+test[0]+"-"+test[1]);
      }
      gravityDomainChecks(window,*editor,*tree,*area,source);
      tideDomainChecks(window,*editor,*tree,*area,source);
      // Replacing polyhedron contributors defers their own validation, not
      // validation of an unrelated spherical-harmonic gravity component.
      editor->setPlainText(source); require(window.buildScript(), "Mixed-force fixture failed to build");
      const auto beforeMixed=properties(); const auto mixedUndo=editor->document()->availableUndoSteps();
      const auto mixedError=window.applyResourceChanges("Gravity",
         {{"GravityField.Earth.Degree","3"},{"GravityField.Earth.Order","4"},{"@PolyhedronForces","[]"}},source);
      require(!mixedError.isEmpty() && editor->toPlainText()==source && properties()==beforeMixed &&
         editor->document()->availableUndoSteps()==mixedUndo,"Polyhedron transaction bypassed gravity bounds or changed state");
      const QJsonObject mixedProof{{"checkpoint","gravity-bounds.mixed-polyhedron"},{"outcome","passed"},
         {"facets",QJsonArray{"frontend-transaction","coupled-degree-order","atomic-rejection","source-history"}}};
      std::cout << "NASA_CHECK " << QJsonDocument(mixedProof).toJson(QJsonDocument::Compact).constData() << std::endl;
      for (const auto &name : completed) {
         const QJsonObject proof{{"checkpoint","gravity-bounds."+name.toString()},{"case",name}, {"outcome","passed"},
            {"facets",QJsonArray{"gui-input","coupled-degree-order","atomic-rejection","pending-preserved","correct-and-apply","exact-undo-redo"}}};
         std::cout << "NASA_CHECK " << QJsonDocument(proof).toJson(QJsonDocument::Compact).constData() << std::endl;
      }
      return 0;
   } catch (BaseException &error) { std::cerr << "FAIL " << error.GetFullMessage() << '\n'; }
     catch (const std::exception &error) { std::cerr << "FAIL " << error.what() << '\n'; }
   return 1;
}
