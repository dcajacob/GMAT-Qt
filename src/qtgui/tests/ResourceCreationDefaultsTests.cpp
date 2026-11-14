#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "AcceptanceView.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTreeWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidgetItemIterator>
#include <QMenu>
#include <QKeyEvent>
#include <QPointer>
#include <QTextDocument>
#include <QSet>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QMap<QString,QPair<quintptr,QString>> configuredResources()
{
   auto *moderator=Moderator::Instance();
   StringArray names=moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT);
   const StringArray bodies=moderator->GetListOfObjects(Gmat::CELESTIAL_BODY);
   names.insert(names.end(),bodies.begin(),bodies.end()); names.push_back("SolarSystem");
   QMap<QString,QPair<quintptr,QString>> result;
   for (const auto &name:names) {
      auto *object=moderator->GetConfiguredObject(name);
      require(object,"Configured resource disappeared during menu-route check");
      result.insert(QString::fromStdString(name),{reinterpret_cast<quintptr>(object),QString::fromStdString(object->GetGeneratingString(Gmat::SHOW_SCRIPT))});
   }
   return result;
}
template<class Inspect> static void resourceMenu(MainWindow &window,QTreeWidget &tree,QTreeWidgetItem &category,Inspect inspect)
{
   tree.expandAll(); tree.scrollToItem(&category); tree.setCurrentItem(&category);
   acceptanceView(tree); QApplication::processEvents();
   std::exception_ptr failure; bool inspected=false; QTimer timer; timer.setSingleShot(true);
   QObject::connect(&timer,&QTimer::timeout,&window,[&] {
      auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
      try { require(menu && menu->objectName()=="resourceContextMenu","Resource context popup missing"); inspected=true; inspect(*menu); }
      catch (...) { failure=std::current_exception(); if (menu) menu->close(); }
   });
   timer.start(qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible" ? 120 : 0); tree.customContextMenuRequested(tree.visualItemRect(&category).center()); timer.stop();
   if (failure) std::rethrow_exception(failure); require(inspected,"Resource context menu was not inspected");
}
static void resourceAddRoutes(MainWindow &window,QPlainTextEdit &editor)
{
   auto *tree=window.findChild<QTreeWidget *>("Resources"); require(tree,"Resources tree missing");
   const auto original=editor.toPlainText(); const auto objects=configuredResources();
   const auto undo=editor.document()->availableUndoSteps(); const auto modified=editor.document()->isModified();
   QList<QTreeWidgetItem *> categories;
   for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole+1).isValid()) categories.append(*it);
   require(!categories.isEmpty(),"No typed resource categories found"); QSet<QString> types; int actions=0;
   for (auto *category:categories) {
      QStringList offered;
      resourceMenu(window,*tree,*category,[&](QMenu &menu) {
         for (auto *action:menu.actions()) if (action->objectName().startsWith("addResource_") && action->isEnabled()) {
            const auto type=action->data().toString(); require(!type.isEmpty(),"Add action has no resource type");
            require(action->objectName()=="addResource_"+type && action->text()=="Add "+type+"…","Add label, name and target type disagree"); offered.append(type);
         }
         menu.close();
      });
      for (const auto &type:offered) {
         bool creatorSeen=false; std::exception_ptr failure; QPointer<QMenu> oldPopup;
         QTimer inspect; inspect.setSingleShot(true);
         QObject::connect(&inspect,&QTimer::timeout,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("newResourceDialog");
            try {
               require(oldPopup.isNull() && !QApplication::activePopupWidget(),"Resource popup survived into the creation dialog");
               require(dialog && dialog->isVisible(),"Add action did not open the creation dialog"); creatorSeen=true;
               auto *choice=dialog->findChild<QComboBox *>("resourceType");
               require(choice && choice->currentText()==type,qPrintable("Wrong creator type for Add "+type));
               require(editor.toPlainText()==original && configuredResources()==objects,"Opening Add changed the mission before acceptance");
               auto *buttons=dialog->findChild<QDialogButtonBox *>(); auto *cancel=buttons ? buttons->button(QDialogButtonBox::Cancel) : nullptr;
               require(cancel && cancel->isEnabled(),"Creation Cancel button missing"); cancel->click();
            } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
         });
         resourceMenu(window,*tree,*category,[&](QMenu &menu) {
            oldPopup=&menu; QAction *chosen=nullptr;
            for (auto *action:menu.actions()) if (action->objectName()=="addResource_"+type) chosen=action;
            require(chosen && chosen->isEnabled(),"Offered Add action disappeared");
            inspect.start(qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible" ? 120 : 0); menu.setActiveAction(chosen);
            QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(&menu,&enter);
         });
         inspect.stop(); if (failure) std::rethrow_exception(failure);
         require(creatorSeen,"Add action returned without opening its resource creator");
         QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
         require(editor.toPlainText()==original && editor.document()->availableUndoSteps()==undo && editor.document()->isModified()==modified && configuredResources()==objects,"Cancel from resource-tree Add changed source, history, dirty state or configured objects");
         types.insert(type); ++actions;
         const QJsonObject proof{{"type",type},{"category",category->text(0)},{"outcome","passed"},{"checks",QJsonArray{"context-menu-action","preset-type","popup-retired","cancel","unchanged-model-source-history"}}};
         std::cout<<"GUI_ROUTE_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      }
   }
   for (const auto &type:QStringList{"Spacecraft","Variable","Planet","ImpulsiveBurn","ReportFile"}) require(types.contains(type),qPrintable("Required Add route was not exercised: "+type));
   std::cout<<"Resource-tree Add routes: "<<actions<<" menu actions across "<<types.size()<<" types passed preset/Cancel invariants.\n";
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   if (!acceptanceWidgetPlatform() || argc<2 || argc>3) return 2;
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("ResourceCreationDefaults");
   const bool starProbe=argc==3 && QString::fromLocal8Bit(argv[2])=="--star";
   const bool probe=starProbe || (argc==3 && QString::fromLocal8Bit(argv[2])=="--probe");
   try {
      TestSettings settings;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime unavailable"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString baseline="% Preserve source and implicit defaults α\nCreate Spacecraft Vehicle;\nCreate Variable Keep;\nKeep = 7; % retained comment\nBeginMissionSequence;\nKeep = Keep + 2;\n";
      editor->setPlainText(baseline); require(window.buildScript(),"Baseline build failed");
      QStringList types;
      QTimer::singleShot(0,&window,[&] {
         auto *dialog=window.findChild<QDialog *>("newResourceDialog");
         if (!dialog) return;
         auto *choice=dialog->findChild<QComboBox *>("resourceType");
         for (int i=0;i<choice->count();++i) types.append(choice->itemText(i));
         dialog->reject();
      });
      window.findChild<QAction *>("createResource")->trigger(); require(!types.isEmpty(),"Creator list unavailable");
      if (!probe) {
         require(!types.contains("Star"),"Uncreatable Star offered in creator");
         const auto blocked=window.createResource("Star","UnsupportedStar",baseline);
         require(!blocked.isEmpty() && editor->toPlainText()==baseline && Moderator::Instance()->GetConfiguredObject("Vehicle"),"Unsupported Star mutated mission");
      }
      // The prior 18-type plugin matrix and user-parameter suites are already qualified.
      const QStringList previous{"Formation","GroundStation","Smoother","ProcessNoiseModel","EstimatedParameter","ExtendedKalmanFilter","Simulator","BatchEstimator","TrackingFileSet","ErrorModel","AcceptFilter","RejectFilter","FileInterface","GmatFunction","Yukon","ThrustHistoryFile","ThrustSegment","EclipseLocator","Variable","String","Array"};
      int checked=0; QStringList failed;
      for (const auto &type:types) {
         if (previous.contains(type)) continue;
         if (starProbe && type!="Star") continue;
         if (!probe && type!="EphemerisFile") continue;
         const auto name="New"+type;
         const auto error=window.createResource(type,name,baseline);
         ++checked;
         if (!error.isEmpty()) {
            failed.append(type); std::cout << "FAIL " << type.toStdString() << ": " << error.toStdString() << '\n';
            std::cout << window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString() << '\n';
            require(editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject(name.toStdString()),"Failed creation left source/model mutation");
         } else {
            require(Moderator::Instance()->GetConfiguredObject(name.toStdString()) && editor->toPlainText().endsWith(baseline),"Creation lost resource/source");
            std::cout << "PASS create " << type.toStdString() << '\n';
            editor->undo(); require(editor->toPlainText()==baseline && window.buildScript(),"Creation Undo/rebuild failed");
         }
      }
      std::cout << "Checked " << checked << " previously untested creator types; failures: " << failed.join(", ").toStdString() << '\n';
      if (!probe) require(failed.isEmpty(),"Resource default creation failures remain");
      if (!probe) {
         QTemporaryDir files; require(files.isValid(),"Temporary directory unavailable");
         auto *tree=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace");
         const auto noSpacecraft=QString("% No spacecraft yet α\nCreate Variable Keep;\nKeep = 7;\nBeginMissionSequence;\nKeep = Keep + 2;\n");
         editor->setPlainText(noSpacecraft); require(window.buildScript(),"Empty-spacecraft baseline failed");
         const auto error=window.createResource("EphemerisFile","Ephemeris",noSpacecraft);
         require(error.contains("Create a Spacecraft") && editor->toPlainText()==noSpacecraft && !Moderator::Instance()->GetConfiguredObject("Ephemeris"),"Missing spacecraft not rejected before transaction");
         std::exception_ptr failure;
         QTimer::singleShot(0,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("newResourceDialog");
            try {
               require(dialog,"Empty-spacecraft dialog missing"); dialog->findChild<QComboBox *>("resourceType")->setCurrentText("EphemerisFile");
               auto *choice=dialog->findChild<QComboBox *>("ephemeris_Spacecraft"); require(choice->isVisible() && choice->count()==0,"Missing spacecraft selector state");
               dialog->findChild<QLineEdit *>("resourceName")->setText("Ephemeris");
               dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
               require(dialog->result()!=QDialog::Accepted && dialog->findChild<QLabel *>("resourceCreationStatus")->text().contains("Create a Spacecraft") && editor->toPlainText()==noSpacecraft,"Missing spacecraft error not correctable");
            } catch (...) { failure=std::current_exception(); }
            if (dialog) dialog->reject();
         });
         window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure);
         const auto multiple=QString("Create Spacecraft Another;\n")+baseline;
         editor->setPlainText(multiple); require(window.buildScript(),"Multiple-spacecraft baseline failed");
         require(!window.createResource("EphemerisFile","Invalid",multiple,1,1,std::nullopt,{},"Keep").isEmpty() && editor->toPlainText()==multiple,"Wrong-type spacecraft accepted");
         bool accepted=false;
         auto creator=[&](bool commit) {
            QTimer::singleShot(0,&window,[&] {
               auto *dialog=window.findChild<QDialog *>("newResourceDialog");
               try {
                  require(dialog,"Ephemeris creator missing"); auto *type=dialog->findChild<QComboBox *>("resourceType"); type->setCurrentText("EphemerisFile");
                  auto *choice=dialog->findChild<QComboBox *>("ephemeris_Spacecraft"); require(choice->isVisible() && choice->count()==2 && choice->findText("Vehicle")>=0 && choice->findText("Another")>=0,"Spacecraft choices incorrect");
                  choice->setCurrentText("Vehicle"); type->setCurrentText("ReportFile"); require(!choice->isVisible(),"Spacecraft chooser leaked into other types");
                  type->setCurrentText("EphemerisFile"); require(choice->currentText()=="Vehicle","Type switch lost pending selection");
                  choice->setCurrentText("Another"); dialog->findChild<QLineEdit *>("resourceName")->setText("Ephemeris");
                  if (commit) { dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); accepted=dialog->result()==QDialog::Accepted; }
               } catch (...) { failure=std::current_exception(); }
               if (dialog && (!commit || !accepted)) dialog->reject();
            });
            window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure);
         };
         creator(false); require(editor->toPlainText()==multiple && !Moderator::Instance()->GetConfiguredObject("Ephemeris"),"Cancel mutated mission");
         creator(true);
         const auto created=editor->toPlainText();
         auto *object=Moderator::Instance()->GetConfiguredObject("Ephemeris");
         require(accepted && object,"Configured Ephemeris creator did not accept");
         require(object->GetStringParameter("Spacecraft")=="Another","Selected spacecraft not committed");
         require(created.contains("Create EphemerisFile Ephemeris;"),"Ephemeris declaration absent");
         require(created.endsWith(multiple),"Unrelated creation source changed");
         require(tree->findItems("Ephemeris",Qt::MatchExactly|Qt::MatchRecursive).size()==1,"Ephemeris tree entry missing");
         require(area->activeSubWindow() && area->activeSubWindow()->property("resourceName").toString()=="Ephemeris","Ephemeris editor did not open");
         for (auto *child:area->subWindowList()) if (child->property("resourceName").toString()=="Ephemeris") child->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
         editor->undo(); require(editor->toPlainText()==multiple && window.buildScript() && !Moderator::Instance()->GetConfiguredObject("Ephemeris"),"Ephemeris Undo failed");
         editor->redo(); require(editor->toPlainText()==created && window.buildScript() && Moderator::Instance()->GetConfiguredObject("Ephemeris")->GetStringParameter("Spacecraft")=="Another","Ephemeris Redo failed");
         const auto saved=files.filePath("ephemeris α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==created && Moderator::Instance()->GetConfiguredObject("Ephemeris")->GetStringParameter("Spacecraft")=="Another","Ephemeris Unicode Save/reopen failed");
         std::cout << "PASS: required spacecraft selection/default, missing/wrong-type correction before transaction, pending selection and Cancel, selected spacecraft/source/tree/editor, Undo/Redo/Unicode Save/reopen; uncreatable Star not offered. No mission or old plugin matrix.\n";
         resourceAddRoutes(window,*editor);
      }
      return 0;
   } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
