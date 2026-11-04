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
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }
static GmatBase *object(const QString &name)
{
   auto *result = Moderator::Instance()->GetConfiguredObject(name.toStdString());
   require(result, "Configured ForceModel missing"); return result;
}
static QString canonical(const QString &name)
{
   auto result = QString::fromStdString(object(name)->GetGeneratingString(Gmat::SHOW_SCRIPT));
   result.replace(QRegularExpression("\\b" + QRegularExpression::escape(name) + "\\b"), "MODEL");
   return result;
}
static QMap<QString, QString> values(const QString &name)
{
   QMap<QString, QString> result;
   for (const auto &field : resourceProperties(*object(name))) {
      auto value = field.value;
      value.replace(QRegularExpression("\\b" + QRegularExpression::escape(name) + "\\b"), "MODEL");
      result.insert(field.name, value);
   }
   return result;
}
static void proof(const QString &operation)
{
   const QJsonObject data{{"checkpoint", "force-clone." + operation}, {"operation", operation},
      {"outcome", "passed"}, {"facets", QJsonArray{"actual-menu", "complete-copy", "owned-force-independence", "source-history", "unrelated-preserved"}}};
   std::cout << "NASA_CHECK " << QJsonDocument(data).toJson(QJsonDocument::Compact).constData() << std::endl;
}
int main(int argc, char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc, argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("NasaForceModelClone");
   try {
      require(argc == 2 && acceptanceWidgetPlatform(), "Expected startup and supported GUI platform");
      TestSettings settings; QTemporaryDir files; require(files.isValid(), "Temporary directory unavailable");
      const auto startup = QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup), "Runtime initialization failed"); window.show();
      auto *editor = window.findChild<QPlainTextEdit *>("scriptEditor");
      auto *tree = window.findChild<QTreeWidget *>("Resources");
      auto *area = window.findChild<QMdiArea *>("workspace");
      require(editor && tree && area, "Workspace controls missing");
      const QString mission = "BeginMissionSequence;\nKeep = Keep + 2; % unrelated mission\n";
      const QString source = "% Multi-force clone fixture\nCreate ForceModel Original;\nOriginal.CentralBody = Earth;\nOriginal.PrimaryBodies = {Earth};\nOriginal.PointMasses = {Luna, Sun};\nOriginal.GravityField.Earth.Degree = 4;\nOriginal.GravityField.Earth.Order = 4;\nOriginal.Drag.AtmosphereModel = NRLMSISE00;\nOriginal.SRP = On;\nOriginal.SRP.Flux = 1367;\nOriginal.RelativisticCorrection = On;\nOriginal.ErrorControl = RSSStep;\nCreate Variable Keep;\nKeep = 11; % unrelated resource\n" + mission;
      editor->setPlainText(source); require(window.buildScript(), "Multi-force fixture failed to build");
      auto item = [&](const QString &name) {
         const auto found = tree->findItems(name, Qt::MatchExactly | Qt::MatchRecursive);
         require(found.size() == 1, "ForceModel tree entry missing or duplicated"); return found.front();
      };
      auto unrelated = [&] {
         require(editor->toPlainText().contains("Keep = 11; % unrelated resource") && editor->toPlainText().endsWith(mission), "Clone workflow changed unrelated source");
         require(object("Keep")->GetRealParameter("Value") == 11, "Clone workflow changed unrelated configured value");
      };
      auto activate = [&](QWidget *control) {
         for (auto *page = control; page; page = page->parentWidget()) for (auto *tabs : window.findChildren<QTabWidget *>())
            if (tabs->indexOf(page) >= 0) tabs->setCurrentWidget(page);
         events();
      };
      auto set = [&](ResourceEditor *panel, const QString &name, const QString &value) {
         if (auto *choice = panel->findChild<QComboBox *>("resource_" + name)) {
            activate(choice); require(choice->isVisible() && choice->findText(value) >= 0, "Requested visible ForceModel choice missing"); choice->setCurrentText(value); return;
         }
         auto *table = panel->findChild<QTableWidget *>("resourceProperties"); require(table, "ForceModel table missing"); activate(table);
         if (auto *sections = panel->findChild<QTabBar *>("propertySections")) for (int i = 0; i < sections->count(); ++i)
            if (sections->tabText(i) == "All Properties") sections->setCurrentIndex(i);
         for (int row = 0; row < table->rowCount(); ++row) if (table->item(row, 0)->text() == name) {
            auto *cell = table->item(row, 1); table->scrollToItem(cell); events();
            require(!table->isRowHidden(row) && cell->flags().testFlag(Qt::ItemIsEditable), "ForceModel cell is not editable/visible"); cell->setText(value); return;
         }
         throw std::runtime_error(("Missing ForceModel input " + name).toStdString());
      };
      auto change = [&](const QString &name, const QMap<QString, QString> &edits) {
         const auto before = editor->toPlainText(); const auto oldValues = values(name);
         tree->itemDoubleClicked(item(name), 0); events();
         auto *child = area->currentSubWindow(); auto *panel = child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr;
         require(panel && child->property("resourceName").toString() == name, "Wrong ForceModel edit window"); acceptanceView(window);
         for (auto it = edits.cbegin(); it != edits.cend(); ++it) set(panel, it.key(), it.value());
         require(panel->hasChanges() && editor->toPlainText() == before && values(name) == oldValues, "ForceModel edit escaped pending isolation");
         panel->requestApply(); events(); panel = dynamic_cast<ResourceEditor *>(child->widget());
         if (!panel || panel->hasChanges()) {
            auto *status = panel ? panel->findChild<QLabel *>("resourceStatus") : nullptr;
            std::cerr << "Apply failure for " << name.toStdString() << ": " << (status ? status->text().toStdString() : "missing status") << '\n'
                      << window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString() << '\n';
         }
         require(panel && !panel->hasChanges(), "ForceModel edit failed to commit");
         const auto after = editor->toPlainText(); require(after != before, "ForceModel Apply did not change source");
         const auto afterValues = values(name);
         for (auto it = oldValues.cbegin(); it != oldValues.cend(); ++it) if (!edits.contains(it.key())) require(afterValues.value(it.key()) == it.value(), "ForceModel Apply changed an unrelated property");
         for (auto it = edits.cbegin(); it != edits.cend(); ++it) {
            bool numeric = false; const auto number = it.value().toDouble(&numeric);
            require(numeric ? afterValues.value(it.key()).toDouble() == number : afterValues.value(it.key()) == it.value(), "ForceModel did not retain an edited value");
         }
         require(child->close(), "Clean edit window failed to close"); events();
         editor->undo(); require(editor->toPlainText() == before && window.buildScript() && values(name) == oldValues, "ForceModel edit Undo was not exact");
         editor->redo(); require(editor->toPlainText() == after && window.buildScript() && values(name) == afterValues, "ForceModel edit Redo was not exact"); unrelated();
      };
      change("Original", {{"SRP.Flux", "1400"}, {"ErrorControl", "RSSState"}, {"GravityField.Earth.Degree", "6"}, {"GravityField.Earth.Order", "6"}});
      const auto original = canonical("Original"); const auto originalValues = values("Original");
      auto clone = [&](bool accept) {
         tree->setCurrentItem(item("Original"));
         auto *action = window.findChild<QAction *>("cloneResource"); require(action && action->isEnabled(), "Clone action missing");
         std::exception_ptr failure; bool visited = false;
         QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<QDialog *>("cloneResourceDialog");
            try {
               require(dialog && dialog->isVisible(), "Clone dialog missing"); visited = true;
               dialog->findChild<QLineEdit *>("resourceName")->setText("Copy");
               auto *buttons = dialog->findChild<QDialogButtonBox *>("resourceCloneButtons"); require(buttons, "Clone buttons missing");
               buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
               require(dialog->result() == (accept ? QDialog::Accepted : QDialog::Rejected), "Clone dialog rejected a valid configured ForceModel");
            } catch (...) { failure = std::current_exception(); if (dialog) dialog->reject(); }
         });
         action->trigger(); events(); if (failure) std::rethrow_exception(failure); require(visited, "Clone dialog was never visited");
      };
      const auto beforeClone = editor->toPlainText(); clone(false);
      require(editor->toPlainText() == beforeClone && !Moderator::Instance()->GetConfiguredObject("Copy"), "Clone Cancel changed source/model");
      clone(true); item("Copy");
      require(canonical("Original") == original && canonical("Copy") == original && values("Copy") == originalValues, "Clone differs from complete original configuration");
      auto independent = [&] {
         auto *a = object("Original"), *b = object("Copy");
         require(a != b && a->GetOwnedObjectCount() == b->GetOwnedObjectCount() && a->GetOwnedObjectCount() >= 6, "Clone lost owned forces");
         QSet<GmatBase *> originalOwned; QSet<QString> types;
         for (int i = 0; i < a->GetOwnedObjectCount(); ++i) originalOwned.insert(a->GetOwnedObject(i));
         for (int i = 0; i < b->GetOwnedObjectCount(); ++i) {
            auto *owned = b->GetOwnedObject(i); require(owned && !originalOwned.contains(owned), "Clone shares an owned force object"); types.insert(QString::fromStdString(owned->GetTypeName()));
         }
         for (const auto &type : {"GravityField", "PointMassForce", "DragForce", "SolarRadiationPressure", "RelativisticCorrection"}) require(types.contains(type), "Clone omitted a configured force family");
      };
      independent(); unrelated(); proof("configured-copy");
      change("Copy", {{"SRP.Flux", "1420"}, {"ErrorControl", "LargestState"}});
      require(canonical("Original") == original && values("Original") == originalValues, "Changing clone changed original");
      const auto changedCopy = canonical("Copy"); const auto changedCopyValues = values("Copy");
      independent(); proof("change-copy");
      change("Original", {{"SRP.Flux", "1350"}, {"GravityField.Earth.Degree", "3"}, {"GravityField.Earth.Order", "1"}});
      require(canonical("Copy") == changedCopy && values("Copy") == changedCopyValues, "Changing original changed clone");
      const auto changedOriginal = canonical("Original"); independent(); proof("change-original");
      auto remove = [&](const QString &name) {
         const auto before = editor->toPlainText(); auto *entry = item(name); tree->scrollToItem(entry); events();
         std::exception_ptr failure; bool confirmed = false;
         QTimer::singleShot(0, &window, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            try {
               require(menu, "Resource context menu missing"); QAction *remove = nullptr;
               for (auto *action : menu->actions()) if (action->text() == "Delete resource…") remove = action;
               require(remove && remove->isEnabled(), "Resource Delete action missing");
               QTimer::singleShot(0, &window, [&] {
                  auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                  try { require(box && box->windowTitle() == "Delete resource", "Delete confirmation missing"); confirmed = true; box->button(QMessageBox::Yes)->click(); }
                  catch (...) { failure = std::current_exception(); if (box) box->reject(); }
               });
               menu->setActiveAction(remove); QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier); QApplication::sendEvent(menu, &enter);
            } catch (...) { failure = std::current_exception(); if (menu) menu->close(); }
         });
         tree->customContextMenuRequested(tree->visualItemRect(entry).center()); events();
         if (failure) std::rethrow_exception(failure);
         require(confirmed && !Moderator::Instance()->GetConfiguredObject(name.toStdString()) && tree->findItems(name, Qt::MatchExactly | Qt::MatchRecursive).isEmpty(), "Delete left resource/model/tree state");
         unrelated(); return before;
      };
      const auto beforeDeleteCopy = remove("Copy"); require(canonical("Original") == changedOriginal, "Deleting clone changed original"); proof("delete-copy");
      editor->undo(); require(editor->toPlainText() == beforeDeleteCopy && window.buildScript() && canonical("Copy") == changedCopy && canonical("Original") == changedOriginal, "Delete-clone Undo did not restore exact independent objects");
      const auto beforeDeleteOriginal = remove("Original"); require(canonical("Copy") == changedCopy && values("Copy") == changedCopyValues, "Deleting original changed clone");
      editor->undo(); require(editor->toPlainText() == beforeDeleteOriginal && window.buildScript(), "Delete-original Undo was not exact"); independent();
      editor->redo(); require(window.buildScript() && !Moderator::Instance()->GetConfiguredObject("Original") && canonical("Copy") == changedCopy, "Delete-original Redo lost clone");
      const auto saved = files.filePath("Independent clone α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && values("Copy") == changedCopyValues && canonical("Copy") == changedCopy, "Saved clone did not preserve complete state");
      unrelated(); proof("delete-original");
      std::cout << "PASS complete mixed-force clone state and independent owned objects, multi-field edits in either copy, deletion of either copy, exact Undo/Redo and Unicode save/reopen. No whole-domain physics or all-field input validation credit.\n";
      return 0;
   } catch (BaseException &error) { std::cerr << "FAIL " << error.GetFullMessage() << '\n'; }
     catch (const std::exception &error) { std::cerr << "FAIL " << error.what() << '\n'; }
   return 1;
}
