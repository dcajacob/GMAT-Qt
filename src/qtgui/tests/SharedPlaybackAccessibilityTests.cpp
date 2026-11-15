#define main retainedSharedPlaybackMain
#include "SharedPlaybackTests.cpp"
#undef main
#include <QAccessible>

int main(int argc, char **argv)
{
   QApplication app(argc, argv);
   try {
      require(argc == 2 && acceptanceWidgetPlatform(), "Supply the playback startup file");
      QApplication::setOrganizationName("GMATTests");
      QApplication::setApplicationName("SharedPlaybackAccessibility");
      TestSettings settings;
      const auto startup = QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window;
      require(window.initialize(startup), "Playback accessibility initialization failed");
      window.show();
      auto *button = window.findChild<QToolButton *>("allPlotReplayPlay");
      require(button && button->defaultAction(), "Shared playback button is missing");
      QStringList observedNames;
      bool labelsMatch = true;
      QObject::connect(button->defaultAction(), &QAction::toggled, &window, [&](bool playing) {
         auto *accessible = QAccessible::queryAccessibleInterface(button);
         const QString name = accessible ? accessible->text(QAccessible::Name) : QString();
         observedNames.append(name);
         labelsMatch = labelsMatch && name == (playing ? "Pause" : "Play");
      });
      sharedPlaybackButtonChecks(window);
      require(labelsMatch && observedNames.contains("Play") && observedNames.contains("Pause"),
         "Shared playback accessible name does not follow its Play/Pause action");
      std::cout << "PASS shared playback accessible names follow the actual Play/Pause action\n";
      return 0;
   } catch (const std::exception &error) {
      std::cerr << error.what() << '\n';
      return 1;
   }
}
