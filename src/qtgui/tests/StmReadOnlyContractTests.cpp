#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "Parameter.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace
{
void require(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
void save(const QString &path, const QByteArray &bytes)
{
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size(), "Cannot retain STM contract evidence");
}
bool same(const Rmatrix &a, const Rmatrix &b)
{
   if (a.GetNumRows() != b.GetNumRows() || a.GetNumColumns() != b.GetNumColumns()) return false;
   for (Integer i = 0; i < a.GetNumRows(); ++i)
      for (Integer j = 0; j < a.GetNumColumns(); ++j)
         if (a(i,j) != b(i,j)) return false;
   return true;
}
QJsonArray array(const Rmatrix &matrix)
{
   QJsonArray result;
   for (Integer i = 0; i < matrix.GetNumRows(); ++i)
   {
      QJsonArray row;
      for (Integer j = 0; j < matrix.GetNumColumns(); ++j) row.append(matrix(i,j));
      result.append(row);
   }
   return result;
}
}

int main(int argc, char **argv)
{
   QCoreApplication application(argc, argv);
   try
   {
      require(argc == 2, "Supply isolated startup file");
      const QString output = qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      require(!output.isEmpty(), "Supply STM evidence directory");
      const auto startup = QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      auto *moderator = Moderator::Instance();
      require(moderator->Initialize(startup.toStdString(), false), "Native STM contract runtime failed");
      const auto ready = qEnvironmentVariable("GMAT_STM_MAP_READY"), ack = qEnvironmentVariable("GMAT_STM_MAP_ACK");
      if (!ready.isEmpty())
      {
         save(ready, QByteArray::number(QCoreApplication::applicationPid()));
         QElapsedTimer timer; timer.start();
         while (!QFile::exists(ack) && timer.elapsed() < 10000) QThread::msleep(5);
         require(QFile::exists(ack), "STM native images were not acknowledged");
      }
      const auto script = QDir(output).filePath("mission.script");
      save(script, "Create Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nBeginMissionSequence;\n");
      require(moderator->InterpretScript(script.toStdString()), "STM contract script failed to build");
      auto *spacecraft = dynamic_cast<Spacecraft *>(moderator->GetConfiguredObject("Sat"));
      require(spacecraft, "STM owner is absent");
      const Rmatrix nativeBefore = spacecraft->GetRmatrixParameter("OrbitSTM");
      const QStringList types{"OrbitSTM", "OrbitSTMA", "OrbitSTMB", "OrbitSTMC", "OrbitSTMD"};
      QJsonArray results, typed;
      int failures = 0;
      for (const auto &type : types)
      {
         auto *original = moderator->CreateParameter(type.toStdString(), ("Sat." + type).toStdString(), "Sat", "", 1);
         require(original && original->Initialize(), "STM contract parameter failed to initialize");
         const Rmatrix truth = original->EvaluateRmatrix();
         std::unique_ptr<Parameter> snapshot(dynamic_cast<Parameter *>(original->Clone()));
         std::unique_ptr<Parameter> clone(dynamic_cast<Parameter *>(original->Clone()));
         std::unique_ptr<Parameter> copied(dynamic_cast<Parameter *>(original->Clone()));
         require(snapshot && clone && copied, "STM parameter clone failed");
         copied->Copy(original);
         const std::vector<std::pair<QString, Parameter *>> providers{
            {"original", original}, {"clone", clone.get()}, {"copy", copied.get()}};
         for (const auto &entry : providers)
         {
            Parameter *parameter = entry.second;
            require(same(parameter->GetRmatrix(), truth), "Clone/Copy changed cached STM value");
            Rmatrix replacement(truth.GetNumRows(), truth.GetNumColumns());
            for (Integer i = 0; i < replacement.GetNumRows(); ++i)
               for (Integer j = 0; j < replacement.GetNumColumns(); ++j) replacement(i,j) = 100 + 10*i + j;
            bool rejected = false;
            QString diagnostic;
            try { parameter->SetRmatrix(replacement); }
            catch (BaseException &error) { rejected = true; diagnostic = QString::fromStdString(error.GetFullMessage()); }
            const Rmatrix after = parameter->GetRmatrix();
            const bool unchanged = same(after, truth);
            const bool ownerUnchanged = same(spacecraft->GetRmatrixParameter("OrbitSTM"), nativeBefore);
            const bool descriptive = diagnostic.contains("read-only", Qt::CaseInsensitive) || diagnostic.contains("read only", Qt::CaseInsensitive);
            const bool passed = !parameter->IsSettable() && rejected && descriptive && unchanged && ownerUnchanged;
            results.append(QJsonObject{{"type", type}, {"provider", entry.first}, {"settable", parameter->IsSettable()},
               {"rejected", rejected}, {"diagnostic", diagnostic}, {"cached_before", array(truth)},
               {"cached_after", array(after)}, {"cached_unchanged", unchanged}, {"owner_unchanged", ownerUnchanged}, {"passed", passed}});
            failures += !passed;
            // Native value Copy is distinct from assigning a report parameter.
            // Preserve its existing use for cache/provider transport.
            parameter->Copy(snapshot.get());
            require(same(parameter->GetRmatrix(), truth), "Native readonly value Copy did not restore cache");
            require(same(parameter->EvaluateRmatrix(), truth), "Rejected setter changed later native evaluation");
         }
         bool rejected = false;
         QString diagnostic;
         try
         {
            if (type == "OrbitSTM") original->SetRmatrix66(Rmatrix66());
            else original->SetRmatrix33(Rmatrix33());
         }
         catch (BaseException &error) { rejected = true; diagnostic = QString::fromStdString(error.GetFullMessage()); }
         const bool unchanged = same(original->GetRmatrix(), truth) && same(spacecraft->GetRmatrixParameter("OrbitSTM"), nativeBefore);
         const bool passed = rejected && !diagnostic.isEmpty() && unchanged;
         typed.append(QJsonObject{{"type", type}, {"rejected", rejected}, {"unchanged", unchanged}, {"diagnostic", diagnostic}, {"passed", passed}});
         failures += !passed;
      }
      require(results.size() == 15 && typed.size() == 5, "STM contract case census differs");
      const QJsonObject proof{{"status", failures ? "failed" : "passed"}, {"generic_cases", results},
         {"typed_controls", typed}, {"failed_cases", failures}, {"native_missions", 0},
         {"scope", "Readonly native matrix setter/Clone/Copy contract only; selected-frame conversion remains a separate open obligation"}};
      save(QDir(output).filePath("proof.json"), QJsonDocument(proof).toJson());
      std::cout << "STM contract15generic5typed failures=" << failures << '\n';
      return failures ? 1 : 0;
   }
   catch (BaseException &error) { std::cerr << error.GetFullMessage(); }
   catch (std::exception &error) { std::cerr << error.what(); }
   return 1;
}
