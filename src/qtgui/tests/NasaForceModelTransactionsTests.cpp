#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTableWidget>
#include <QTabWidget>
#include <QTabBar>
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
static QByteArray read(const QString &path)
{
   QFile file(path);
   require(file.open(QIODevice::ReadOnly), "Saved fixture is unreadable");
   return file.readAll();
}
static GmatBase *model()
{
   auto *value = Moderator::Instance()->GetConfiguredObject("Gravity");
   require(value, "ForceModel missing");
   return value;
}
static QString flux()
{
   for (const auto &property : resourceProperties(*model()))
      if (property.name == "SRP.Flux") return property.value;
   throw std::runtime_error("Applied SRP flux property missing");
}
static void proof(const QString &name)
{
   const QJsonObject record{{"checkpoint", "force-transaction." + name}, {"operation", name},
      {"outcome", "passed"}, {"facets", QJsonArray{"actual-control", "pending-isolation", "source-model-file", "undo-retained"}}};
   std::cout << "NASA_CHECK " << QJsonDocument(record).toJson(QJsonDocument::Compact).constData() << std::endl;
}

int main(int argc, char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc, argv);
   app.setOrganizationName("GMATTests");
   app.setApplicationName("NasaForceModelTransactions");
   try {
      require(argc == 2 && acceptanceWidgetPlatform(), "Expected startup and supported widget platform");
      TestSettings settings;
      QTemporaryDir files;
      require(files.isValid(), "Temporary fixture directory missing");
      const auto startup = QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window;
      require(window.initialize(startup), "Runtime initialization failed");
      window.show();
      auto *editor = window.findChild<QPlainTextEdit *>("scriptEditor");
      auto *tree = window.findChild<QTreeWidget *>("Resources");
      auto *area = window.findChild<QMdiArea *>("workspace");
      require(editor && tree && area, "Workspace controls missing");
      const auto filename = files.filePath("ForceModel exit α.script");
      const QString baseline = "% Preserve this mission and file exactly\nCreate ForceModel Gravity;\nGravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth};\nGravity.SRP = On;\nGravity.SRP.Flux = 1367;\nGravity.ErrorControl = RSSStep;\nCreate Variable Keep;\nKeep = 9; % preserve unrelated bytes\nBeginMissionSequence;\n";
      auto reset = [&] {
         QFile file(filename);
         require(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "Cannot create saved fixture");
         const auto data = baseline.toUtf8();
         require(file.write(data) == data.size(), "Incomplete saved fixture"); file.close();
         window.show();
         require(window.loadScript(filename) && window.buildScript(), "Saved ForceModel fixture failed to load");
         require(editor->toPlainText() == baseline && !editor->document()->isModified(), "Fixture loaded with different bytes or dirty state");
      };
      auto open = [&] {
         const auto items = tree->findItems("Gravity", Qt::MatchExactly | Qt::MatchRecursive);
         require(items.size() == 1, "ForceModel tree item missing");
         tree->itemDoubleClicked(items.front(), 0); events();
         auto *child = area->currentSubWindow();
         require(child && child->property("resourceName").toString() == "Gravity", "Wrong resource opened");
         require(dynamic_cast<ResourceEditor *>(child->widget()), "ForceModel resource editor missing");
         acceptanceView(window);
         return child;
      };
      auto activate = [&](QWidget *control) {
         for (auto *page = control; page; page = page->parentWidget())
            for (auto *tabs : window.findChildren<QTabWidget *>())
               if (tabs->indexOf(page) >= 0) tabs->setCurrentWidget(page);
         events();
      };
      auto field = [&](ResourceEditor *panel) {
         auto *table = panel->findChild<QTableWidget *>("resourceProperties");
         require(table, "ForceModel property table missing");
         activate(table);
         auto *sections = panel->findChild<QTabBar *>("propertySections");
         if (sections) for (int i = 0; i < sections->count(); ++i)
            if (sections->tabText(i) == "All Properties") sections->setCurrentIndex(i);
         events();
         for (int row = 0; row < table->rowCount(); ++row)
            if (table->item(row, 0)->text() == "SRP.Flux") {
               auto *value = table->item(row, 1);
               table->scrollToItem(value); events();
               require(table->isVisible() && !table->isRowHidden(row) && value->flags().testFlag(Qt::ItemIsEditable), "SRP flux table cell is not visible/editable");
               return value;
            }
         throw std::runtime_error("SRP flux GUI input missing");
      };
      auto select = [&](ResourceEditor *panel) {
         auto *value = panel->findChild<QComboBox *>("resource_ErrorControl");
         require(value, "ErrorControl GUI selector missing");
         activate(value);
         require(value->isVisible(), "ErrorControl selector is hidden");
         value->setCurrentText("RSSState");
      };
      reset();
      auto *child = open();
      auto *panel = dynamic_cast<ResourceEditor *>(child->widget());
      const auto cleanUndo = editor->document()->availableUndoSteps();
      const auto oldStatus = panel->findChild<QLabel *>("resourceStatus")->text();
      field(panel)->setText("-1"); select(panel);
      require(panel->hasChanges() && editor->toPlainText() == baseline && flux().toDouble() == 1367 && model()->GetStringParameter("ErrorControl") == "RSSStep", "Mixed edit escaped pending isolation");
      panel->requestApply(); events();
      auto *status = panel->findChild<QLabel *>("resourceStatus");
      require(child->widget() == panel && panel->hasChanges() && field(panel)->text() == "-1", "Invalid Apply lost its pending editor/input");
      require(status && !status->text().isEmpty() && status->text() != oldStatus, "Invalid flux Apply omitted its diagnostic");
      require(editor->toPlainText() == baseline && editor->document()->availableUndoSteps() == cleanUndo && !editor->document()->isModified() && read(filename) == baseline.toUtf8(), "Invalid Apply changed source, history or saved file");
      require(flux().toDouble() == 1367 && model()->GetStringParameter("ErrorControl") == "RSSStep", "Invalid mixed Apply partially committed the ForceModel");
      proof("apply-error");
      field(panel)->setText("1400"); panel->requestApply(); events();
      panel = dynamic_cast<ResourceEditor *>(child->widget());
      require(panel && !panel->hasChanges() && flux().toDouble() == 1400 && model()->GetStringParameter("ErrorControl") == "RSSState", "Corrected Apply did not commit atomically");
      const auto applied = editor->toPlainText();
      require(applied != baseline && applied.contains("Keep = 9; % preserve unrelated bytes") && read(filename) == baseline.toUtf8(), "Corrected Apply changed unrelated source or saved prematurely");
      require(child->close(), "Clean panel failed to close"); events();
      editor->undo(); require(editor->toPlainText() == baseline && window.buildScript(), "Corrected Apply Undo was not exact");
      editor->redo(); require(editor->toPlainText() == applied && window.buildScript(), "Corrected Apply Redo was not exact");

      auto preparePending = [&] {
         child = open(); panel = dynamic_cast<ResourceEditor *>(child->widget());
         field(panel)->setText("1500");
         require(panel->hasChanges() && editor->toPlainText() == applied && flux().toDouble() == 1400, "Exit fixture lost its separate committed/pending values");
      };
      preparePending();
      QAction *exit = nullptr;
      for (auto *action : window.findChildren<QAction *>()) if (action->text() == "E&xit") exit = action;
      require(exit && exit->isEnabled(), "File Exit action missing");
      const auto undo = editor->document()->availableUndoSteps();
      const auto retained = [&] {
         require(window.isVisible() && child && child->isVisible() && child->widget() == panel && panel->hasChanges() && field(panel)->text() == "1500", "Canceled application exit lost pending panel state");
         require(editor->toPlainText() == applied && editor->document()->availableUndoSteps() == undo && editor->document()->isModified() && read(filename) == baseline.toUtf8(), "Canceled application exit changed source/history/file");
         require(flux().toDouble() == 1400 && model()->GetStringParameter("ErrorControl") == "RSSState", "Canceled exit changed applied ForceModel");
      };
      using Answer = QPair<QString, QMessageBox::StandardButton>;
      auto leave = [&](const QList<Answer> &answers) {
         int seen = 0;
         std::exception_ptr failure;
         QTimer responder;
         QElapsedTimer elapsed; elapsed.start();
         QObject::connect(&responder, &QTimer::timeout, &window, [&] {
            auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            try {
               require(elapsed.elapsed() < 30000, "Exit prompt did not arrive within its viewing budget");
               if (!box) return;
               require(seen < answers.size() && box->windowTitle() == answers[seen].first, "Unexpected exit prompt/order");
               auto *button = box->button(answers[seen].second);
               require(button && button->isVisible() && button->isEnabled(), "Expected exit response button missing");
               ++seen; button->click();
            } catch (...) { failure = std::current_exception(); responder.stop(); if (box) box->reject(); }
         });
         responder.start(10); exit->trigger(); responder.stop(); events();
         if (failure) std::rethrow_exception(failure);
         require(seen == answers.size(), "Application exit skipped an expected decision");
      };
      leave({{"Unapplied panel changes", QMessageBox::Cancel}});
      retained(); proof("program-exit-cancel-pending");
      leave({{"Unapplied panel changes", QMessageBox::Discard}, {"Unsaved script", QMessageBox::Cancel}});
      retained(); proof("program-exit-cancel-save");
      QPointer<QMdiSubWindow> closed = child;
      leave({{"Unapplied panel changes", QMessageBox::Discard}, {"Unsaved script", QMessageBox::Discard}});
      require(!window.isVisible() && closed.isNull() && read(filename) == baseline.toUtf8() && editor->toPlainText() == applied && editor->document()->availableUndoSteps() == undo, "Discard exit did not close or preserve saved file/history");
      proof("program-exit-discard");

      // Reopen the saved baseline, recreate the committed/pending separation,
      // and exercise the opposite save decision through the same Exit action.
      reset(); child = open(); panel = dynamic_cast<ResourceEditor *>(child->widget());
      field(panel)->setText("1400"); select(panel); panel->requestApply(); events();
      require(editor->toPlainText() == applied && flux().toDouble() == 1400, "Reopened fixture did not recreate committed state");
      panel = dynamic_cast<ResourceEditor *>(child->widget()); field(panel)->setText("1500");
      closed = child;
      const auto saveUndo = editor->document()->availableUndoSteps();
      leave({{"Unapplied panel changes", QMessageBox::Discard}, {"Unsaved script", QMessageBox::Save}});
      require(!window.isVisible() && closed.isNull() && read(filename) == applied.toUtf8() && editor->document()->availableUndoSteps() == saveUndo, "Save exit did not persist exactly the committed source/history");
      require(!read(filename).contains("1500") && flux().toDouble() == 1400, "Save exit serialized unapplied ForceModel edits");
      proof("program-exit-save");
      std::cout << "PASS mixed invalid ForceModel Apply atomic rejection/correction, exact Undo/Redo, File Exit Cancel at both stages and saved/unsaved discard decisions; pending values never serialized.\n";
      return 0;
   } catch (BaseException &error) { std::cerr << "FAIL " << error.GetFullMessage() << '\n'; }
     catch (const std::exception &error) { std::cerr << "FAIL " << error.what() << '\n'; }
   return 1;
}
