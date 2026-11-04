// Read-only diagnostic for already-frozen candidate and reference missions.
#include "MainWindow.hpp"
#include "ResourceProperties.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <set>
#include <iostream>

static QJsonObject rawProperties(GmatBase &object,int depth=0)
{
   QJsonObject values{{"@type",QString::fromStdString(object.GetTypeName())}};
   for (int id=0;id<object.GetParameterCount();++id) try {
      const auto name=QString::fromStdString(object.GetParameterText(id));
      switch (object.GetParameterType(id)) {
         case Gmat::REAL_TYPE: values[name]=object.GetRealParameter(id); break;
         case Gmat::INTEGER_TYPE: values[name]=object.GetIntegerParameter(id); break;
         case Gmat::BOOLEAN_TYPE: values[name]=object.GetBooleanParameter(id); break;
         case Gmat::STRING_TYPE: case Gmat::ENUMERATION_TYPE: case Gmat::OBJECT_TYPE: case Gmat::FILENAME_TYPE:
            values[name]=QString::fromStdString(object.GetStringParameter(id)); break;
         default: break;
      }
   } catch (...) { }
   if (depth<2) for (int i=0;i<object.GetOwnedObjectCount();++i) try {
      if (auto *owned=object.GetOwnedObject(i);owned && owned!=&object)
         values[QString("@owned-%1").arg(i)]=rawProperties(*owned,depth+1);
   } catch (...) { }
   return values;
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   if (argc!=4) return 2;
   try {
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(argv[2]).absolutePath());
      MainWindow window;
      if (!window.initialize(startup) || !window.loadScript(QFileInfo(argv[2]).absoluteFilePath()) ||
          !window.convertOpenFramesScript() || !window.buildScript()) return 3;
      QJsonObject objects;
      std::set<std::string> names;
      for (const auto type:{Gmat::SPACECRAFT,Gmat::HARDWARE,Gmat::PROP_SETUP,Gmat::BURN,Gmat::ODE_MODEL,
                           Gmat::COORDINATE_SYSTEM,Gmat::SOLVER,Gmat::SUBSCRIBER,Gmat::CALCULATED_POINT,Gmat::FUNCTION})
         for (const auto &name:Moderator::Instance()->GetListOfObjects(type)) names.insert(name);
      for (const auto &name:names) {
         auto *object=Moderator::Instance()->GetConfiguredObject(name);
         if (!object || (object->IsOfType(Gmat::PARAMETER) && !object->IsOfType("Variable") && !object->IsOfType("String") && !object->IsOfType("Array"))) continue;
         QJsonObject values{{"@type",QString::fromStdString(object->GetTypeName())}};
         values["@raw"]=rawProperties(*object);
         for (const auto &field:resourceProperties(*object)) values[field.name]=field.value;
         if (auto *sat=dynamic_cast<Spacecraft *>(object)) {
            values["@epoch_double"]=sat->GetEpoch();
            values["@epoch_precision"]=QString::fromStdString(sat->GetEpochGT().ToString());
            values["@precision_time"]=sat->HasPrecisionTime();
            const Rvector6 state=sat->GetState().GetState(); QJsonArray array;
            for (int i=0;i<6;++i) array.append(state[i]); values["@initial_internal_cartesian"]=array;
         }
         objects[QString::fromStdString(name)]=values;
      }
      QFile output(argv[3]);
      if (!output.open(QIODevice::WriteOnly)) return 4;
      output.write(QJsonDocument(objects).toJson()); return 0;
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; }
   return 1;
}
