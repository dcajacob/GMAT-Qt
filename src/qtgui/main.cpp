#include "MainWindow.hpp"
#include "FunctionValidation.hpp"
#include <cstring>
#include <QApplication>
#include <QScreen>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <cstdio>
#include <QTimer>
#include <QPlainTextEdit>
#include <QStatusBar>
#include <QSettings>
int main(int argc, char **argv)
{
   if (argc>1 && std::strcmp(argv[1],"--validate-functions")==0) return qtFunctionValidationWorker(argc,argv);
   QApplication app(argc, argv);
   QApplication::setOrganizationName("GMAT");
   QApplication::setApplicationName("GMAT Qt 6");
   QCommandLineParser parser;
   parser.setApplicationDescription("GMAT Qt 6 desktop interface");
   parser.addHelpOption();
   parser.addOption({"startup", "GMAT startup file", "file"});
   parser.addOption({"screenshot", "Save a startup screenshot and exit", "file"});
   parser.addOption({"run", "Run the loaded mission after opening the window"});
   parser.addOption({"convert-views", "Convert supported OpenFrames viewer definitions to Qt OrbitView in memory; review before saving"});
   parser.addOption({"settings-dir", "Use an isolated INI settings directory", "directory"});
   parser.addPositionalArgument("script", "Optional GMAT script to open");
   parser.process(app);
   if (parser.isSet("settings-dir")) {
      const auto directory=QFileInfo(parser.value("settings-dir")).absoluteFilePath();
      if (!QDir().mkpath(directory)) { std::fprintf(stderr,"Cannot create settings directory.\n"); return 2; }
      QSettings::setDefaultFormat(QSettings::IniFormat);
      QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,directory);
      QSettings::setPath(QSettings::IniFormat,QSettings::SystemScope,directory+"/system");
   }
   QString defaultStartup=QCoreApplication::applicationDirPath()+"/gmat_startup_qt.txt";
#ifdef Q_OS_MACOS
   if (!QFileInfo::exists(defaultStartup))
      defaultStartup=QCoreApplication::applicationDirPath()+"/../../../gmat_startup_qt.txt";
#endif
   const QString startup = parser.isSet("startup") ? QFileInfo(parser.value("startup")).absoluteFilePath() : defaultStartup;
   const QString script = parser.positionalArguments().isEmpty() ? QString() : QFileInfo(parser.positionalArguments().first()).absoluteFilePath();
   const QString screenshot = parser.isSet("screenshot") ? QFileInfo(parser.value("screenshot")).absoluteFilePath() : QString();
   if (!QFileInfo::exists(startup) || (!script.isEmpty() && !QFileInfo::exists(script))) {
      std::fprintf(stderr, "Startup file or script does not exist.\n");
      return 2;
   }
   // Startup-relative entries are documented relative to the executable.
   // A saved startup file may live outside the installation directory.
   QDir::setCurrent(QCoreApplication::applicationDirPath());
   MainWindow window;
   window.show();
   const bool initialized = window.initialize(startup);
   bool loaded = true;
   if (initialized && !script.isEmpty()) loaded = window.loadScript(script) &&
      (!parser.isSet("convert-views") || window.convertOpenFramesScript()) && window.buildScript();
   QTimer::singleShot(0, &window, [&] {
      bool succeeded = initialized && loaded;
      if (succeeded && script.isEmpty() && !parser.isSet("run") && screenshot.isEmpty()) window.showWelcome(true);
      if (parser.isSet("run") && succeeded)
         succeeded = window.runMission() == MainWindow::RunResult::Completed;
      if (!succeeded) {
         // Preserve the same diagnostic the GUI shows for command-line batch
         // qualification, including conversion failures before interpretation.
         const auto status=window.statusBar()->currentMessage().toUtf8();
         if (!status.isEmpty()) std::fprintf(stderr,"%s\n",status.constData());
         if (auto *messages=window.findChild<QPlainTextEdit *>("messageWindow")) {
            const auto text=messages->toPlainText().toUtf8();
            if (!text.isEmpty()) std::fprintf(stderr,"%s\n",text.constData());
         }
      }
      if (!screenshot.isEmpty())
         QTimer::singleShot(200, &window, [&, succeeded] {
            // QWidget::grab can repaint a QOpenGLWidget without its composed
            // overlays. On X11 capture the displayed native window instead.
            const auto image=QGuiApplication::platformName()=="xcb" && window.screen()
               ? window.screen()->grabWindow(window.winId()) : window.grab();
            app.exit(image.save(screenshot) && succeeded ? 0 : 1);
         });
   });
   return app.exec();
}
