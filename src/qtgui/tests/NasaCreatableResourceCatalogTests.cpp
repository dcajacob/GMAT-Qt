#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTest>
#include <QTextDocument>
#include <QTimer>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char **argv){
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("CreatableResourceCatalog");
   try{
      require(argc==2&&acceptanceWidgetPlatform(),"Supply startup on a widget platform");TestSettings settings;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window;require(window.initialize(startup),"Catalog runtime initialization failed");window.show();
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");require(source&&window.buildScript(),"Catalog baseline build failed");
      const auto before=source->toPlainText();const auto undo=source->document()->availableUndoSteps();
      auto *moderator=Moderator::Instance();const auto objectsBefore=moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT);
      QStringList native;QJsonArray categories;
      for(const auto category:{Gmat::SPACECRAFT,Gmat::FORMATION,Gmat::SPACE_POINT,Gmat::REGION,Gmat::HARDWARE,Gmat::BURN,Gmat::PROP_SETUP,Gmat::ODE_MODEL,Gmat::COORDINATE_SYSTEM,Gmat::SOLVER,Gmat::SUBSCRIBER,Gmat::FUNCTION,Gmat::EVENT_LOCATOR,Gmat::CALCULATED_POINT,Gmat::CELESTIAL_BODY,Gmat::MEASUREMENT_MODEL,Gmat::ERROR_MODEL,Gmat::INTERFACE,Gmat::DATA_FILTER,Gmat::FIELD_OF_VIEW}){
         QJsonArray types;for(const auto &name:moderator->GetListOfViewableItems(category)){native.append(QString::fromStdString(name));types.append(QString::fromStdString(name));}
         categories.append(QJsonObject{{"category_id",int(category)},{"types",types}});
      }
      for(const auto *name:{"Smoother","ProcessNoiseModel","EstimatedParameter"}){
         const auto category=GmatType::GetTypeId(name);if(category==Gmat::UNKNOWN_OBJECT)continue;
         QJsonArray types;for(const auto &type:moderator->GetListOfViewableItems(category)){native.append(QString::fromStdString(type));types.append(QString::fromStdString(type));}
         categories.append(QJsonObject{{"category_name",name},{"category_id",int(category)},{"types",types}});
      }
      native.append({"Variable","String","Array"});const bool starAdvertised=native.contains("Star");native.removeAll("Star");native.removeDuplicates();native.sort();
      auto *action=window.findChild<QAction *>("createResource");require(action&&action->isEnabled(),"Catalog creation action unavailable");QMenu *menu=nullptr;
      for(auto *candidate:window.menuBar()->findChildren<QMenu *>())if(candidate->actions().contains(action))menu=candidate;
      require(menu,"Catalog edit menu missing");acceptanceView(window);bool visited=false;std::exception_ptr failure;QJsonArray offered;
      QTimer::singleShot(0,&window,[&]{
         try{
            require(menu->isVisible(),"Catalog actual menu not visible");acceptanceView(*menu);
            QTimer::singleShot(0,&window,[&]{auto *dialog=window.findChild<QDialog *>("newResourceDialog");try{
               require(dialog,"Catalog actual resource dialog missing");auto *combo=dialog->findChild<QComboBox *>("resourceType");require(combo&&combo->isEnabled(),"Catalog type selector unavailable");acceptanceView(*dialog);
               QStringList actual;for(int i=0;i<combo->count();++i){actual.append(combo->itemText(i));offered.append(combo->itemText(i));}
               require(actual==native&&!actual.isEmpty(),"GUI catalog differs from current native resource view and explicit exceptions");
               combo->setFocus();acceptanceView(*combo);QTest::keyClick(combo,Qt::Key_Down,Qt::AltModifier);QApplication::processEvents();require(combo->view()->isVisible(),"Catalog popup did not open");acceptanceView(*combo->view());QTest::keyClick(combo,Qt::Key_Escape);
               auto *cancel=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel);require(cancel,"Catalog Cancel absent");acceptanceView(*cancel);QTest::mouseClick(cancel,Qt::LeftButton);visited=true;
            }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});
            menu->setActiveAction(action);QTest::keyClick(menu,Qt::Key_Return);
         }catch(...){failure=std::current_exception();menu->close();}
      });
      QTest::mouseClick(window.menuBar(),Qt::LeftButton,Qt::NoModifier,window.menuBar()->actionGeometry(menu->menuAction()).center());QApplication::processEvents();if(failure)std::rethrow_exception(failure);
      require(visited&&source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&objectsBefore==moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT),"Catalog inspection/Cancel changed configured objects/source/history");
      QJsonObject proof{{"checkpoint","current-creatable-resource-catalog"},{"outcome","passed"},{"offered_types",offered},{"offered_type_count",offered.size()},{"native_categories",categories},{"native_Star_advertised",starAdvertised},{"explicit_excluded_types",QJsonArray{"Star"}},{"configured_objects_source_and_history_unchanged",true},{"facets",QJsonArray{"actual-menu-and-dialog-type-selector","native-resource-catalog-and-explicit-exceptions","cancel-source-history-configured-object-isolation"}},{"scope","Inventory of current loaded GUI resource types only. Listing and popup visibility do not qualify creation,function-local use,scientific behavior or all resource properties. Star is intentionally excluded by current GUI policy;existing Sun remains editable."}};
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!evidence.isEmpty()){QFile file(QDir(evidence).filePath("resource-catalog.json"));auto bytes=QJsonDocument(proof).toJson();require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Catalog evidence write failed");}
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
