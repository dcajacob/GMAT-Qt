#include "GetLastState.hpp"
#include "ObjectWrapper.hpp"
#include "Spacecraft.hpp"
#include "Array.hpp"
#include "FileManager.hpp"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>

int main(int argc, char **argv)
{
   QJsonArray rows;
   int failed = 0;
   try
   {
      if (argc != 2)
         throw std::runtime_error("Supply startup file");
      FileManager::Instance()->ReadStartupFile(argv[1]);
      const std::array<Real, 6> literal{7100,20,-30,-.125,7.25,.375};
      for (int mode = 0; mode < 3; ++mode)
      {
         Spacecraft actual("Actual"), decoy("FormalInput");
         actual.GetState().SetState(literal.data(), 6);
         std::array<Real, 6> other{};
         if (mode == 2)
            other = {-8,-9,-10,-11,-12,-13};
         decoy.GetState().SetState(other.data(), 6);
         GetLastState function("GetLastState", "Binding");
         const auto input = function.GetStringParameter("Input", 0);
         const auto outputName = function.GetStringParameter("Output", 0);
         Array formalOutput(outputName);
         formalOutput.SetSize(6, 1);
         ObjectMap store{{input, mode == 0 ? &actual : &decoy}, {outputName, &formalOutput}};
         ObjectWrapper wrapper;
         wrapper.SetDescription("Actual");
         if (!wrapper.SetRefObject(&actual) || !function.SetInputElementWrapper(input, &wrapper))
            throw std::runtime_error("Input wrapper binding failed");
         function.SetFunctionObjectMap(&store);
         if (!function.Execute(nullptr))
            throw std::runtime_error("GetLastState execution failed");
         std::unique_ptr<ElementWrapper> output(function.GetOutputArgument(0));
         const bool storedOutput = output->GetRefObject() == &formalOutput;
         std::unique_ptr<GmatBase> detachedOutput(storedOutput ? nullptr : output->GetRefObject());
         const auto matrix = output->EvaluateArray();
         bool passed = storedOutput && matrix.GetNumRows() == 6 && matrix.GetNumColumns() == 1;
         QJsonArray observed, expected, placeholder;
         for (int i = 0; i < 6; ++i)
         {
            observed.append(matrix(i,0));
            expected.append(literal[i]);
            placeholder.append(mode == 0 ? literal[i] : other[i]);
            passed = passed && matrix(i,0) == literal[i] && formalOutput.GetRmatrix()(i,0) == literal[i] && actual.GetState()[i] == literal[i] && decoy.GetState()[i] == other[i];
         }
         if (!passed)
            ++failed;
         rows.append(QJsonObject{{"mode",mode},{"same_object",mode == 0},{"expected",expected},{"formal_placeholder",placeholder},{"observed",observed},{"stored_output_identity",storedOutput},{"passed",passed}});
      }
      const QJsonObject proof{{"checkpoint","get-last-state-bound-object"},{"outcome",failed ? "failed" : "passed"},{"controls",3},{"failed_controls",failed},{"rows",rows},{"scope","Actual ObjectWrapper reference versus distinct formal-placeholder states; six literal components and unchanged source/decoy. No propagation,frame transformation,manager ownership or universal qualification."}};
      const auto directory = qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      if (!directory.isEmpty())
      {
         QFile file(QDir(directory).filePath("proof.json"));
         if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(proof).toJson()) <= 0)
            throw std::runtime_error("Binding proof write failed");
      }
      std::cout << "NASA_CHECK " << QJsonDocument(proof).toJson(QJsonDocument::Compact).constData() << std::endl;
      return failed ? 1 : 0;
   }
   catch (BaseException &e)
   {
      std::cerr << e.GetFullMessage() << std::endl;
   }
   catch (std::exception &e)
   {
      std::cerr << e.what() << std::endl;
   }
   return 1;
}
