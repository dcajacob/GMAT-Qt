// Packaging-only check: resolve and read a bundled model without SDK fallbacks.
#include <osg/ComputeBoundsVisitor>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <cmath>
#include <filesystem>
#include <iostream>

int main(int argc, char **argv)
{
   if (argc != 3) return 2;
   const auto contents = std::filesystem::canonical(argv[1]);
   const auto plugins = contents / "PlugIns";
   auto *registry = osgDB::Registry::instance();
   auto &paths = registry->getLibraryFilePathList();
   // Retain only the native bundle path discovered by OSG itself. This also
   // verifies automatic discovery of the flat Contents/PlugIns deployment.
   osgDB::FilePathList bundled;
   for (const auto &path : paths) {
      std::error_code error;
      if (std::filesystem::canonical(path, error) == plugins && !error)
         bundled.push_back(path);
   }
   paths = bundled;
   const auto found = osgDB::findLibraryFile(registry->createLibraryNameForExtension("3ds"));
   if (found.empty() || std::filesystem::canonical(found).parent_path() != plugins) {
      std::cerr << "3DS reader was not discovered inside the application bundle\n";
      return 3;
   }
   const auto model = osgDB::readRefNodeFile(argv[2]);
   if (!model) return 4;
   osg::ComputeBoundsVisitor bounds;
   model->accept(bounds);
   const auto box = bounds.getBoundingBox();
   if (!box.valid() || !std::isfinite(box.radius()) || box.radius() <= 0) return 5;
   std::cout << "Bundled 3DS reader passed: " << found << "; radius=" << box.radius() << '\n';
   return 0;
}
