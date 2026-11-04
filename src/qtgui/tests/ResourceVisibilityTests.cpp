#include "AcceptanceView.hpp"
#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void checkProperties(GmatBase &object,ResourceEditor &panel)
{
   const QStringList internal{"UpperLeft","Size","RelativeZOrder","Minimized","Maximized","TargetStatus"};
   for (const auto &field:resourceProperties(object)) require(!internal.contains(field.name),"Internal subscriber property exposed");
   auto *table=panel.findChild<QTableWidget *>("resourceProperties"); require(table,"Property backing table absent");
   for (int row=0;row<table->rowCount();++row) require(!internal.contains(table->item(row,0)->text()),"Internal property in resource editor");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   if (!acceptanceWidgetPlatform() || argc<2 || argc>3) return 2;
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("ResourceVisibility");
   try {
      TestSettings settings; QTemporaryDir files;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString layout="Orb.UpperLeft = [0.15 0.25]; % keep position α\nOrb.Size = [0.65 0.45]; % keep size\nOrb.RelativeZOrder = 3; % keep order\nOrb.Maximized = true; % keep state\n";
      const QString source="Create Spacecraft Vehicle;\nCreate OrbitView Orb;\nOrb.Add = {Vehicle, Earth};\nOrb.ShowPlot = false;\n"+layout+
         "Create GroundTrack Ground;\nCreate XYPlot XY;\nCreate ReportFile OutputReport;\nCreate EphemerisFile Export;\nExport.Spacecraft = Vehicle;\nCreate DynamicDataDisplay Data;\nBeginMissionSequence;\n";
      editor->setPlainText(source); require(window.buildScript(),"Visibility fixture failed");
      acceptanceView(window);
      auto *tree=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace");
      for (const auto &name:QStringList{"Orb","Ground","XY","OutputReport","Export","Data"}) {
         const auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Resource tree entry missing"); tree->itemDoubleClicked(items.first(),0);
         auto *child=area->activeSubWindow(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Resource editor missing");
         checkProperties(*Moderator::Instance()->GetConfiguredObject(name.toStdString()),*panel);
      acceptanceView(*panel);
         if (name=="Orb") {
            auto *tabs=panel->findChild<QTabWidget *>("resourceEditorTabs");
            for (int tab=0;tab<tabs->count();++tab) if (tabs->tabText(tab)=="Advanced") tabs->setCurrentIndex(tab);
            require(panel->findChild<QPushButton *>("editOrbitDrawing")->isVisible(),"Legitimate object drawing action hidden");
            require(!panel->findChild<QTableWidget *>("resourceProperties")->isVisible() && !panel->findChild<QWidget *>("propertyFilter")->isVisible(),"Empty Advanced property grid retained");
            if (argc==3) require(panel->grab().save(QFileInfo(argv[2]).absoluteFilePath()+".editor.png"),"Editor capture failed");
         }
         require(!panel->hasChanges() && editor->toPlainText()==source,"Opening panels changed source"); child->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      }
      std::exception_ptr failure;
      QTimer::singleShot(0,&window,[&] {
         auto *dialog=window.findChild<QDialog *>("newResourceDialog");
         try {
            auto *type=dialog->findChild<QComboBox *>("resourceType");
            for (const auto &name:QStringList{"OrbitView","GroundTrack","XYPlot"}) {
               type->setCurrentText(name); ResourceEditor *panel=nullptr;
               for (auto *widget:dialog->findChildren<QWidget *>()) if (auto *candidate=dynamic_cast<ResourceEditor *>(widget);candidate && candidate->isVisible()) panel=candidate;
               require(panel,"Creator draft missing"); auto *grid=panel->findChild<QTableWidget *>("resourceProperties");
               for (int row=0;row<grid->rowCount();++row) require(!QStringList{"UpperLeft","Size","RelativeZOrder","Maximized"}.contains(grid->item(row,0)->text()),"Creator exposes window state");
            }
            type->setCurrentText("OrbitView"); QTabWidget *tabs=nullptr;
            for (auto *widget:dialog->findChildren<QWidget *>()) if (auto *candidate=dynamic_cast<ResourceEditor *>(widget);candidate && candidate->isVisible()) tabs=candidate->findChild<QTabWidget *>("resourceEditorTabs");
            for (int tab=0;tab<tabs->count();++tab) if (tabs->tabText(tab)=="Advanced") tabs->setCurrentIndex(tab);
            if (argc==3) require(dialog->grab().save(QFileInfo(argv[2]).absoluteFilePath()+".create.png"),"Creator capture failed");
         } catch (...) { failure=std::current_exception(); }
         if (dialog) dialog->reject();
      });
      window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure);
      require(editor->toPlainText()==source,"Creator Cancel changed source");
      require(window.applyResourceChanges("Orb",{{"ShowPlot","true"}},source).isEmpty(),"Legitimate output edit rejected");
      const auto edited=editor->toPlainText(); require(edited.contains(layout),"Hidden window assignments/comments changed on Apply");
      editor->undo(); require(editor->toPlainText()==source && window.buildScript(),"Visibility edit Undo changed layout");
      editor->redo(); require(editor->toPlainText()==edited && window.buildScript(),"Visibility edit Redo changed layout");
      const auto saved=files.filePath("visibility α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==edited,"Visibility Save/reopen changed source");
      std::cout << "PASS: hidden subscriber metadata absent in six output editors and three plot creators; object drawing retained, empty Advanced grid removed; saved layout/comments preserved on Apply, Undo/Redo and Unicode Save/reopen. Offscreen only; no mission.\n";
      return 0;
   } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
