#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "FileManager.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QStyle>
#include <QStyleOptionTitleBar>
#include <QSet>
#include <QTabBar>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextDocument>
#include <QThread>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <stdexcept>

static void require(bool value, const char *message)
{
   if (!value) throw std::runtime_error(message);
}

static void events()
{
   QApplication::processEvents();
   QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

static void key(QWidget *widget, Qt::Key value)
{
   QKeyEvent down(QEvent::KeyPress, value, Qt::NoModifier);
   QKeyEvent up(QEvent::KeyRelease, value, Qt::NoModifier);
   QApplication::sendEvent(widget, &down);
   QApplication::sendEvent(widget, &up);
   events();
}

static void titleButton(QMdiSubWindow *child, QStyle::SubControl control)
{
   QStyleOptionTitleBar option;
   option.initFrom(child);
   option.titleBarFlags = child->windowFlags();
   option.titleBarState = child->windowState();
   option.rect = QRect(0, 0, child->width(), child->style()->pixelMetric(QStyle::PM_TitleBarHeight, &option, child));
   const auto rect = child->style()->subControlRect(QStyle::CC_TitleBar, &option, control, child);
   require(rect.isValid() && !rect.isEmpty(), "Child title-bar control has no clickable rectangle");
   const QPointF local = rect.center(), global = child->mapToGlobal(rect.center());
   QMouseEvent move(QEvent::MouseMove, local, global, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
   QApplication::sendEvent(child, &move);
   QMouseEvent press(QEvent::MouseButtonPress, local, global, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
   QMouseEvent release(QEvent::MouseButtonRelease, local, global, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
   QApplication::sendEvent(child, &press);
   QEventLoop settle;
   QTimer::singleShot(50, &settle, &QEventLoop::quit);
   settle.exec();
   QApplication::sendEvent(child, &release);
   events();
}

static void proof(const QString &operation)
{
   const QJsonObject data{{"checkpoint", "force-window." + operation}, {"operation", operation},
      {"outcome", "passed"}, {"facets", QJsonArray{"actual-control", "pending-retained", "source-model-unchanged"}}};
   std::cout << "NASA_CHECK " << QJsonDocument(data).toJson(QJsonDocument::Compact).constData() << std::endl;
}

int main(int argc, char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc, argv);
   app.setOrganizationName("GMATTests");
   app.setApplicationName("NasaForceModelWindow");
   try {
      require(argc == 2 && acceptanceWidgetPlatform(), "Expected startup and supported GUI platform");
      TestSettings settings;
      const auto startup = QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window;
      require(window.initialize(startup), "Runtime initialization failed");
      window.show();
      auto *editor = window.findChild<QPlainTextEdit *>("scriptEditor");
      auto *tree = window.findChild<QTreeWidget *>("Resources");
      auto *area = window.findChild<QMdiArea *>("workspace");
      require(editor && tree && area, "Workspace controls missing");
      const QString source = "% ForceModel window behavior; preserve unrelated source\nCreate ForceModel Gravity;\nGravity.ErrorControl = RSSStep;\nCreate Variable Keep;\nKeep = 9;\nBeginMissionSequence;\n";
      editor->setPlainText(source);
      require(window.buildScript(), "ForceModel fixture failed to build");
      const auto items = tree->findItems("Gravity", Qt::MatchExactly | Qt::MatchRecursive);
      require(items.size() == 1, "ForceModel tree item missing");
      tree->itemDoubleClicked(items.front(), 0);
      events();
      auto *child = area->currentSubWindow();
      auto *panel = child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr;
      require(child && panel && child->property("resourceName").toString() == "Gravity", "Wrong ForceModel child opened");
      child->showNormal();
      window.activateWindow();
      area->setActiveSubWindow(child);
      events();
      acceptanceView(window);
      auto *choice = panel->findChild<QComboBox *>("resource_ErrorControl");
      require(choice && choice->findText("RSSState") >= 0, "ForceModel error-control selector missing");
      choice->setCurrentText("RSSState");
      const auto unchanged = [&] {
         auto *model = Moderator::Instance()->GetConfiguredObject("Gravity");
         require(panel->hasChanges() && choice->currentText() == "RSSState", "Window action lost pending ForceModel input");
         require(editor->toPlainText() == source && model && model->GetStringParameter("ErrorControl") == "RSSStep", "Window action committed or changed source/model");
      };
      unchanged();
      {
         // Deliver real Qt key events to the focused widget. Calling
         // focusNextChild directly would not exercise keyboard dispatch.
         auto tab=[](bool backwards) {
            auto *target=QApplication::focusWidget(); require(target,"Tab traversal lost keyboard focus");
            const auto modifiers=backwards ? Qt::ShiftModifier : Qt::NoModifier;
            QKeyEvent down(QEvent::KeyPress,Qt::Key_Tab,modifiers);
            QKeyEvent up(QEvent::KeyRelease,Qt::Key_Tab,modifiers);
            QApplication::sendEvent(target,&down); QApplication::sendEvent(target,&up); events();
            if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible")
               QThread::msleep(qBound(0,qEnvironmentVariableIntValue("GMAT_QT_ACCEPTANCE_PAUSE_MS"),200));
         };
         auto *first=panel->findChild<QWidget *>("resource_CentralBody"); require(first && first->isVisible() && first->isEnabled(),"First ForceModel body selector unavailable");
         first->setFocus(Qt::TabFocusReason); events();
         QList<QWidget *> forward; QSet<QWidget *> visited;
         for (int step=0;step<256;++step) {
            auto *focused=QApplication::focusWidget(); require(focused && focused->isVisible() && focused->isEnabled(),"Tab reached a hidden or disabled control");
            if (visited.contains(focused)) { require(focused==forward.front(),"Tab focus repeated before completing its cycle"); break; }
            if (!panel->isAncestorOf(focused)) break;
            visited.insert(focused); forward.append(focused); tab(false);
         }
         QStringList names; for (auto *widget:forward) names.append(widget->objectName());
         std::cout<<"ForceModel Tab order: "<<names.join(" -> ").toStdString()<<std::endl;
         for (const auto &name:QStringList{"resource_CentralBody","resource_PrimaryBodies","resource_PointMasses","resource_ErrorControl","resource_SRP","resource_RelativisticCorrection","okPanel","cancelPanel"})
            require(names.contains(name),qPrintable("Tab did not reach displayed ForceModel control "+name));
         require(forward.size()<256,"Tab traversal did not terminate");
         forward.back()->setFocus(Qt::BacktabFocusReason); events();
         for (int i=forward.size()-2;i>=0;--i) { tab(true); require(QApplication::focusWidget()==forward[i],"Shift+Tab did not reverse the forward focus order"); }
         unchanged();
         auto *pages=panel->findChild<QTabWidget *>("resourceEditorTabs"); require(pages,"ForceModel pages missing");
         const auto primaryPage=pages->currentIndex();
         int advanced=-1; for (int i=0;i<pages->count();++i) if (pages->tabText(i)=="Advanced") advanced=i;
         require(advanced>=0,"ForceModel Advanced page missing"); pages->setCurrentIndex(advanced); events();
         auto *sections=panel->findChild<QTabBar *>("propertySections"); require(sections,"ForceModel sections missing");
         int all=-1; for (int i=0;i<sections->count();++i) if (sections->tabText(i)=="All Properties") all=i;
         require(all>=0,"All Properties section missing"); sections->setCurrentIndex(all); events();
         auto *grid=panel->findChild<QTableWidget *>("resourceProperties"); require(grid && grid->isVisible(),"Advanced property grid hidden");
         QList<QModelIndex> cells;
         for (int row=0;row<grid->rowCount();++row) if (!grid->isRowHidden(row))
            for (int col=0;col<grid->columnCount();++col) if (!grid->isColumnHidden(col)) {
               const auto index=grid->model()->index(row,col);
               if (index.flags().testFlag(Qt::ItemIsEnabled)) cells.append(index);
            }
         require(cells.size()>4,"Advanced grid has insufficient visible cells");
         grid->setCurrentIndex(cells.front()); grid->setFocus(Qt::TabFocusReason); events();
         for (int i=1;i<cells.size();++i) { tab(false); require(grid->currentIndex()==cells[i],"Advanced Tab skipped or targeted a hidden/disabled cell"); }
         for (int i=cells.size()-2;i>=0;--i) { tab(true); require(grid->currentIndex()==cells[i],"Advanced Shift+Tab did not reverse visible-cell traversal"); }
         unchanged(); pages->setCurrentIndex(primaryPage); events(); choice->setFocus();
         const QJsonObject tabProof{{"checkpoint","force-window.tab"},{"operation","tab"},{"outcome","passed"},{"controls",QJsonArray::fromStringList(names)},
            {"advanced_cells",cells.size()},{"facets",QJsonArray{"qt-key-events","forward-traversal","reverse-traversal","advanced-grid","hidden-disabled-exclusion","pending-retained","source-model-unchanged"}}};
         std::cout<<"NASA_CHECK "<<QJsonDocument(tabProof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      }
      titleButton(child, QStyle::SC_TitleBarMinButton);
      require(child->isMinimized(), "Title-bar Minimize did not minimize ForceModel child");
      require(!panel->isVisible() && child->height() < 100, "Minimized child retained its full-size content");
      unchanged();
      child->showNormal();
      events();
      require(!child->isMinimized() && !child->isMaximized() && panel->isVisible(), "ForceModel did not restore from minimized state");
      unchanged();
      proof("minimize");
      titleButton(child, QStyle::SC_TitleBarMaxButton);
      require(child->isMaximized(), "Title-bar Maximize did not maximize ForceModel child");
      require(child->geometry().contains(area->viewport()->rect()) && panel->isVisible() && choice->isVisible(), "Maximized child did not fill the workspace with visible pending controls");
      unchanged();
      child->showNormal();
      events();
      require(!child->isMinimized() && !child->isMaximized() && panel->isVisible(), "ForceModel did not restore from maximized state");
      unchanged();
      proof("maximize");

      const auto checkHelp = [&] {
         auto *help = window.findChild<QDialog *>("helpDialog");
         require(help && help->isVisible(), "ForceModel Help did not open");
         auto *browser = help->findChild<QTextBrowser *>("helpBrowser");
         auto *status = help->findChild<QLabel *>("helpStatus");
         const auto helpRoot = QString::fromStdString(FileManager::Instance()->GetFullPathname("HELP_PATH"));
         const auto expected = QDir(helpRoot).filePath("html/ForceModel.html");
         require(QFileInfo(expected).isReadable() && browser && browser->source().toLocalFile() == expected && browser->isReadOnly() && browser->toPlainText().contains("ForceModel") && status && status->text().isEmpty(), "Wrong or unreadable ForceModel help topic");
         acceptanceView(*help);
         key(help, Qt::Key_Escape);
         unchanged();
      };
      auto *help = panel->findChild<QPushButton *>("contextHelp");
      require(help && help->isVisible() && help->isEnabled(), "ForceModel Help button missing");
      help->click(); events(); checkHelp();
      key(choice, Qt::Key_F1); checkHelp();
      proof("help");

      const auto expectedScript = QString::fromStdString(Moderator::Instance()->GetConfiguredObject("Gravity")->GetGeneratingString(Gmat::SHOW_SCRIPT));
      auto *preview = panel->findChild<QPushButton *>("showScript");
      require(preview && preview->isVisible() && preview->isEnabled(), "ForceModel Show Script button missing");
      std::exception_ptr failure;
      bool inspected = false;
      QTimer::singleShot(0, &window, [&] {
         auto *dialog = panel->findChild<QDialog *>("inspectionDialog");
         try {
            require(dialog && dialog->isVisible(), "ForceModel script inspection missing");
            auto *text = dialog->findChild<QPlainTextEdit *>("inspectionText");
            require(text && text->isReadOnly() && text->toPlainText() == expectedScript && text->toPlainText().contains("RSSStep") && !text->toPlainText().contains("RSSState"), "Show Script did not preserve exact applied ForceModel settings");
            inspected = true;
         } catch (...) { failure = std::current_exception(); }
         if (dialog) dialog->reject();
      });
      preview->click();
      if (failure) std::rethrow_exception(failure);
      require(inspected, "Script inspection callback did not execute");
      unchanged();
      proof("show-script");
      panel->discardChanges();
      require(child->close(), "Clean ForceModel child failed to close");
      events();
      auto reopen=[&] {
         tree->itemDoubleClicked(items.front(),0); events(); child=area->currentSubWindow();
         panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr;
         require(panel,"ForceModel could not reopen after Close");
         child->showNormal(); area->setActiveSubWindow(child); acceptanceView(window);
         choice=panel->findChild<QComboBox *>("resource_ErrorControl"); require(choice && choice->isVisible(),"Reopened error control is not visible");
      };
      reopen(); choice->setCurrentText("RSSState"); unchanged();
      const auto undo=editor->document()->availableUndoSteps();
      auto exitPrompt=[&](QMessageBox::StandardButton answer) {
         std::exception_ptr failure; bool inspected=false;
         QTimer response; response.setInterval(1);
         QObject::connect(&response,&QTimer::timeout,&window,[&] {
            auto *dialog=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (!dialog) return; // The title-bar press settles before its release opens the prompt.
            response.stop();
            try {
               require(dialog && dialog->windowTitle()=="Unapplied changes" && dialog->text().contains("Discard"),"ForceModel title-bar Close omitted the unapplied-change prompt");
               acceptanceView(*dialog); inspected=true;
               require(dialog->button(answer) && dialog->button(answer)->isEnabled(),"Exit response missing"); dialog->button(answer)->click();
            } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
         });
         response.start();
         titleButton(child,QStyle::SC_TitleBarCloseButton);
         response.stop();
         if (failure) std::rethrow_exception(failure); require(inspected,"ForceModel Close did not inspect a prompt"); events();
      };
      QPointer<QMdiSubWindow> retained=child;
      exitPrompt(QMessageBox::Cancel);
      require(retained && retained==child && child->isVisible() && child->widget()==panel && editor->document()->availableUndoSteps()==undo,"Canceling ForceModel Close lost its editor/history"); unchanged();
      exitPrompt(QMessageBox::Discard);
      require(!retained && editor->toPlainText()==source && editor->document()->availableUndoSteps()==undo && Moderator::Instance()->GetConfiguredObject("Gravity")->GetStringParameter("ErrorControl")=="RSSStep","Discarding ForceModel Close changed applied data or failed to close");
      reopen(); choice->setCurrentText("RSSState"); unchanged(); retained=child;
      auto *ok=panel->findChild<QPushButton *>("okPanel"); require(ok && ok->isVisible() && ok->isEnabled(),"ForceModel OK action missing"); ok->click(); events();
      require(!retained && editor->toPlainText()!=source && Moderator::Instance()->GetConfiguredObject("Gravity")->GetStringParameter("ErrorControl")=="RSSState","ForceModel OK did not apply and close");
      const auto committed=editor->toPlainText();
      editor->undo(); require(editor->toPlainText()==source && window.buildScript() && Moderator::Instance()->GetConfiguredObject("Gravity")->GetStringParameter("ErrorControl")=="RSSStep","ForceModel OK Undo failed");
      editor->redo(); require(editor->toPlainText()==committed && window.buildScript() && Moderator::Instance()->GetConfiguredObject("Gravity")->GetStringParameter("ErrorControl")=="RSSState","ForceModel OK Redo failed");
      const QJsonObject exitProof{{"checkpoint","force-window.dialog-exit"},{"operation","dialog-exit"},{"outcome","passed"},
         {"facets",QJsonArray{"title-bar-close","pending-cancel","pending-discard","ok-apply-close","source-model-history","exact-undo-redo"}}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(exitProof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      std::cout << "PASS ForceModel child minimize/maximize and restore, correct Help button/F1, and exact applied Show Script with pending edits retained; no field-domain or native-input credit.\n";
      return 0;
   } catch (BaseException &error) { std::cerr << "FAIL " << error.GetFullMessage() << '\n'; }
     catch (const std::exception &error) { std::cerr << "FAIL " << error.what() << '\n'; }
   return 1;
}
