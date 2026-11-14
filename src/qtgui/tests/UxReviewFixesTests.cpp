#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "ResourceEditor.hpp"
#include "SatelliteImportDialog.hpp"
#include "GroundStationDialog.hpp"
#include "MatrixTableWidget.hpp"
#include "PlotWidget.hpp"
#include "QtPlotReceiver.hpp"
#include "Moderator.hpp"
#include "StandardIcons.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}
void events() { QApplication::processEvents(); QTest::qWait(20); }
void capture(QWidget &widget,const QString &name)
{
   const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   if (directory.isEmpty()) return;
   QDir().mkpath(directory); events();
   require(widget.grab().save(directory+'/'+name+".png"),"Cannot save UX check screenshot");
}

ResourceEditor *openResource(MainWindow &window,const QString &name)
{
   auto *tree=window.findChild<QTreeWidget *>("Resources");
   const auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);
   require(items.size()==1,"Expected resource is missing");
   tree->setCurrentItem(items.first()); tree->itemDoubleClicked(items.first(),0); events();
   auto *area=window.findChild<QMdiArea *>("workspace");
   auto *child=area->activeSubWindow();
   auto *panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr;
   require(panel,"Resource editor did not open"); child->showMaximized(); events();
   return panel;
}

void checkModelState(MainWindow &window,const QString &directory)
{
   auto *banner=window.findChild<QWidget *>("modelBuildStatus");
   auto *message=window.findChild<QLabel *>("modelBuildMessage");
   auto *build=window.findChild<QPushButton *>("modelBuildButton");
   auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
   auto *resources=window.findChild<QTreeWidget *>("Resources");
   require(banner && message && build && editor && resources,"Build-state controls are missing");
   require(!banner->isVisible(),"Current built model shows an out-of-date notice");
   const auto path=directory+"/ModelState.script";
   QFile file(path); require(file.open(QIODevice::WriteOnly),"Cannot create model-state fixture");
   file.write("Create Spacecraft AuditSat;\nBeginMissionSequence;\n"); file.close();
   require(window.loadScript(path),"Cannot load state fixture"); events();
   require(banner->isVisible() && message->text().contains("last build"),"Loaded script lacks a clear last-build notice");
   capture(window,"02-loaded-build-status");
   require(resources->findItems("AuditSat",Qt::MatchExactly|Qt::MatchRecursive).isEmpty(),"Fixture was built implicitly");
   QTest::mouseClick(build,Qt::LeftButton); events();
   require(!banner->isVisible() && resources->findItems("AuditSat",Qt::MatchExactly|Qt::MatchRecursive).size()==1,
           "Build action did not synchronize the resource model and notice");
   auto cursor=editor->textCursor(); cursor.movePosition(QTextCursor::End); cursor.insertText("% changed\n"); events();
   require(banner->isVisible(),"Editing the built source lacks a rebuild notice");
   editor->undo(); events(); require(!banner->isVisible(),"Undo to the built source left a stale notice");
   cursor=editor->textCursor(); cursor.movePosition(QTextCursor::End); cursor.insertText("InvalidCommandForUxTest;\n");
   QTest::mouseClick(build,Qt::LeftButton); events();
   require(banner->isVisible() && message->text().contains("populate"),"Failed build lost its persistent rebuild notice");
   require(editor->toPlainText().contains("InvalidCommandForUxTest"),"Failed build changed the user's source");
   editor->undo(); require(window.buildScript(),"Cannot recover the valid source"); events();
   require(!banner->isVisible(),"Successful recovery left a stale rebuild notice");
   std::cout<<"PASS loaded, edited, undone, failed and recovered build-state feedback\n";
}

void checkImportDiscovery(MainWindow &window)
{
   bool buttonFound=false,importOpened=false,draftRetained=false,typeVisibility=false;
   QTimer::singleShot(0,&window,[&] {
      auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
      if (!dialog) return;
      auto *button=dialog->findChild<QPushButton *>("newSpacecraftImport");
      auto *name=dialog->findChild<QLineEdit *>("resourceName");
      auto *type=dialog->findChild<QComboBox *>("resourceType");
      buttonFound=button && button->isVisible();
      if (buttonFound && name && type) {
         name->setText("DraftSat");
         capture(*dialog,"03-new-spacecraft-import");
         QTimer::singleShot(0,&window,[&] {
            auto *import=qobject_cast<QDialog *>(QApplication::activeModalWidget());
            importOpened=import && import->objectName()=="satelliteImportDialog";
            if (import) import->reject();
         });
         button->click();
         draftRetained=dialog->isVisible() && name->text()=="DraftSat";
         type->setCurrentText("Array"); events();
         typeVisibility=!button->isVisible();
         type->setCurrentText("Spacecraft"); events();
         typeVisibility=typeVisibility && button->isVisible();
      }
      dialog->reject();
   });
   window.findChild<QAction *>("createResource")->trigger();
   require(buttonFound && importOpened && draftRetained && typeVisibility,"Spacecraft import discovery or cancelled-draft recovery failed");
   auto *resources=window.findChild<QTreeWidget *>("Resources");
   const auto rows=resources->findItems("Spacecraft",Qt::MatchExactly|Qt::MatchRecursive);
   require(rows.size()==1,"Spacecraft resource category is missing");
   resources->scrollToItem(rows.first()); events();
   bool contextFound=false;
   QTimer::singleShot(0,&window,[&] {
      auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
      if (!menu) return;
      for (auto *action:menu->actions()) if (action->objectName()=="importSatelliteFromSpacecraft")
         contextFound=action->isEnabled();
      menu->close();
   });
   resources->customContextMenuRequested(resources->visualItemRect(rows.first()).center());
   require(contextFound,"Spacecraft category lacks its import entry");
   std::cout<<"PASS import entry in spacecraft creation/category and preserved draft after cancellation\n";
}

void checkSeries(MainWindow &window)
{
   auto &receiver=*window.plotReceiver();
   receiver.CreateXyPlotWindow("UxSeries","",0,0,0,0,false,"Series defaults","Time","Value",true);
   receiver.AddXyPlotCurve("UxSeries",0,"First",0);
   receiver.AddXyPlotCurve("UxSeries",1,"Second",0);
   receiver.AddXyPlotCurve("UxSeries",2,"Third",0x009e73);
   receiver.XyPlotCurveSettings("UxSeries",true,1,100,false,4,-1,false,-1);
   const auto model=receiver.model("UxSeries");
   require(model->curves[0].lineStyle!=model->curves[1].lineStyle && model->curves[1].lineStyle!=model->curves[2].lineStyle,
           "Default XY series do not have distinct patterns");
   require(model->curves[0].color==QColor(Qt::black) && model->curves[2].color==QColor(0,158,115),"Configured series colors changed");
   for (int sample=0;sample<20;++sample) for (int curve=0;curve<3;++curve)
      receiver.UpdateXyPlotCurve("UxSeries",curve,sample,sample*(curve+1));
   events(); capture(window,"04-series-defaults");
   receiver.XyPlotChangeStyle("UxSeries",0,100,1);
   require(model->curves[1].lineStyle==100,"Explicit solid-line selection was overridden");
   receiver.XyPlotChangeColor("UxSeries",0,0xff0000,1);
   require(model->curves[1].color==QColor(Qt::red) && model->curves[1].points.back().y==38,"Explicit color or retained data changed");
   std::cout<<"PASS distinct default series patterns, configured colors and explicit overrides\n";
}

void checkImportPalette(MainWindow &window)
{
   const auto original=QApplication::palette();
   SatelliteImportDialog dialog({"EarthMJ2000Eq"},{"Earth"},[](const auto &,const auto &) { return QString(); },&window);
   dialog.show();
   for (bool dark:{true,false}) {
      auto colors=original;
      for (auto group:{QPalette::Active,QPalette::Inactive,QPalette::Disabled}) {
         for (auto role:{QPalette::Window,QPalette::Button}) colors.setColor(group,role,dark ? QColor(40,40,40) : QColor(245,245,245));
         colors.setColor(group,QPalette::Base,dark ? QColor(30,30,30) : QColor(255,255,255));
         for (auto role:{QPalette::Text,QPalette::WindowText,QPalette::ButtonText})
            colors.setColor(group,role,dark ? QColor(235,235,235) : QColor(20,20,20));
      }
      colors.setColor(QPalette::Link,Qt::blue); colors.setColor(QPalette::LinkVisited,QColor(100,0,150));
      colors.setColor(QPalette::PlaceholderText,dark ? QColor(40,40,40) : QColor(220,220,220));
      QApplication::setPalette(colors); events();
      auto *credit=dialog.findChild<QLabel *>("satelliteCatalogCredits");
      auto *query=dialog.findChild<QLineEdit *>("satelliteCatalogQuery");
      require(credit && query,"Catalog labels are missing");
      for (auto role:{QPalette::Link,QPalette::LinkVisited})
         require(GmatQtIcons::contrast(credit->palette().color(role),colors.color(QPalette::Window))>=6.99,"Catalog link contrast is insufficient");
      require(GmatQtIcons::contrast(query->palette().color(QPalette::PlaceholderText),query->palette().color(QPalette::Base))>=4.49,
              "Catalog placeholder contrast is insufficient");
      const auto image=credit->grab().toImage(); const auto link=credit->palette().color(QPalette::Link);
      int painted=0;
      for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) {
         const auto pixel=image.pixelColor(x,y);
         if (std::abs(pixel.red()-link.red())<4 && std::abs(pixel.green()-link.green())<4 && std::abs(pixel.blue()-link.blue())<4)
            ++painted;
      }
      require(painted>4,"Rendered catalog links ignore the readable theme color");
      capture(dialog,dark ? "05-catalog-dark" : "05-catalog-light");
   }
   dialog.close(); QApplication::setPalette(original);
   std::cout<<"PASS catalog link, visited-link and placeholder contrast across live theme changes\n";
}

void checkPanelActions(MainWindow &window)
{
   const auto names=Moderator::Instance()->GetListOfObjects(Gmat::SPACECRAFT);
   require(!names.empty(),"No spacecraft fixture exists"); const auto name=QString::fromStdString(names.front());
   auto *object=Moderator::Instance()->GetConfiguredObject(names.front()); const double original=object->GetRealParameter("X");
   auto *panel=openResource(window,name); auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");
   require(buttons && buttons->button(QDialogButtonBox::Ok)->text()=="Close", "Clean panel lacks a simple Close action");
   require(!buttons->button(QDialogButtonBox::Close)->isVisible() && !buttons->button(QDialogButtonBox::Cancel)->isVisible(),
           "Clean panel still shows redundant completion actions");
   auto *value=panel->findChild<QLineEdit *>("resource_X"); require(value,"Spacecraft X field is missing");
   value->setText(QString::number(original+1,'g',17)); events();
   require(buttons->button(QDialogButtonBox::Ok)->text()=="Apply and close" && buttons->button(QDialogButtonBox::Cancel)->isVisible(),
           "Pending panel does not explain apply/discard completion");
   capture(window,"06-panel-pending-actions");
   buttons->button(QDialogButtonBox::Cancel)->click(); events();
   require(Moderator::Instance()->GetConfiguredObject(names.front())->GetRealParameter("X")==original,"Discard committed a draft");
   panel=openResource(window,name); buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");
   panel->findChild<QLineEdit *>("resource_X")->setText(QString::number(original+1,'g',17));
   QPointer<QWidget> owner=panel->parentWidget(); buttons->button(QDialogButtonBox::Ok)->click(); events();
   require((!owner || owner->isHidden()) && Moderator::Instance()->GetConfiguredObject(names.front())->GetRealParameter("X")==original+1,
           "Apply and close did not commit and close the correct resource");
   std::cout<<"PASS clean/pending action labels, discard and apply-and-close transactions\n";
}

void fillState(SatelliteImportDialog &dialog,const QString &name)
{
   dialog.findChild<QTabWidget *>("satelliteImportSource")->setCurrentIndex(2);
   dialog.findChild<QLineEdit *>("importSatelliteName")->setText(name);
   dialog.findChild<QLineEdit *>("satelliteStateEpoch")->setText("2026-11-13T12:00:00Z");
   const QStringList fields={"X","Y","Z","VX","VY","VZ"};
   const QStringList values={"7000","0","0","0","7.5","0"};
   for (int i=0;i<fields.size();++i) dialog.findChild<QLineEdit *>("satelliteState"+fields[i])->setText(values[i]);
}

void checkImportReadiness(MainWindow &window)
{
   bool applied=false;
   SatelliteImportDialog dialog({"EarthMJ2000Eq"},{"Earth"},[&](const auto &data,const auto &name) {
      applied=data.state.has_value() && name=="UxState"; return QString();
   },&window);
   dialog.show(); events(); auto *import=dialog.findChild<QPushButton *>("importSatellite");
   require(import && !import->isEnabled(),"Empty catalog enables Import");
   fillState(dialog,"UxState"); events(); require(import->isEnabled(),"Complete valid state does not enable Import");
   auto *x=dialog.findChild<QLineEdit *>("satelliteStateX"); x->setText("nan"); events();
   require(!import->isEnabled() && !dialog.findChild<QLabel *>("satelliteImportGuidance")->text().isEmpty(),"Invalid state lacks readiness guidance");
   x->setText("7000"); import->click(); require(applied && dialog.result()==QDialog::Accepted,"Ready state did not import");
   bool importedThroughCreation=false;
   QTimer::singleShot(0,&window,[&] {
      auto *creation=qobject_cast<QDialog *>(QApplication::activeModalWidget());
      if (!creation) return;
      QTimer::singleShot(0,&window,[&] {
         auto *dialog=dynamic_cast<SatelliteImportDialog *>(QApplication::activeModalWidget());
         if (!dialog) return;
         fillState(*dialog,"UxImported");
         dialog->findChild<QPushButton *>("importSatellite")->click();
         importedThroughCreation=dialog->result()==QDialog::Accepted;
      });
      creation->findChild<QPushButton *>("newSpacecraftImport")->click();
      if (!importedThroughCreation) creation->reject();
   });
   window.findChild<QAction *>("createResource")->trigger(); events();
   require(importedThroughCreation && Moderator::Instance()->GetConfiguredObject("UxImported"),"New Spacecraft import did not create its resource");
   std::cout<<"PASS empty/invalid/ready import states and successful import through New Spacecraft\n";
}

void checkPlaybackScope(MainWindow &window)
{
   auto &receiver=*window.plotReceiver();
   receiver.CreateXyPlotWindow("ScopeData","",0,0,0,0,false,"Scope","Time","Value",true);
   receiver.AddXyPlotCurve("ScopeData",0,"First",0x0072b2);
   receiver.UpdateXyPlotCurve("ScopeData",0,0,0); receiver.UpdateXyPlotCurve("ScopeData",0,1,1);
   require(window.findChild<QLabel *>("allPlotPlaybackScope")->text()=="All plots","Global playback scope is missing");
   receiver.CreateGlPlotWindow("UxScope","",0,0,0,0,false,0); events();
   auto *area=window.findChild<QMdiArea *>("workspace");
   auto *view=area->activeSubWindow() ? dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget()) : nullptr;
   require(view,"Playback scope view did not open");
   auto *scope=view->findChild<QLabel *>("plotPlaybackScope"); require(scope && scope->text()=="This view","Local playback scope is missing");
   receiver.setReplayPosition(500); require(scope->text()=="All plots","Linked plot scope is unclear");
   view->findChild<QAction *>("plotReplayStart")->trigger(); events();
   require(scope->text()=="This view","Independent plot scope did not reset");
   require(window.findChild<QLabel *>("allPlotReplayPosition")->text()=="Independent","Global independent state is unclear");
   capture(*view,"08-playback-scope");
   std::cout<<"PASS all-plot, linked and independent playback scope labels\n";
}

void checkCoordinates(MainWindow &window)
{
   auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
   require(window.createResource("GroundStation","UxStation",editor->toPlainText()).isEmpty(),"Cannot create station fixture");
   auto *station=Moderator::Instance()->GetConfiguredObject("UxStation");
   station->SetStringParameter("StateType","Spherical");
   station->SetRealParameter("Location1",39.741129999999998); station->SetRealParameter("Location2",255.01404000000002);
   GroundStationDialog dialog(*station,{},&window); dialog.show(); events();
   const auto initial=dialog.settings(); auto *longitude=dialog.findChild<QLineEdit *>("station_Location2");
   require(longitude && longitude->text()==initial.value("Location2"),"Coordinate display changed its exact edit value");
   require(longitude->toolTip().contains(longitude->text()),"Coordinate exact-value hint is missing");
   require(dialog.findChild<QLabel *>("stationUnit_Location2")->text().contains("W"),"Longitude direction cue is missing");
   dialog.findChild<QPushButton *>("stationFindPlace")->setFocus(); events(); capture(dialog,"09-readable-coordinates");
   require(dialog.settings()==initial,"Unfocused coordinate display rounded the pending state");
   longitude->setFocus(); longitude->setText("255.01404000000002");
   require(dialog.settings().value("Location2")=="255.01404000000002","Exact coordinate editing lost precision");
   dialog.reject();
   std::cout<<"PASS readable coordinate presentation, full-precision values and west/east cue\n";
}

void checkMatrix(MainWindow &window)
{
   auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
   require(window.createResource("Array","UxMatrix",editor->toPlainText(),3,3).isEmpty(),"Cannot create matrix fixture");
   auto *panel=openResource(window,"UxMatrix");
   bool visible=false,pasted=false,undone=false;
   QTimer::singleShot(0,&window,[&] {
      auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
      if (!dialog) return;
      auto *grid=dynamic_cast<MatrixTableWidget *>(dialog->findChild<QTableWidget *>("numericGrid"));
      if (grid) {
         events(); visible=grid->viewport()->rect().contains(grid->visualItemRect(grid->item(2,2)));
         grid->setCurrentCell(0,0); grid->setFocus(); QApplication::clipboard()->setText("1 2 3\n4 5 6\n7 8 9");
         QTest::keySequence(grid,QKeySequence::Paste); events();
         const auto ranges=grid->selectedRanges();
         pasted=grid->item(2,2)->text()=="9" && ranges.size()==1 && ranges[0].rowCount()==3 && ranges[0].columnCount()==3;
         capture(*dialog,"10-small-matrix");
         QTest::keySequence(grid,QKeySequence::Undo); undone=grid->item(2,2)->text()=="0";
      }
      dialog->reject();
   });
   panel->findChild<QPushButton *>("editCells_RmatValue")->click();
   require(visible && pasted && undone,"Small matrix visibility, pasted-block selection or Undo failed");
   panel->discardChanges(); panel->parentWidget()->close();
   std::cout<<"PASS all 3x3 cells visible, pasted block highlighted and atomic Undo\n";
}
}

int main(int argc,char **argv)
{
   QApplication application(argc,argv);
   application.setOrganizationName("GMATTests"); application.setApplicationName("UxReviewFixes");
   TestSettings settings;
   try {
      require(argc==2 || argc==3,"Supply the GMAT startup file and optional check name");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      QTemporaryDir directory; require(directory.isValid(),"Cannot create UX fixture directory");
      MainWindow window; window.resize(1500,950); window.show();
      require(window.initialize(startup),"Cannot initialize the UX review fixture"); events();
      const QString selected=argc==3 ? QString::fromLocal8Bit(argv[2]) : QStringLiteral("all");
      require(QStringList{"all","state","import","series","palette","panels","readiness","playback","coordinates","matrix"}.contains(selected),
              "Unknown UX check name");
      const auto run=[&](const QString &name,auto check) { if (selected=="all" || selected==name) check(); };
      run("state",[&] { checkModelState(window,directory.path()); });
      run("import",[&] { checkImportDiscovery(window); });
      run("series",[&] { checkSeries(window); });
      run("palette",[&] { checkImportPalette(window); });
      run("panels",[&] { checkPanelActions(window); });
      run("readiness",[&] { checkImportReadiness(window); });
      run("playback",[&] { checkPlaybackScope(window); });
      run("coordinates",[&] { checkCoordinates(window); });
      run("matrix",[&] { checkMatrix(window); });
      return 0;
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
