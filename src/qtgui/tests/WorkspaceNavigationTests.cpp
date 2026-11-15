#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "WorkspaceLayout.hpp"
#include "WorkspaceTabs.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}
void events()
{
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
   QApplication::processEvents();
   QTest::qWait(10);
}
int indexOf(WorkspaceTabs *tabs,const QString &title)
{
   for (int index=0;index<tabs->count();++index)
      if (tabs->tabText(index).contains(title)) return index;
   throw std::runtime_error("An open window is missing from the tab strip");
}
QMdiSubWindow *namedWindow(QMdiArea *area,const QString &name)
{
   for (auto *window:area->subWindowList()) if (window->windowTitle()==name) return window;
   throw std::runtime_error("Expected output window is missing");
}
void checkTiles(QMdiArea *area,int limit)
{
   QList<QRect> rectangles;
   QMap<int,int> rowCounts;
   const QPoint offset(area->horizontalScrollBar()->value(),area->verticalScrollBar()->value());
   for (auto *window:area->subWindowList()) {
      if (window->isHidden() || window->isMinimized()) continue;
      require(!window->isMaximized(),"Tiling left a window maximized");
      const auto rect=window->geometry().translated(offset);
      require(rect.width()>=480 && rect.height()>=360,"Tiled content shrank below its readable size");
      if (rect.top()!=0 || rect.height()!=area->viewport()->height())
         std::cerr<<"Tile "<<window->windowTitle().toStdString()<<" geometry="<<rect.x()<<','<<rect.y()<<','<<rect.width()<<','<<rect.height()
                  <<" viewport="<<area->viewport()->width()<<'x'<<area->viewport()->height()<<std::endl;
      require(rect.top()==0 && rect.height()==area->viewport()->height(),"Tiled content does not fill the workspace height");
      for (const auto &other:rectangles) require(!rect.intersects(other),"Tiled windows overlap");
      rectangles.append(rect); ++rowCounts[rect.top()];
   }
   require(!rectangles.isEmpty(),"No windows were tiled");
   for (int count:rowCounts) require(count<=limit,"Tiling exceeded the selected column limit");
}

void checkNewEditors(MainWindow &window,QMdiArea *area,QPlainTextEdit *editor)
{
   const QString source="Create Spacecraft Existing;\nBeginMissionSequence;\n";
   editor->setPlainText(source);
   require(window.buildScript(),"Editor-opening fixture failed to build");
   auto create=[&](bool accept) {
      std::exception_ptr failure;
      QTimer::singleShot(0,&window,[&] {
         auto *dialog=window.findChild<QDialog *>("newResourceDialog");
         try {
            require(dialog && dialog->isMaximized(),"New Spacecraft dialog did not open maximized");
            require(dialog->windowFlags().testFlag(Qt::WindowMaximizeButtonHint),"New Spacecraft lacks a restore/maximize control");
            dialog->findChild<QLineEdit *>("resourceName")->setText("NewSatellite");
            auto *buttons=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons");
            require(buttons,"Creation buttons missing");
            buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
            require(!dialog->isVisible(),"Spacecraft dialog did not finish");
         } catch (...) {
            failure=std::current_exception();
            if (dialog) dialog->reject();
         }
      });
      window.findChild<QAction *>("createResource")->trigger();
      if (failure) std::rethrow_exception(failure);
      events();
   };
   create(false);
   require(editor->toPlainText()==source,"Cancelling the maximized creator changed the script");
   create(true);
   auto *resource=namedWindow(area,"NewSatellite");
   require(resource->isMaximized(),"New spacecraft editor did not open maximized");
   resource->showNormal(); resource->resize(700,500); events();
   require(!resource->isMaximized(),"Manual restoration of a new editor was overridden");
   resource->close(); events();

   std::exception_ptr failure;
   QTimer::singleShot(0,&window,[&] {
      auto *dialog=window.findChild<QDialog *>("satelliteImportDialog");
      try {
         require(dialog && dialog->isMaximized(),"Satellite import did not open maximized");
         require(dialog->windowFlags().testFlag(Qt::WindowMaximizeButtonHint),"Satellite import lacks a restore/maximize control");
      } catch (...) { failure=std::current_exception(); }
      if (dialog) dialog->reject();
   });
   window.findChild<QAction *>("importSatelliteCatalog")->trigger();
   if (failure) std::rethrow_exception(failure);
   window.plotReceiver()->CreateXyPlotWindow("Positioned","",.1,.1,.4,.4,false,
                                            "Positioned","Time","Value",true);
   events();
   require(!namedWindow(area,"Positioned")->isMaximized(),"An explicit plot placement was replaced by the default state");
}
}

int main(int argc,char **argv)
{
   QApplication application(argc,argv);
   application.setOrganizationName("GMATTests"); application.setApplicationName("WorkspaceNavigation");
   TestSettings settings;
   try {
      require(argc==2,"Supply a startup file");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.resize(1500,950); window.show();
      require(window.initialize(startup),"Workspace runtime initialization failed");
      events();
      auto *area=window.findChild<QMdiArea *>("workspace");
      auto *tabs=dynamic_cast<WorkspaceTabs *>(window.findChild<QTabBar *>("workspaceTabs"));
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      require(area && tabs && editor && tabs->count()==1,"Initial script tab is missing");
      auto *controls=window.findChild<QWidget *>("workspaceWindowControls");
      auto *minimize=window.findChild<QToolButton *>("workspaceMinimizeWindow");
      auto *restore=window.findChild<QToolButton *>("workspaceRestoreWindow");
      auto *closeWindow=window.findChild<QToolButton *>("workspaceCloseWindow");
      require(controls && minimize && restore && closeWindow,"Workspace window controls are missing");
      const auto original=editor->toPlainText();
      QTemporaryDir files; require(files.isValid(),"Script fixture directory is unavailable");
      QFile file(files.filePath("Other.script")); require(file.open(QIODevice::WriteOnly),"Cannot write script fixture");
      file.write("% another script\n"); file.close();
      require(window.openScriptDocument(file.fileName()),"Cannot open a second script");
      auto *other=area->activeSubWindow();
      events();
      require(other->isMaximized(),"A new script window did not open maximized");
      auto *receiver=window.plotReceiver();
      for (int i=0;i<5;++i) {
         const auto name=std::string("Output")+std::to_string(i);
         receiver->CreateXyPlotWindow(name,"",0,0,0,0,false,name,"Time","Value",true);
         receiver->AddXyPlotCurve(name,0,"Curve",0xff0000);
         receiver->UpdateXyPlotCurve(name,0,1,2);
      }
      events();
      require(tabs->count()==7,"Script/output tabs are incomplete");
      auto *latest=namedWindow(area,"Output4");
      require(area->activeSubWindow()==latest && latest->geometry().intersects(area->viewport()->rect()),
              "The newly activated output is outside the workspace viewport");
      require(latest->isMaximized(),"A new unpositioned plot did not open maximized");
      auto *first=namedWindow(area,"Output0"),*second=namedWindow(area,"Output1");
      first->showMinimized(); second->showMinimized(); other->showMinimized(); events();
      require(tabs->count()==7,"Minimized windows disappeared from the tab strip");
      for (auto *target:{first,second,other}) {
         const int index=indexOf(tabs,target==other ? QString("Other.script") : target->windowTitle());
         tabs->setCurrentIndex(index); events();
         QTest::mouseClick(tabs,Qt::LeftButton,Qt::NoModifier,tabs->tabRect(index).center()); events();
         require(!target->isMinimized() && area->activeSubWindow()==target,"A tab did not restore/activate its window");
         require(target->isMaximized(),"A tab did not maximize its window");
         require(controls->isVisible(),"Maximized window controls are missing from the workspace");
         const auto position=controls->mapToGlobal(QPoint(0,0));
         const auto viewportPosition=area->viewport()->mapToGlobal(QPoint(0,0));
         require(position.y()>window.menuBar()->mapToGlobal(QPoint(0,window.menuBar()->height())).y() &&
                 position.y()+controls->height()<=viewportPosition.y() &&
                 std::abs(position.x()+controls->width()-viewportPosition.x()-area->viewport()->width())<=2,
                 "Maximized controls are outside the upper-right workspace header");
         require(!target->maximizedButtonsWidget() || !target->maximizedButtonsWidget()->isVisible(),
                 "Duplicate maximized controls remain in the main menu bar");
         QTest::mouseClick(restore,Qt::LeftButton); events();
         require(!target->isMaximized() && !controls->isVisible(),"Workspace Restore did not restore its window");
         const int selectedIndex=indexOf(tabs,target->windowTitle());
         QTest::mouseClick(tabs,Qt::LeftButton,Qt::NoModifier,tabs->tabRect(selectedIndex).center()); events();
         require(area->activeSubWindow()==target && target->isMaximized(),
                 "Clicking the selected tab did not maximize its window again");
      }
      require(qobject_cast<QPlainTextEdit *>(other->widget())->toPlainText()=="% another script\n",
              "Window controls changed the inactive script");
      const QString directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      if (!directory.isEmpty()) { QDir().mkpath(directory); window.grab().save(directory+"/maximized-workspace.png"); }
      require(editor->toPlainText()==original,"Window navigation changed the active mission");
      first->setWindowTitle("Renamed output"); events();
      require(tabs->tabToolTip(indexOf(tabs,"Renamed output"))=="Renamed output","Tab title/tooltip did not follow its window");
      const auto retained=receiver->model("Output1");
      tabs->setCurrentIndex(indexOf(tabs,second->windowTitle())); events();
      QTest::mouseClick(closeWindow,Qt::LeftButton); events(); require(tabs->count()==6,"Workspace Close retained the output tab");
      require(receiver->show("Output1"),"Cannot reopen retained output"); events();
      require(tabs->count()==7 && receiver->model("Output1")==retained,"Reopening output duplicated its tab or lost its model");
      auto *inactive=qobject_cast<QPlainTextEdit *>(other->widget()); require(inactive,"Second script editor is missing");
      inactive->appendPlainText("% unsaved change");
      events();
      tabs->setCurrentIndex(indexOf(tabs,other->windowTitle())); events();
      QTimer::singleShot(0,[] {
         if (auto *dialog=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) dialog->done(QMessageBox::Cancel);
      });
      QTest::mouseClick(closeWindow,Qt::LeftButton); events();
      require(tabs->count()==7 && inactive->toPlainText().contains("unsaved change"),"Cancelled close lost the script/tab");
      inactive->document()->setModified(false);
      auto *menu=window.findChild<QMenu *>("windowMenu"); require(menu,"Window menu is missing");
      QMetaObject::invokeMethod(menu,"aboutToShow",Qt::DirectConnection);
      require(workspaceColumnLimit()==2,"Default tiling limit is not two columns");
      auto *activeBeforeTiling=area->activeSubWindow();
      window.findChild<QAction *>("tileWindows")->trigger(); events(); checkTiles(area,2);
      require(area->activeSubWindow()==activeBeforeTiling,"Tiling changed the active window");
      int minimized=0;
      QMdiSubWindow *minimizedWindow=nullptr;
      for (auto *child:area->subWindowList()) if (child->isMinimized()) { ++minimized; minimizedWindow=child; }
      require(minimized==5 && tabs->count()==7,"Tiling did not retain five surplus windows as minimized tabs");
      const int minimizedIndex=indexOf(tabs,minimizedWindow->windowTitle());
      QTest::mouseClick(tabs,Qt::LeftButton,Qt::NoModifier,tabs->tabRect(minimizedIndex).center()); events();
      require(area->activeSubWindow()==minimizedWindow && minimizedWindow->isMaximized(),
              "A surplus window was not restored and maximized by its tab");
      tabs->setCurrentIndex(indexOf(tabs,first->windowTitle())); events();
      QTest::mouseClick(minimize,Qt::LeftButton); events();
      require(first->isMinimized(),"Minimizing a plot was undone before tiling");
      window.findChild<QAction *>("tileWindows")->trigger(); events();
      require(first->isMinimized(),"Tiling restored a deliberately minimized plot");
      first->showNormal(); other->showNormal(); latest->showNormal();
      window.resize(2300,1400); events();
      QTimer::singleShot(0,[] {
         if (auto *dialog=qobject_cast<QInputDialog *>(QApplication::activeModalWidget())) { dialog->setIntValue(3); dialog->accept(); }
      });
      window.findChild<QAction *>("maximumTileColumns")->trigger(); events();
      require(workspaceColumnLimit()==3,"Column preference did not persist"); checkTiles(area,3);
      int widestRow=0; QMap<int,int> rows;
      for (auto *child:area->subWindowList()) if (!child->isHidden()&&!child->isMinimized()) widestRow=std::max(widestRow,++rows[child->y()]);
      require(widestRow==3,"Larger workspace did not use the selected third column");
      require(editor->toPlainText()==original,"Tiling changed the mission source");
      if (!directory.isEmpty()) { QDir().mkpath(directory); window.grab().save(directory+"/workspace.png"); }
      checkNewEditors(window,area,editor);
      std::cout<<"PASS maximized new script/plot/resource windows and satellite dialogs, manual restoration, creation Cancel, workspace controls, tabs, close/reopen, dirty-close cancellation, full-height bounded tiling and persisted column preference\n";
      return 0;
   } catch (const std::exception &error) {
      std::cerr<<error.what()<<std::endl; return 1;
   }
}
