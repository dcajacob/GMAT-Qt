#include "Moderator.hpp"
#include "FactoryManager.hpp"
#include "GmatBase.hpp"
#include "Interface.hpp"
#include "BaseException.hpp"
#include "ResourceDraft.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
void write(const QString &path, const QByteArray &bytes) {
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size(), "Service ownership evidence write failed");
}
QJsonArray names(const StringArray &values) {
   QJsonArray result;
   for (const auto &value : values) result.append(QString::fromStdString(value));
   return result;
}
bool contains(const StringArray &values, const std::string &name) {
   return std::find(values.begin(), values.end(), name) != values.end();
}
}

int main(int argc, char **argv) {
   QCoreApplication app(argc, argv);
   const auto directory = qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   QJsonObject proof{{"passed", false}};
   try {
      require(argc == 3 && !directory.isEmpty(), "Supply startup, service ownership case and output directory");
      const QString selected = argv[2];
      proof["case"] = selected;
      const auto startup = QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      auto *moderator = Moderator::Instance();
      require(moderator->Initialize(startup.toStdString(), false), "Native service runtime unavailable");
      write(qEnvironmentVariable("GMAT_SOLVER_MAP_READY"), QByteArray::number(QCoreApplication::applicationPid()));
      QElapsedTimer timer;
      timer.start();
      while (!QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")) && timer.elapsed() < 10000) QThread::msleep(5);
      require(QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")), "Native service images not acknowledged");

      GmatBase *service = moderator->GetMatlabInterface();
      require(service && service->GetTypeName() == "MatlabInterface", "Loaded MATLAB service is absent");
      const auto originalName = service->GetName();
      const auto originalMode = service->GetIntegerParameter("MatlabMode");
      proof["service_name"] = QString::fromStdString(originalName);
      proof["service_mode"] = originalMode;
      if (selected == "FactoryView") {
         const StringArray all = moderator->GetListOfFactoryItems(Gmat::INTERFACE);
         const StringArray visible = moderator->GetListOfViewableItems(Gmat::INTERFACE);
         const StringArray hidden = moderator->GetListOfUnviewableItems(Gmat::INTERFACE);
         GmatBase *borrowed = FactoryManager::Instance()->CreateInterface("MatlabInterface", "NativeService");
         // The baseline returns the owned process service. Never delete it.
         proof["factory_types"] = names(all);
         proof["viewable_types"] = names(visible);
         proof["unviewable_types"] = names(hidden);
         proof["typed_factory_returns_process_service"] = borrowed == service;
         require(borrowed == service && contains(all, "MatlabInterface"), "Native service factory identity changed");
         require(!contains(visible, "MatlabInterface") && contains(hidden, "MatlabInterface"), "Process service is advertised as an independently owned GUI resource");
      } else if (selected == "ScriptCreate") {
         const auto path = directory + "/named-service.script";
         write(path, "Create MatlabInterface Rogue;\nBeginMissionSequence;\n");
         const bool accepted = moderator->InterpretScript(path.toStdString());
         proof["script_accepted"] = accepted;
         proof["configured_requested_name_is_service"] = moderator->GetConfiguredObject("Rogue") == service;
         require(!accepted, "Script creation accepted the process service as a named Resource");
      } else if (selected == "ModeratorCreate" || selected == "OtherCreator" || selected == "GenericCreator") {
         GmatBase *created = nullptr;
         QString error;
         try {
            if (selected == "ModeratorCreate")
               created = moderator->CreateObject(Gmat::INTERFACE, "MatlabInterface", "Rogue", false);
            else if (selected == "OtherCreator")
               created = moderator->CreateOtherObject(Gmat::INTERFACE, "MatlabInterface", "Rogue", false);
            else
               created = FactoryManager::Instance()->CreateObject(Gmat::INTERFACE, "MatlabInterface", "Rogue");
         } catch (BaseException &e) { error = QString::fromStdString(e.GetFullMessage()); }
         proof["creation_error"] = error;
         proof["returned_process_service"] = created == service;
         proof["returned_object"] = created != nullptr;
         require(!created && !error.isEmpty(), "Native named-resource creation returned the process service");
      } else if (selected == "DraftOwnership") {
         QString error;
         bool returned = false, borrowed = false;
         try {
            auto draft = resourceDraft("MatlabInterface", "IndependentDraft");
            returned = bool(draft);
            borrowed = draft.get() == service;
            // Expose the erroneous ownership without destroying the baseline service.
            if (borrowed) draft.release();
         } catch (BaseException &e) { error = QString::fromStdString(e.GetFullMessage()); }
           catch (std::exception &e) { error = QString::fromUtf8(e.what()); }
         proof["draft_returned"] = returned;
         proof["draft_adopted_service"] = borrowed;
         proof["draft_error"] = error;
         require(!returned && !borrowed && !error.isEmpty(), "GUI draft adopted the process service as deletable preview storage");
         auto variable = resourceDraft("Variable", "IndependentVariable");
         require(variable && variable.get() != service && variable->GetTypeName() == "Variable" && variable->GetName() == "IndependentVariable", "Ordinary independently owned drafts no longer work");
         proof["ordinary_variable_draft"] = true;
      } else throw std::runtime_error("Unknown service ownership case");
      require(moderator->GetMatlabInterface() == service && service->GetName() == originalName && service->GetIntegerParameter("MatlabMode") == originalMode, "Resource rejection changed the actual service identity, name or mode");
      proof["service_identity_name_mode_unchanged"] = true;
      proof["engine_open_calls"] = 0;
      proof["scope"] = "Native process service versus independently owned resource boundaries. No engine invocation, session-mode, callback, physics, platform or whole Resource qualification.";
      proof["passed"] = true;
   } catch (BaseException &e) { proof["error"] = QString::fromStdString(e.GetFullMessage()); }
     catch (std::exception &e) { proof["error"] = QString::fromUtf8(e.what()); }
   write(QDir(directory).filePath("proof.json"), QJsonDocument(proof).toJson());
   std::cout << QJsonDocument(proof).toJson(QJsonDocument::Compact).constData() << std::endl;
   return proof["passed"].toBool() ? 0 : 1;
}
