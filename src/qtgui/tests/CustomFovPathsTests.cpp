#include "MainWindow.hpp"
#include "ResourceDraft.hpp"
#include "ResourcePreview.hpp"
#include "HelpDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "FileManager.hpp"
#include "CustomFOV.hpp"
#include "CustomFovProperties.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <iostream>
#include <optional>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static std::optional<std::string> definition()
{
   try { return FileManager::Instance()->GetPathDefinition("FOV_DATA_PATH"); }
   catch (BaseException &) { return std::nullopt; }
}
static GmatBase *object(const char *name) { return Moderator::Instance()->GetConfiguredObject(name); }
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("CustomFovPaths");
   try {
      require(argc==2 && QGuiApplication::platformName()=="offscreen","FOV paths requires an offscreen startup");
      TestSettings settings; QTemporaryDir masks,documents;
      require(masks.isValid() && documents.isValid(),"Temporary FOV paths unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"FOV paths runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Source editor missing");
      const QString baseline="% preserve relative FOV source α\nCreate Variable Keep;\nKeep = 7;\nBeginMissionSequence;\n";
      editor->setPlainText(baseline); require(window.buildScript(),"FOV path baseline failed");
      require(window.saveScriptTo(documents.filePath("mission β.script")),"FOV baseline save failed");
      auto *files=FileManager::Instance(); const auto initialState=files->CaptureState(); const auto initialDefinition=definition();
      int failures=0;
      const auto check=[&](bool value,const char *label) { std::cout<<(value ? "PASS " : "FAIL ")<<label<<'\n'; if (!value) ++failures; };
      const auto initialized=[&](const char *name) {
         try { auto *configured=object(name); if (!configured) return false; QtResourcePreview copy(configured->Clone()); return copy->Initialize() && copy->GetRvectorParameter("ConeAngles").GetSize()==4; }
         catch (BaseException &error) { std::cout<<"Initialization detail: "<<error.GetFullMessage()<<'\n'; return false; }
      };
      {
         auto draft=resourceDraft("CustomFOV","UnregisteredFov"); require(draft && !object("UnregisteredFov"),"FOV draft was registered");
         check(definition()==initialDefinition,"Detached FOV draft preserves existing path or absence");
      }
      files->RestoreState(*initialState);
      const auto leaf=QString("relative mask α.txt"); QFile input(masks.filePath(leaf)); require(input.open(QIODevice::WriteOnly),"FOV file fixture unavailable"); input.write("ConeClockAngles\n20 0\n20 90\n20 180\n20 270\n"); input.close();
      {
         auto draft=resourceDraft("CustomFOV","AbsolutePending");
         applyCustomFovProperties(*draft,{{"@CustomFovSource","File"},{"FOVFileName",masks.filePath(leaf)}});
         check(definition()==initialDefinition,"First absolute-file detached validation preserves path absence");
         QtResourcePreview validated(draft->Clone());
         check(validated->Initialize() && validated->GetRvectorParameter("ConeAngles").GetSize()==4,"First absolute-file detached FOV initializes");
      }
      files->RestoreState(*initialState);
      const auto alias=[&] {
         files->AddFileType("FOV_TEST_PATH",masks.path().toStdString());
         files->AddFileType("FOV_DATA_PATH","FOV_TEST_PATH");
         return definition();
      };
      const auto configuredDefinition=alias();
      require(configuredDefinition && *configuredDefinition=="FOV_TEST_PATH/","FOV test alias was not stored verbatim");
      const auto unchanged=[&](const char *label) { check(definition()==configuredDefinition,label); };
      const auto sourceBefore=editor->toPlainText(); const auto undoBefore=editor->document()->availableUndoSteps();
      std::exception_ptr pendingFailure;
      QTimer::singleShot(0,&window,[&] {
         auto *dialog=window.findChild<QDialog *>("newResourceDialog");
         try {
            require(dialog,"Resource creator missing"); auto *types=dialog->findChild<QComboBox *>("resourceType");
            require(types && types->findText("CustomFOV")>=0,"CustomFOV creator type missing"); types->setCurrentText("CustomFOV");
            unchanged("Choosing CustomFOV preserves raw path alias");
            auto *name=dialog->findChild<QLineEdit *>("resourceName"); require(name,"Creator name missing"); name->setText("PendingFov");
            auto *buttons=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons"); require(buttons && buttons->button(QDialogButtonBox::Help),"Creator Help missing");
            const auto help=[&] {
               HelpDialog *visible=nullptr; for (auto *candidate:dialog->findChildren<QDialog *>("helpDialog")) if (auto *found=dynamic_cast<HelpDialog *>(candidate);found && found->isVisible()) visible=found;
               require(visible,"Pending FOV Help did not open"); unchanged("Pending CustomFOV Help preserves raw path alias"); visible->reject(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
            };
            buttons->button(QDialogButtonBox::Help)->click(); help();
            QKeyEvent down(QEvent::KeyPress,Qt::Key_F1,Qt::NoModifier),up(QEvent::KeyRelease,Qt::Key_F1,Qt::NoModifier);
            QApplication::sendEvent(name,&down); QApplication::sendEvent(name,&up); help();
         } catch (...) { pendingFailure=std::current_exception(); }
         if (dialog) dialog->reject();
      });
      auto *create=window.findChild<QAction *>("createResource"); require(create && create->isEnabled(),"Resource creator unavailable"); create->trigger();
      if (pendingFailure) std::rethrow_exception(pendingFailure);
      unchanged("CustomFOV creator Cancel preserves raw path alias");
      check(editor->toPlainText()==sourceBefore && editor->document()->availableUndoSteps()==undoBefore && !object("PendingFov"),"Pending creator Help/F1/Cancel preserves source/model/Undo");
      // With no configured FOV path, a committed CustomFOV still installs the
      // normal engine default. Detached creation must preserve prior absence.
      files->RestoreState(*initialState);
      const QMap<QString,QString> vertices{{"@CustomFovSource","Vertices"},{"@CustomFovCoordinates","ConeClock"},{"ConeAngles","20 20 20 20"},{"ClockAngles","0 90 180 270"}};
      require(window.createResource("CustomFOV","Mask",baseline,1,1,std::nullopt,{},{},vertices).isEmpty(),"Vertex mask fixture failed to create");
      check(bool(definition()),"Committed FOV retains engine default when path was absent");
      check(initialized("Mask"),"Committed vertex FOV initializes");
      auto source=editor->toPlainText(); auto steps=editor->document()->availableUndoSteps(); alias();
      const auto invalidCreate=window.createResource("CustomFOV","MissingFov",source,1,1,std::nullopt,{},{},{{"@CustomFovSource","File"},{"FOVFileName","missing mask.txt"}});
      check(!invalidCreate.isEmpty() && editor->toPlainText()==source && editor->document()->availableUndoSteps()==steps && !object("MissingFov"),"Invalid relative FOV create preserves source/model/Undo"); unchanged("Invalid relative FOV create preserves raw path alias");
      alias();
      const auto validCreate=window.createResource("CustomFOV","FileMask",source,1,1,std::nullopt,{},{},{{"@CustomFovSource","File"},{"FOVFileName",leaf}});
      if (!validCreate.isEmpty()) std::cout<<"Create detail: "<<validCreate.toStdString()<<'\n';
      check(validCreate.isEmpty() && object("FileMask"),"Relative FOV create uses configured fallback path"); unchanged("Relative FOV create preserves raw path alias");
      if (validCreate.isEmpty()) check(initialized("FileMask"),"Created relative file FOV initializes after reconstruction");
      source=editor->toPlainText(); alias();
      const auto applied=window.applyResourceChanges("Mask",{{"@CustomFovSource","File"},{"FOVFileName",leaf}},source);
      if (!applied.isEmpty()) std::cout<<"Apply detail: "<<applied.toStdString()<<'\n';
      check(applied.isEmpty(),"Relative FOV Apply validates and commits"); unchanged("Relative FOV Apply preserves raw path alias");
      check(initialized("Mask"),"Applied relative file FOV initializes after reconstruction");
      source=editor->toPlainText(); steps=editor->document()->availableUndoSteps(); alias(); auto *prior=object("Mask");
      const auto invalidApply=window.applyResourceChanges("Mask",{{"@CustomFovSource","File"},{"FOVFileName","missing mask.txt"}},source);
      check(!invalidApply.isEmpty() && editor->toPlainText()==source && editor->document()->availableUndoSteps()==steps && object("Mask")==prior,"Invalid relative FOV Apply preserves source/model/Undo"); unchanged("Invalid relative FOV Apply preserves raw path alias");
      alias();
      const auto invalidCandidate=window.createResource("Array","BadCells",source,1,1,std::nullopt,{},{},{{"@ArrayExpressions",R"([{"row":1,"column":1,"expression":"MissingOperand"}])"}});
      if (!invalidCandidate.isEmpty()) std::cout<<"Rejected candidate detail: "<<invalidCandidate.toStdString()<<'\n';
      check(!invalidCandidate.isEmpty() && editor->toPlainText()==source && editor->document()->availableUndoSteps()==steps && !object("BadCells"),"Rejected interpreted candidate restores source/model/Undo"); unchanged("Rejected candidate and model restoration preserve raw path alias");
      check(initialized("Mask"),"Restored relative file FOV initializes");
      alias(); editor->setPlainText(source+"Create MissingResource Broken;\n");
      check(!window.buildScript(),"Invalid build is rejected"); unchanged("Failed build preserves raw path alias");
      editor->setPlainText(source); check(window.buildScript(),"Existing file-backed source rebuilds after failed build"); unchanged("Rebuild preserves raw path alias");
      check(initialized("Mask"),"Rebuilt relative file FOV initializes");
      const auto saved=documents.filePath("saved FOV γ.script");
      check(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==source,"Relative FOV save/reopen retains exact source"); unchanged("Save/reopen preserves raw path alias");
      check(initialized("Mask"),"Reopened relative file FOV initializes from configured fallback");
      std::cout<<(failures ? "FAIL " : "PASS ")<<"CustomFOV paths: "<<failures<<" failures; no mission propagation, numerical algorithm or host qualification.\n";
      return failures ? 1 : 0;
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
