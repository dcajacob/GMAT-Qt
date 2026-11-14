// Supplementary witnesses for holes in the saved command-context matrix.
#include "NativeMessageCapture.hpp"
#include "Moderator.hpp"
#include "GmatCommand.hpp"
#include "UserDefinedFunction.hpp"
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
#include <cmath>
#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
QByteArray read(const QString &path) {
   QFile f(path); require(f.open(QIODevice::ReadOnly), "Missing context evidence"); return f.readAll();
}
void write(const QString &path, const QByteArray &bytes) {
   QFile f(path); require(f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size(), "Cannot write context evidence");
}
}

int main(int argc, char **argv) {
   QCoreApplication app(argc, argv);
   try {
      require(argc == 4, "Supply startup, frozen matrix, and case");
      const auto output = qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      require(!output.isEmpty(), "Context evidence directory required");
      const auto plan = QJsonDocument::fromJson(read(argv[2])).object();
      QJsonObject spec;
      for (const auto &entry : plan["cases"].toArray())
         if (entry.toObject()["key"].toString() == argv[3]) spec = entry.toObject();
      require(!spec.isEmpty(), "Unknown context case");
      auto expand = [&](QString text) { return text.replace("@OUTPUT@", output); };
      const auto files = spec["files"].toObject();
      for (auto it = files.begin(); it != files.end(); ++it) {
         require(QFileInfo(it.key()).fileName() == it.key(), "Support file must be local");
         write(output + '/' + it.key(), expand(it.value().toString()).toUtf8());
      }
      const auto source = expand(spec["script"].toString());
      const auto mission = output + "/mission.script";
      write(mission, source.toUtf8());
      write(output + "/case.json", QJsonDocument(spec).toJson());
      const auto startup = QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      NativeMessageCapture messages;
      auto *engine = Moderator::Instance();
      require(engine->Initialize(startup.toStdString(), false), "Context runtime initialization failed");
      const auto ready = qEnvironmentVariable("GMAT_CONTEXT_MAP_READY");
      if (!ready.isEmpty()) {
         write(ready, QByteArray::number(QCoreApplication::applicationPid()));
         QElapsedTimer timer; timer.start();
         while (!QFile::exists(qEnvironmentVariable("GMAT_CONTEXT_MAP_ACK")) && timer.elapsed() < 10000) QThread::msleep(5);
         require(QFile::exists(qEnvironmentVariable("GMAT_CONTEXT_MAP_ACK")), "Context loaded-image receipt missing");
      }
      messages.text.clear();
      const bool built = engine->InterpretScript(mission.toStdString());
      write(output + "/build-messages.txt", QByteArray::fromStdString(messages.text));
      int status = -99;
      if (built) status = engine->RunMission();
      const auto diagnostic = QString::fromStdString(messages.text);
      write(output + "/messages.txt", diagnostic.toUtf8());
      QJsonObject proof{{"case", spec["key"]}, {"built", built}, {"run_status", status}, {"passed", false}};
      write(output + "/proof.json", QJsonDocument(proof).toJson());
      if (spec["reject"].toBool()) {
         require(!built || status != 1, "Illegal placement executed successfully");
         for (const auto &text : spec["diagnostic_contains"].toArray())
            require(diagnostic.contains(text.toString(), Qt::CaseInsensitive), "Rejection diagnostic did not identify the context rule");
         const auto recovery = expand(plan["recovery_script"].toString());
         write(output + "/recovery.script", recovery.toUtf8());
         messages.text.clear();
         require(engine->InterpretScript((output + "/recovery.script").toStdString()) && engine->RunMission() == 1,
                 "Rejected placement poisoned a subsequent valid mission");
         auto *keep = engine->GetInternalObject("Keep");
         require(keep && keep->GetRealParameter("Value") == 42, "Recovery mission did not execute");
         proof["recovery_keep"] = 42;
         proof["rejected"] = true;
         write(output + "/recovery-messages.txt", QByteArray::fromStdString(messages.text));
      } else {
         require(built && status == 1, "Valid command context did not complete");
         QJsonArray graph;
         std::set<GmatCommand *> seen;
         std::function<void(GmatCommand *, GmatCommand *)> walk = [&](GmatCommand *command, GmatCommand *owner) {
            while (command && command != owner && seen.insert(command).second) {
               graph.append(QString::fromStdString(command->GetTypeName()));
               for (int i = 0; auto *child = command->GetChildCommand(i); ++i) walk(child, command);
               if (command->IsOfType("CallFunction")) {
                  auto *function = dynamic_cast<UserDefinedFunction *>(command->GetRefObject(Gmat::FUNCTION, ""));
                  if (function) walk(function->GetFunctionControlSequence(), nullptr);
               }
               command = command->GetNextWhileExecuting();
            }
         };
         walk(engine->GetFirstCommand(), nullptr);
         for (const auto &type : spec["required_types"].toArray()) require(graph.contains(type), "Required executed command absent");
         proof["native_types"] = graph;
         QJsonObject observations;
         const auto expectedFiles = spec["expected_files"].toObject();
         for (auto it = expectedFiles.begin(); it != expectedFiles.end(); ++it) {
            const auto rows = QString::fromUtf8(read(output + '/' + it.key())).trimmed().split('\n');
            const auto wanted = it.value().toArray();
            require(rows.size() == wanted.size(), "Context output row count differs");
            QJsonArray actual;
            for (int row = 0; row < rows.size(); ++row) {
               const auto fields = rows[row].trimmed().split(',');
               const auto target = wanted[row].toArray();
               require(fields.size() == target.size(), "Context output column count differs");
               QJsonArray values;
               for (int column = 0; column < fields.size(); ++column) {
                  bool ok = false; const double value = fields[column].trimmed().toDouble(&ok);
                  require(ok && std::isfinite(value) && std::abs(value - target[column].toDouble()) <= spec["tolerance"].toDouble(),
                          "Context output differs from the frozen independent oracle");
                  values.append(value);
               }
               actual.append(values);
            }
            observations[it.key()] = actual;
         }
         const auto timeFiles = spec["time_files"].toObject();
         for (auto it = timeFiles.begin(); it != timeFiles.end(); ++it) {
            const auto rows = QString::fromUtf8(read(output + '/' + it.key())).trimmed().split('\n');
            const auto rule = it.value().toObject();
            const double tolerance = rule["tolerance"].toDouble();
            QJsonArray actual;
            require(tolerance > 0 && !rows.isEmpty(), "Missing automatic subscriber output");
            for (const auto &row : rows) {
               bool ok = false; const double value = row.trimmed().toDouble(&ok);
               bool allowed = false;
               for (const auto &interval : rule["allowed_intervals"].toArray()) {
                  const auto bounds = interval.toArray();
                  allowed |= value >= bounds[0].toDouble() - tolerance && value <= bounds[1].toDouble() + tolerance;
               }
               require(ok && std::isfinite(value) && allowed, "Inactive subscriber recorded a forbidden interval");
               actual.append(value);
            }
            for (const auto &wanted : rule["required_times"].toArray()) {
               bool present = false;
               for (const auto &value : actual) present |= std::abs(value.toDouble() - wanted.toDouble()) <= tolerance;
               require(present, "Active subscriber missed a required time");
            }
            observations[it.key()] = actual;
         }
         for (const auto &text : spec["message_contains"].toArray())
            require(diagnostic.contains(text.toString()), "Native Write output missing");
         proof["observed"] = observations;
      }
      require(read(mission) == source.toUtf8(), "Execution changed the input mission");
      for (auto it = files.begin(); it != files.end(); ++it)
         require(read(output + '/' + it.key()) == expand(it.value().toString()).toUtf8(), "Execution changed a function file");
      proof["source_files_unchanged"] = true;
      proof["passed"] = true;
      proof["new_requirement_credit"] = 0;
      write(output + "/proof.json", QJsonDocument(proof).toJson());
      std::cout << QJsonDocument(proof).toJson(QJsonDocument::Compact).constData() << '\n';
      return 0;
   } catch (BaseException &error) { std::cerr << error.GetFullMessage(); }
     catch (std::exception &error) { std::cerr << error.what(); }
   return 1;
}
