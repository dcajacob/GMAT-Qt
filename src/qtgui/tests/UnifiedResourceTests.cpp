#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceForm.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Array.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QMessageBox>
#include <QSpinBox>
#include <QTreeWidget>
#include <iostream>
#include <cmath>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static ResourceEditor *panel(MainWindow &window,const QString &name)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (child->property("resourceName")==name) return dynamic_cast<ResourceEditor *>(child->widget());
   throw std::runtime_error("Resource panel missing");
}
static void closePanels(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (!child->property("resourceName").toString().isEmpty()) child->close();
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   if (!acceptanceWidgetPlatform() || argc<2 || argc>3) return 2;
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("UnifiedResources");
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Temporary files unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString baseline="% Keep source α and implicit settings\nCreate Spacecraft Vehicle;\nCreate Variable Keep;\nKeep = 7; % keep comment\nBeginMissionSequence;\nKeep = Keep + 2;\n";
      editor->setPlainText(baseline); require(window.buildScript(),"Baseline failed");
      // A clean retained spacecraft form must stay clean across model rebuilds.
      // Reading it must not consult the previous model's attitude references.
      {
         const auto attitudeSource=QString(baseline).replace("Create Variable Keep;","Vehicle.Attitude = Spinner;\nVehicle.AttitudeDisplayStateType = EulerAngles;\nCreate Variable Keep;");
         editor->setPlainText(attitudeSource); require(window.buildScript(),"Writable attitude fixture failed");
         QString staleError; QWidget owner;
         ResourceEditor retained(*Moderator::Instance()->GetConfiguredObject("Vehicle"),[&](const auto &changes) {
            staleError=window.applyResourceChanges("Vehicle",changes,attitudeSource); return staleError;
         },&owner,attitudeSource);
         require(!retained.hasChanges(),"Untouched spacecraft form has pending edits");
         require(window.applyResourceChanges("Vehicle",{{"DryMass","650"}},attitudeSource).isEmpty(),"Retained-form rebuild fixture failed");
         require(!retained.hasChanges(),"Model rebuild changed untouched attitude input");
         const auto current=editor->toPlainText();
         require(window.applyResourceChanges("Vehicle",{{"DryMass","675"}},current).isEmpty() && !retained.hasChanges(),"Second rebuild read an old attitude model");
         QLineEdit *angle=nullptr;
         for (auto *field:retained.findChildren<QLineEdit *>()) if (field->objectName().startsWith("attitude_EulerAngle") || field->objectName()=="attitude_Q1") { angle=field; break; }
         require(angle,"Retained attitude input missing"); const auto old=angle->text(); angle->setText("1");
         require(retained.hasChanges(),"Pending attitude edit was not detected");
         const auto stable=editor->toPlainText(); retained.requestApply();
         require(!staleError.isEmpty() && editor->toPlainText()==stable && retained.hasChanges(),"Stale retained attitude Apply changed the new model");
         angle->setText(old); require(!retained.hasChanges(),"Restored attitude input remained dirty");
      }
      {
         const auto current=editor->toPlainText(); QString error; QWidget owner;
         ResourceEditor fresh(*Moderator::Instance()->GetConfiguredObject("Vehicle"),[&](const auto &changes) {
            error=window.applyResourceChanges("Vehicle",changes,current); return error;
         },&owner,current);
         auto *angle=fresh.findChild<QLineEdit *>("attitude_EulerAngle1"); require(angle,"Fresh attitude input missing"); angle->setText("5"); fresh.requestApply();
         require(error.isEmpty() && !fresh.hasChanges() && std::abs(Moderator::Instance()->GetConfiguredObject("Vehicle")->GetOwnedObject(0)->GetRealParameter("EulerAngle1")-5)<1e-9,"Fresh embedded attitude Apply failed");
         const auto applied=editor->toPlainText(); editor->undo(); require(editor->toPlainText()==current && window.buildScript(),"Embedded attitude Undo lost source");
         editor->redo(); require(editor->toPlainText()==applied && window.buildScript(),"Embedded attitude Redo lost source");
      }
      editor->setPlainText(baseline); require(window.buildScript(),"Retained-form fixture restoration failed");
      std::exception_ptr failure;
      auto creator=[&](const std::function<void(QDialog *)> &check) {
         QTimer::singleShot(0,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("newResourceDialog");
            try { require(dialog,"Unified creator missing"); check(dialog); }
            catch (...) { failure=std::current_exception(); }
            if (dialog && dialog->result()!=QDialog::Accepted) dialog->reject();
         });
         window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure);
      };
      creator([&](QDialog *dialog) {
         require(dialog->width()>=800 && dialog->height()>=650,"Creator still an intermediate small dialog");
         auto *type=dialog->findChild<QComboBox *>("resourceType");
         for (const auto &name:QStringList{"Spacecraft","EphemerisFile","ChemicalThruster","ImpulsiveBurn","GroundStation","OrbitView","GroundTrack","XYPlot","EclipseLocator","DynamicDataDisplay","TrackingFileSet","ElectricTank","DifferentialCorrector","Planet"}) {
            require(type->findText(name)>=0,"Supported type absent"); type->setCurrentText(name);
            bool found=false;
            for (auto *candidate:dialog->findChildren<QWidget *>()) if (auto *resource=dynamic_cast<ResourceEditor *>(candidate);resource && resource->isVisible()) {
               found=true; require(resource->findChild<QTableWidget *>("resourceProperties") && !resource->findChild<QTableWidget *>("resourceProperties")->isVisible(),"Generic property grid exposed as main editor");
               require(!resource->findChild<QDialogButtonBox *>("resourceButtons")->isVisible(),"Duplicate Apply/Close buttons in creator");
            }
            require(found,"Inline draft editor not visible");
            require(editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject(dialog->findChild<QLineEdit *>("resourceName")->placeholderText().toStdString()),"Opening/type switch registered draft or changed source");
         }
         type->setCurrentText("EphemerisFile");
         dialog->findChild<QLineEdit *>("ephemeris_InterpolationOrder")->setText("9"); type->setCurrentText("GroundStation"); type->setCurrentText("EphemerisFile");
         require(dialog->findChild<QLineEdit *>("ephemeris_InterpolationOrder")->text()=="9","Type switch lost pending form values");
      });
      require(editor->toPlainText()==baseline,"Creator Cancel changed source");
      const auto output=files.filePath("trajectory α.bsp");
      creator([&](QDialog *dialog) {
         dialog->findChild<QComboBox *>("resourceType")->setCurrentText("EphemerisFile");
         dialog->findChild<QLineEdit *>("resourceName")->setText("Ephemeris");
         require(dialog->findChild<QLineEdit *>("ephemeris_Filename")->text()=="Ephemeris.oem","Default filename did not follow entered name");
         dialog->findChild<QComboBox *>("ephemeris_FileFormat")->setCurrentText("SPK");
         dialog->findChild<QLineEdit *>("ephemeris_Filename")->setText(output);
         auto *order=dialog->findChild<QLineEdit *>("ephemeris_InterpolationOrder");
         if (argc==3) require(dialog->grab().save(QFileInfo(argv[2]).absoluteFilePath()+".create.png"),"Unified creator capture failed");
         order->setText("11");
         auto *create=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok);
         create->click(); require(dialog->result()!=QDialog::Accepted && editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("Ephemeris"),"Invalid creation partially committed");
         order->setText("7");
         create->click();
         if (dialog->result()!=QDialog::Accepted) std::cerr << dialog->findChild<QLabel *>("resourceCreationStatus")->text().toStdString() << '\n';
         require(dialog->result()==QDialog::Accepted,"Configured ephemeris Create failed");
      });
      auto *object=Moderator::Instance()->GetConfiguredObject("Ephemeris");
      require(object && object->GetStringParameter("Spacecraft")=="Vehicle" && object->GetStringParameter("FileFormat")=="SPK" && QString::fromStdString(object->GetStringParameter("Filename"))==output,"Name and form values not committed together");
      const auto created=editor->toPlainText();
      auto *resource=panel(window,"Ephemeris");
      require(!resource->findChild<QTableWidget *>("resourceProperties")->isVisible() && resource->findChild<QLineEdit *>("ephemeris_Filename")->isVisible(),"Created editor still requires setup dialog");
      require(!resource->hasChanges(),"New primary form dirty without edits");
      if (argc==3) require(resource->grab().save(QFileInfo(argv[2]).absoluteFilePath()+".editor.png"),"Inline editor capture failed");
      auto *order=resource->findChild<QLineEdit *>("ephemeris_InterpolationOrder"); order->setText("11");
      resource->requestApply(); require(editor->toPlainText()==created && resource->hasChanges(),"Invalid inline Apply committed or lost pending data");
      order->setText("9"); resource->requestApply();
      require(Moderator::Instance()->GetConfiguredObject("Ephemeris")->GetIntegerParameter("InterpolationOrder")==9,"Inline Apply ignored primary values");
      const auto edited=editor->toPlainText(); require(edited!=created && !panel(window,"Ephemeris")->hasChanges(),"Applied snapshot not refreshed");
      // Discard pending form edits through the existing MDI close guard.
      resource=panel(window,"Ephemeris"); resource->findChild<QLineEdit *>("ephemeris_Filename")->setText(files.filePath("discarded.bsp"));
      QMdiSubWindow *child=nullptr;
      for (auto *candidate:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (candidate->widget()==resource) child=candidate;
      QTimer::singleShot(0,&window,[] { if (auto *question=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) question->button(QMessageBox::Discard)->click(); });
      require(child,"Discard target missing"); require(child->close(),"Discard did not close panel"); require(editor->toPlainText()==edited,"Close/discard changed applied source"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      editor->undo(); require(editor->toPlainText()==created && window.buildScript(),"Inline edit Undo failed");
      closePanels(window);
      editor->undo(); require(editor->toPlainText()==baseline && window.buildScript() && !Moderator::Instance()->GetConfiguredObject("Ephemeris"),"Creation not one Undo operation");
      editor->redo(); require(editor->toPlainText()==created && window.buildScript(),"Creation Redo failed");
      const auto saved=files.filePath("unified α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==created,"Unicode save/reopen changed source");
      require(window.createResource("ElectricTank","Tank",created).isEmpty(),"Generic tank creation failed");
      const auto afterTank=editor->toPlainText();
      creator([&](QDialog *dialog) {
         dialog->findChild<QComboBox *>("resourceType")->setCurrentText("ChemicalTank");
         dialog->findChild<QLineEdit *>("resourceName")->setText("ConfiguredTank");
         dialog->findChild<QLineEdit *>("resource_FuelMass")->setText("550.5");
         dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok)->click();
         require(dialog->result()==QDialog::Accepted,"Generic inline form Create failed");
      });
      require(Moderator::Instance()->GetConfiguredObject("ConfiguredTank")->GetRealParameter("FuelMass")==550.5,"Generic form value ignored");
      closePanels(window); editor->undo(); require(editor->toPlainText()==afterTank && window.buildScript(),"Generic configured creation Undo failed");
      const auto beforeSpacecraft=editor->toPlainText();
      creator([&](QDialog *dialog) {
         dialog->findChild<QComboBox *>("resourceType")->setCurrentText("Spacecraft"); dialog->findChild<QLineEdit *>("resourceName")->setText("ConfiguredVehicle");
         dialog->findChild<QLineEdit *>("resource_X")->setText("7100");
         dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok)->click();
         if (dialog->result()!=QDialog::Accepted) std::cerr << dialog->findChild<QLabel *>("resourceCreationStatus")->text().toStdString() << '\n';
         require(dialog->result()==QDialog::Accepted,"Configured spacecraft draft failed");
      });
      require(Moderator::Instance()->GetConfiguredObject("ConfiguredVehicle")->GetRealParameter("X")==7100,"Draft orbit state ignored");
      closePanels(window); editor->undo(); require(editor->toPlainText()==beforeSpacecraft && window.buildScript(),"Configured spacecraft not one Undo");
      const auto beforeParameters=editor->toPlainText();
      creator([&](QDialog *dialog) {
         auto *type=dialog->findChild<QComboBox *>("resourceType");
         auto value=[dialog] { for (auto *field:dialog->findChildren<QLineEdit *>("parameterValue")) if (field->isVisible()) return field; return static_cast<QLineEdit *>(nullptr); };
         type->setCurrentText("Variable"); value()->setText("-17.75"); type->setCurrentText("String"); value()->setText("retained α");
         type->setCurrentText("Variable"); require(value()->text()=="-17.75","Numeric draft lost on type switch");
         type->setCurrentText("String"); require(value()->text()=="retained α","String draft lost on type switch");
         dialog->findChild<QLineEdit *>("resourceName")->setText("ConfiguredText");
         dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok)->click();
         require(dialog->result()==QDialog::Accepted,"Inline string creation failed");
      });
      require(Moderator::Instance()->GetConfiguredObject("ConfiguredText")->GetStringParameter("Expression")=="retained α","Inline scalar value ignored");
      closePanels(window); editor->undo(); require(editor->toPlainText()==beforeParameters && window.buildScript(),"String creation not one Undo");
      creator([&](QDialog *dialog) {
         auto *type=dialog->findChild<QComboBox *>("resourceType"); type->setCurrentText("Array");
         dialog->findChild<QLineEdit *>("resourceName")->setText("ConfiguredArray");
         dialog->findChild<QSpinBox *>("arrayRows")->setValue(2); dialog->findChild<QSpinBox *>("arrayColumns")->setValue(3);
         auto *matrix=dialog->findChild<QLineEdit *>("resource_RmatValue"); require(matrix && matrix->text()=="0 0 0; 0 0 0","Dimensions did not resize pending cells");
         matrix->setText("1 2 3; 4 5 6"); type->setCurrentText("GroundStation"); type->setCurrentText("Array");
         require(matrix->text()=="1 2 3; 4 5 6","Type switch lost pending array cells");
         dialog->findChild<QSpinBox *>("arrayColumns")->setValue(2); require(matrix->text()=="1 2; 4 5","Array resize lost retained cells");
         dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok)->click();
         if (dialog->result()!=QDialog::Accepted) std::cerr << dialog->findChild<QLabel *>("resourceCreationStatus")->text().toStdString() << '\n';
         require(dialog->result()==QDialog::Accepted,"Inline configured array failed");
      });
      auto *array=dynamic_cast<Array *>(Moderator::Instance()->GetConfiguredObject("ConfiguredArray"));
      require(array && array->GetIntegerParameter("NumRows")==2 && array->GetIntegerParameter("NumCols")==2 && array->GetRealParameter("SingleValue",1,1)==5,"Pending array size or values ignored");
      closePanels(window); editor->undo(); require(editor->toPlainText()==beforeParameters && window.buildScript(),"Configured array not one Undo");
      creator([&](QDialog *dialog) {
         dialog->findChild<QComboBox *>("resourceType")->setCurrentText("Planet"); dialog->findChild<QLineEdit *>("resourceName")->setText("ConfiguredPlanet");
         auto *radius=dialog->findChild<QLineEdit *>("body_EquatorialRadius"); require(radius,"Draft planet radius missing"); radius->setText("500");
         dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok)->click();
         if (dialog->result()!=QDialog::Accepted) std::cerr << dialog->findChild<QLabel *>("resourceCreationStatus")->text().toStdString() << '\n';
         require(dialog->result()==QDialog::Accepted,"Configured celestial-body draft failed");
      });
      require(Moderator::Instance()->GetConfiguredObject("ConfiguredPlanet")->GetRealParameter("EquatorialRadius")==500,"Draft body settings ignored");
      closePanels(window); editor->undo(); require(editor->toPlainText()==beforeParameters && window.buildScript(),"Configured body not one Undo");
      // Blank and whitespace names use collision-free suggestions; explicit names stay untouched.
      require(window.createResource("GroundStation","GroundStation1",editor->toPlainText()).isEmpty(),"Automatic-name collision fixture failed");
      const auto beforeAutomatic=editor->toPlainText();
      creator([&](QDialog *dialog) {
         auto *type=dialog->findChild<QComboBox *>("resourceType"); auto *name=dialog->findChild<QLineEdit *>("resourceName");
         require(name->text().isEmpty() && name->placeholderText().startsWith("Spacecraft"),"Initial suggested name missing");
         type->setCurrentText("GroundStation"); require(name->text().isEmpty() && name->placeholderText()=="GroundStation2","Suggestion ignored type or collision");
         name->setText("ManualName"); type->setCurrentText("ChemicalTank"); require(name->text()=="ManualName","Type switch replaced explicit name");
         type->setCurrentText("GroundStation"); name->setText("   ");
         dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok)->click();
         require(dialog->result()==QDialog::Accepted && name->text()=="GroundStation2","Blank name was not assigned at Create");
      });
      require(Moderator::Instance()->GetConfiguredObject("GroundStation1") && Moderator::Instance()->GetConfiguredObject("GroundStation2"),"Automatic name replaced existing resource");
      closePanels(window); editor->undo(); require(editor->toPlainText()==beforeAutomatic && window.buildScript() && !Moderator::Instance()->GetConfiguredObject("GroundStation2"),"Automatic-name creation not one Undo");
      std::cout << "PASS: one large name/type/configuration creator, 14 unregistered form drafts, hidden Advanced grid, pending type-switch retention, Cancel and invalid creation atomicity, configured ephemeris/tank/spacecraft/string/array/planet creation in one transaction, pending array resize, direct form Apply validation and Close/discard, optional names with type-based collision avoidance and explicit-name retention, Undo/Redo/Unicode Save/reopen. Offscreen only; no mission.\n";
      return 0;
   } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
