#include "ICRFFile.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;
int checks = 0;
void require(bool ok, const char *message)
{
   ++checks;
   if (!ok) throw std::runtime_error(message);
}
struct Scratch
{
   fs::path path;
   bool retained;
   Scratch()
   {
      const char *output = std::getenv("GMAT_QT_ACCEPTANCE_DIR");
      retained = output && *output;
      path = retained ? fs::path(output) / "icrf-fixtures" :
         fs::temp_directory_path() / ("gmat-icrf-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
      require(fs::create_directories(path), "ICRF fixture directory already exists");
   }
   ~Scratch()
   {
      if (!retained) { std::error_code ignored; fs::remove_all(path, ignored); }
   }
};
void write(const fs::path &path, const std::string &text)
{
   std::ofstream file(path);
   file << text;
   require(bool(file), "Cannot write ICRF fixture");
}
std::array<double, 3> linear(double epoch)
{
   const double t = epoch - 21545.;
   return {std::ldexp(1., -20) + t * std::ldexp(1., -30),
           std::ldexp(1., -21) + t * std::ldexp(1., -29),
          -std::ldexp(1., -22) + t * std::ldexp(1., -31)};
}
void table(const fs::path &path, double constantAngle = -1.)
{
   std::ofstream file(path);
   file << std::setprecision(17);
   // More than six initial buffers, exercising three growth transitions.
   for (int i = 0; i < 769; ++i)
   {
      const double epoch = 21545. + i / 8.;
      const auto value = constantAngle < 0 ? linear(epoch) :
         std::array<double, 3>{constantAngle, 0., 0.};
      file << epoch << ", " << value[0] << ", " << value[1] << ", " << value[2] << '\n';
   }
   require(bool(file), "Cannot write complete ICRF growth table");
}
void select(ICRFFile *loader, const fs::path &path)
{
   loader->Finalize();
   FileManager::Instance()->AddFileType("ICRF_FILE", path.string());
}
void verifyLinear(ICRFFile *loader)
{
   loader->Initialize();
   loader->Initialize(); // Initialization is idempotent.
   for (double elapsed : {0., .5, 13.25, 95.75})
   {
      const double epoch = 21545. + elapsed;
      double actual[3];
      require(loader->GetICRFRotationVector(epoch, actual, 3, 9), "ICRF interpolation failed");
      const auto expected = linear(epoch);
      for (int i = 0; i < 3; ++i)
         require(std::isfinite(actual[i]) && std::abs(actual[i] - expected[i]) < 1e-18,
            "ICRF growth/reload changed the independent linear interpolation");
   }
}
size_t descriptors()
{
#ifdef __linux__
   size_t count = 0;
   for (const auto &entry : fs::directory_iterator("/proc/self/fd")) { (void)entry; ++count; }
   return count;
#else
   return 0;
#endif
}
}

int main(int argc, char **argv)
{
   try
   {
      require(argc == 2, "Supply ICRF lifecycle startup");
      const auto startup = fs::absolute(argv[1]);
      fs::current_path(startup.parent_path());
      auto *manager = FileManager::Instance();
      manager->ReadStartupFile(startup.string());
      const auto original = manager->GetFullPathname(FileManager::ICRF_FILE);
      auto *loader = ICRFFile::Instance();
      Scratch scratch;
      const auto valid = scratch.path / "linear.csv";
      table(valid);
      select(loader, valid);
      verifyLinear(loader);
      const auto before = descriptors();
      for (int i = 0; i < 20; ++i)
      {
         select(loader, valid);
         verifyLinear(loader);
         loader->Finalize();
         loader->Finalize(); // Cleanup must also be idempotent.
      }
      require(descriptors() == before, "ICRF reload leaked file descriptors");

      // Reloading at the same epoch must invalidate the previous rotation cache.
      const Real input[6] = {0., 1., 0., 0., 0., 0.};
      for (double angle : {1e-6, 2e-6})
      {
         const auto file = scratch.path / (angle == 1e-6 ? "first.csv" : "second.csv");
         table(file, angle);
         select(loader, file);
         Real output[6];
         loader->RotateFromICRFtoFK5(input, output, 21550.);
         require(std::abs(output[0]) < 1e-14 &&
                 std::abs(output[1] - std::cos(angle)) < 1e-14 &&
                 std::abs(output[2] - std::sin(angle)) < 1e-14,
            "ICRF reload reused an old same-epoch rotation");
      }

      const std::vector<std::string> invalid = {
         "", "bad record\n", "21545, 1, 2\n", "21545, nan, 0, 0\n",
         "21545, 1, 0, 0\n21545, 2, 0, 0\n",
         "21546, 1, 0, 0\n21545, 2, 0, 0\n"};
      int rejections = 0;
      for (size_t i = 0; i <= invalid.size(); ++i)
      {
         const auto file = scratch.path / ("invalid-" + std::to_string(i) + ".csv");
         if (i < invalid.size()) write(file, invalid[i]); // Last path is absent.
         select(loader, file);
         bool rejected = false;
         try { loader->Initialize(); } catch (BaseException &) { rejected = true; }
         require(rejected, "Invalid ICRF input was accepted");
         ++rejections;
         // Recovery must clean any partial input without requiring Finalize.
         manager->AddFileType("ICRF_FILE", valid.string());
         verifyLinear(loader);
      }
      select(loader, original);
      loader->Initialize(); // Also exercise the complete shipped table.
      Real actual[3];
      require(loader->GetICRFRotationVector(21545., actual, 3, 9), "Shipped ICRF table failed");
      for (Real value : actual) require(std::isfinite(value), "Shipped ICRF table is nonfinite");
      loader->Finalize();
      std::cout << "PASS ICRF growth/interpolation,20reloads,closedfiles,sameepochcache,"
                << rejections << " invalid/recovery cases and shipped table;checks=" << checks << '\n';
      return 0;
   }
   catch (BaseException &error) { std::cerr << "FAIL " << error.GetFullMessage() << '\n'; }
   catch (std::exception &error) { std::cerr << "FAIL " << error.what() << '\n'; }
   return 1;
}
