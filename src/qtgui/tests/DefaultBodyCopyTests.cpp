#include "FormationAcceptance.hpp"
#include "CelestialBodyPanel.hpp"
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QPixmap>
#include <QTest>
#include <QTimer>

using namespace formaccept;

int main(int argc, char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc, argv);
   app.setOrganizationName("GMATTests");
   app.setApplicationName("DefaultBodyCopy");
   try
   {
      require(argc == 4, "Supply startup, frozen body-copy plan and body name");
      const auto plan = QJsonDocument::fromJson(read(argv[2])).object();
      const QString name = argv[3];
      require(plan["bodies"].toArray().contains(name), "Unknown default body copy case");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath());
#ifdef __linux__
      rig.save("loaded-images.txt",read("/proc/self/maps"));
#endif
      const QString copyName = name + "Copy";
      const QString initial = "% Default-body copy witness Ω\nCreate Variable Keep;\n"
         "Keep = 41;\nBeginMissionSequence;\nKeep = Keep + 1;\n";
      rig.source->setPlainText(initial);
      require(rig.window.buildScript(), "Default body copy baseline failed Build");
      auto body = [](const QString &value) {
         return Moderator::Instance()->GetSolarSystemInUse()->GetBody(value.toStdString());
      };
      auto properties = [&](const QString &value) {
         auto *object = body(value);
         require(object, "Body-copy native resource missing");
         QJsonObject result;
         for (const auto &field : plan["real_fields"].toArray())
            result[field.toString()] = object->GetRealParameter(object->GetParameterID(field.toString().toStdString()));
         for (const auto &field : plan["string_fields"].toArray())
            result[field.toString()] = QString::fromStdString(object->GetStringParameter(
               object->GetParameterID(field.toString().toStdString())));
         return result;
      };
      require(body(name) && !body(name)->IsUserDefined(), "Original body is not a protected default");
      const auto original = properties(name);
      const auto type = body(name)->GetTypeName();
      QJsonArray stages;
      auto check = [&](const QString &phase, bool edited) {
         auto *source = body(name);
         auto *copy = body(copyName);
         require(source && copy && source != copy && !source->IsUserDefined() && copy->IsUserDefined(),
                 "Body-copy identity or default protection changed");
         require(copy->GetTypeName() == type && copy->UsesDefaultBodyModel() &&
                 copy->GetPhysicalBodyName() == name.toStdString(), "Copied physical provider identity differs");
         require(properties(name) == original, "Copy operation changed the original body");
         auto expected = original;
         if (edited)
            expected["Mu"] = original["Mu"].toDouble()*plan["mu_multiplier"].toDouble();
         require(properties(copyName) == expected, "Copied native body fields differ");
         QJsonArray ephemerides;
         for (const auto &epochValue : plan["epochs"].toArray())
         {
            const double atTime = epochValue.toDouble();
            const Rvector6 reference = source->GetMJ2000State(A1Mjd(atTime));
            const Rvector6 actual = copy->GetMJ2000State(A1Mjd(atTime));
            QJsonArray values;
            for (int component=0;component<6;++component)
            {
               const double tolerance = plan[component<3 ? "state_tolerance_km" : "velocity_tolerance_km_s"].toDouble();
               require(std::isfinite(actual[component]) &&
                       std::abs(actual[component]-reference[component])<=tolerance,
                       "Body-copy ephemeris differs from retained default provider");
               values.append(actual[component]);
            }
            ephemerides.append(QJsonObject{{"epoch",atTime},{"state",values}});
         }
         stages.append(QJsonObject{{"phase",phase},{"fields",properties(copyName)},
                                   {"ephemerides",ephemerides}});
      };
      auto select = [&](const QString &value) {
         rig.closePanels();
         const auto items = rig.resources->findItems(value,Qt::MatchExactly|Qt::MatchRecursive);
         require(items.size()==1, "Body-copy tree item missing or ambiguous");
         rig.resources->expandAll();
         rig.resources->setCurrentItem(items.first());
         rig.resources->scrollToItem(items.first());
         events();
         return items.first();
      };
      auto copyThroughMenu = [&](bool accept) {
         auto *item = select(name);
         const auto before = rig.source->toPlainText();
         const int undo = rig.source->document()->availableUndoSteps();
         bool visited = false;
         std::exception_ptr failure;
         QTimer::singleShot(0,&rig.window,[&] {
            auto *menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
            try
            {
               require(menu, "Default-body resource menu missing");
               QAction *copyAction = nullptr;
               QAction *cloneAction = nullptr;
               for (auto *action : menu->actions())
               {
                  if (action->objectName()=="copyCelestialBody") copyAction=action;
                  if (action->objectName()=="cloneResource") cloneAction=action;
               }
               require(copyAction && copyAction->isEnabled() && cloneAction && !cloneAction->isEnabled(),
                       "Body-copy action or original clone protection differs");
               QTimer::singleShot(0,&rig.window,[&] {
                  auto *dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
                  try
                  {
                     require(dialog && dialog->objectName()=="cloneResourceDialog", "Body-copy modal missing");
                     auto *input = dialog->findChild<QLineEdit*>("resourceName");
                     auto *buttons = dialog->findChild<QDialogButtonBox*>("resourceCloneButtons");
                     require(input && buttons, "Body-copy dialog controls missing");
                     input->setText(copyName);
                     acceptanceView(*dialog);
                     if (accept && !rig.evidence.isEmpty())
                        require(dialog->grab().save(rig.evidence+"/copy-dialog.png"), "Body-copy screenshot failed");
                     QTest::mouseClick(buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel),Qt::LeftButton);
                     require(dialog->result()==(accept ? QDialog::Accepted : QDialog::Rejected),
                             "Body-copy dialog did not complete");
                     visited=true;
                  }
                  catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
               });
               menu->setActiveAction(copyAction);
               QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
               QApplication::sendEvent(menu,&enter);
            }
            catch (...) { failure=std::current_exception(); if (menu) menu->close(); }
         });
         rig.resources->customContextMenuRequested(rig.resources->visualItemRect(item).center());
         events();
         if (failure) std::rethrow_exception(failure);
         require(visited, "Default-body copy dialog was not exercised");
         if (!accept)
            require(rig.source->toPlainText()==before && rig.source->document()->availableUndoSteps()==undo &&
                    !body(copyName) && properties(name)==original, "Copy Cancel changed source, history or bodies");
         rig.closePanels();
      };
      copyThroughMenu(false);
      copyThroughMenu(true);
      const auto copied = rig.source->toPlainText();
      rig.save("copied.script",copied.toUtf8());
      check("copied",false);
      auto *item = select(copyName);
      rig.resources->itemDoubleClicked(item,0);
      events();
      auto *panel = rig.area->activeSubWindow();
      require(panel && dynamic_cast<CelestialBodyPanel*>(panel->widget()), "Copied body editor missing");
      auto *muField = panel->findChild<QLineEdit*>("body_Mu");
      auto *buttons = panel->findChild<QDialogButtonBox*>();
      require(muField && buttons, "Copied body mass parameter controls missing");
      muField->setText(number(original["Mu"].toDouble()*plan["mu_multiplier"].toDouble()));
      QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);
      events();
      rig.closePanels();
      check("edited",true);
      const auto edited = rig.source->toPlainText();
      rig.source->undo();
      require(rig.source->toPlainText()==copied && rig.window.buildScript(), "Body-copy edit Undo failed");
      check("undo-edit",false);
      rig.source->undo();
      require(rig.source->toPlainText()==initial && rig.window.buildScript() && !body(copyName), "Body-copy Undo failed");
      rig.source->redo();
      require(rig.source->toPlainText()==copied && rig.window.buildScript(), "Body-copy Redo failed");
      check("redo-copy",false);
      rig.source->redo();
      require(rig.source->toPlainText()==edited && rig.window.buildScript(), "Body-copy edit Redo failed");
      check("redo-edit",true);
      const auto saved = rig.files.filePath("Copied body Ω.script");
      require(rig.window.saveScriptTo(saved) && rig.window.loadScript(saved) && rig.window.buildScript() &&
              rig.source->toPlainText()==edited, "Copied body Unicode save/reopen failed");
      check("reopened",true);
      require(rig.window.runMission()==MainWindow::RunResult::Completed &&
              Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,
              "Copied body mission continuation failed");
      check("executed",true);
      rig.save("final.script",rig.source->toPlainText().toUtf8());
      rig.save("proof.json",QJsonDocument(QJsonObject{{"status","passed"},{"body",name},
         {"type",QString::fromStdString(type)},{"original_fields",original},{"stages",stages},
         {"cancel_isolated",true},{"default_protected",true},{"sentinel",42},
         {"scope","Native X11 widgets, default-body copy, field/provider retention and persistence"}}).toJson());
      std::cout << "Default body copy passed for " << name.toStdString() << std::endl;
      return 0;
   }
   catch (BaseException &error) { std::cerr << error.GetFullMessage() << std::endl; }
   catch (std::exception &error) { std::cerr << error.what() << std::endl; }
   return 1;
}
