#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message)
{
   if (!condition)
      throw std::runtime_error(message);
}

void save(const QString &path, const QByteArray &bytes)
{
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size(),
           "Cannot write function bracket evidence");
}

QByteArray read(const QString &path)
{
   QFile file(path);
   require(file.open(QIODevice::ReadOnly), "Cannot read function bracket evidence");
   return file.readAll();
}
}

int main(int argc, char **argv)
{
   QApplication app(argc, argv);
   try
   {
      require(argc == 2 && acceptanceWidgetPlatform(), "Supply startup and native widget platform");
      TestSettings settings;
      const QString output = qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      require(!output.isEmpty(), "Supply function bracket evidence directory");
      const auto startup = QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      const QString tickPath = output + "/Tick.gmf";
      const QString pairPath = output + "/Pair.gmf";
      const QString onePath = output + "/One.gmf";
      const QByteArray tickSource = "function Tick(input)\nGlobal Shared;\n"
         "BeginMissionSequence;\nShared = Shared + input;\ninput = 999;\n";
      const QByteArray pairSource = "function [first, second] = Pair(input)\n"
         "Create Variable first second;\nBeginMissionSequence;\n"
         "first = input * 2;\nsecond = input - 1;\ninput = 999;\n";
      const QByteArray oneSource = "function [out] = One(input)\nCreate Variable out;\n"
         "BeginMissionSequence;\nout = input * 2;\n";
      save(tickPath,tickSource);
      save(pairPath,pairSource);
      save(onePath,oneSource);
      MainWindow window;
      require(window.initialize(startup), "Function bracket runtime did not initialize");
      window.show();
      QApplication::processEvents();
      acceptanceView(window);
      auto *editor = window.findChild<QPlainTextEdit*>("scriptEditor");
      auto *messages = window.findChild<QPlainTextEdit*>("messageWindow");
      require(editor && messages, "Function bracket GUI is unavailable");
      QJsonArray observations;
      int missions = 0;
      for (bool strict : {false,true})
      {
         for (const QString &kind : {QString("zero"),QString("multiple"),QString("unbracketed_multiple"),QString("array_element")})
         {
            const bool zero = kind == "zero";
            const bool array = kind == "array_element";
            const bool accepted = kind != "unbracketed_multiple" && !(strict && array);
            QString function = "Pair";
            QString functionPath = pairPath;
            if (zero)
            {
               function = "Tick";
               functionPath = tickPath;
            }
            else if (array)
            {
               function = "One";
               functionPath = onePath;
            }
            QString call;
            if (zero)
               call = "Tick(Input);";
            else if (array)
               call = "Cells(1, 2) = One(Input);";
            else if (kind == "multiple")
               call = "[A, B] = Pair(Input);";
            else
               call = "A, B = Pair(Input);";
            const QString key = (strict ? "strict_" : "native_") + kind;
            const QString report = output + "/" + key + ".csv";
            QString script = "Create Variable Input A B Shared Keep;\nInput = 3;\nA = -1;\n"
               "B = -2;\nShared = 5;\nKeep = 41;\nCreate Array Cells[1,2];\nCells(1,1) = 10;\nCells(1,2) = 11;\nCreate GmatFunction " + function + ";\n" +
               function + ".FunctionPath = '" + functionPath + "';\n" +
               function + ".RequireOutputBrackets = " + (strict ? "true" : "false") + ";\n"
               "Create ReportFile Values;\nValues.Filename = '" + report + "';\n"
               "Values.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\n"
               "Values.Precision = 17;\nBeginMissionSequence;\n" +
               (zero ? "Global Shared;\n" : "") + call +
               "\nKeep = 42;\nReport Values Input A B Shared Keep Cells(1,2);\n";
            editor->setPlainText(script);
            messages->clear();
            const bool built = window.buildScript();
            QApplication::processEvents();
            save(output + "/" + key + "-source.script",script.toUtf8());
            save(output + "/" + key + "-build.txt",messages->toPlainText().toUtf8());
            require(built == accepted, "Function bracket boundary acceptance differs");
            require(editor->toPlainText() == script, "Function build changed the source text");
            if (!accepted)
            {
               require(!messages->toPlainText().trimmed().isEmpty(), "Rejected syntax has no diagnostic");
               script.replace(call,array ? "[Cells(1, 2)] = One(Input);" : "[A, B] = Pair(Input);");
               editor->setPlainText(script);
               require(window.buildScript(), "Function bracket recovery build failed");
               save(output + "/" + key + "-recovery.script",script.toUtf8());
            }
            const auto runStatus = window.runMission();
            save(output + "/" + key + "-run.txt",messages->toPlainText().toUtf8());
            if (runStatus != MainWindow::RunResult::Completed)
               std::cerr << messages->toPlainText().toStdString();
            require(runStatus == MainWindow::RunResult::Completed, "Function bracket mission failed");
            ++missions;
            QJsonArray expected{3,zero || array ? -1 : 6,zero || array ? -2 : 2,zero ? 8 : 5,42};
            const QStringList names{"Input","A","B","Shared","Keep"};
            QJsonArray values;
            for (int index=0;index<names.size();++index)
            {
               auto *value = Moderator::Instance()->GetInternalObject(names[index].toStdString());
               require(value && value->GetRealParameter("Value") == expected[index].toDouble(),
                       "Function bracket numerical result differs");
               values.append(value->GetRealParameter("Value"));
            }
            auto *cells = Moderator::Instance()->GetInternalObject("Cells");
            require(cells != nullptr, "Array output is unavailable");
            const double cellValue = cells->GetRealParameter("SingleValue",0,1);
            expected.append(array ? 6 : 11);
            require(cellValue == expected.last().toDouble(), "Array-element output differs");
            values.append(cellValue);
            const auto fields = QString::fromUtf8(read(report)).trimmed().split(',');
            require(fields.size() == expected.size(), "Function bracket report shape differs");
            for (int index=0;index<fields.size();++index)
            {
               bool numeric = false;
               const double value = fields[index].toDouble(&numeric);
               require(numeric && value == expected[index].toDouble(), "Function bracket report differs");
            }
            require(read(tickPath) == tickSource && read(pairPath) == pairSource && read(onePath) == oneSource,
                    "Function call changed its definition file");
            observations.append(QJsonObject{{"case",kind},{"strict",strict},{"accepted",built},
               {"recovered",!accepted},{"values",values},{"source_preserved",true}});
         }
      }
      const QJsonObject proof{{"status","passed"},{"observations",observations},
         {"native_missions",missions},{"new_requirement_credit",0}};
#ifdef Q_OS_LINUX
      save(output + "/loaded-images.txt",read("/proc/self/maps"));
#endif
      save(output + "/proof.json",QJsonDocument(proof).toJson());
      std::cout << "NASA_CHECK " << QJsonDocument(proof).toJson(QJsonDocument::Compact).constData() << '\n';
      return 0;
   }
   catch (const BaseException &error)
   {
      std::cerr << error.GetFullMessage() << '\n';
   }
   catch (const std::exception &error)
   {
      std::cerr << error.what() << '\n';
   }
   return 1;
}
