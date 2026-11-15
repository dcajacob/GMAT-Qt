#pragma once
#include "ListenerManager.hpp"
#include <memory>
#include <map>
#include <QJsonArray>
class QMdiArea;
class QtSolverListener;

class QtSolverListenerManager final : public ListenerManager
{
public:
   explicit QtSolverListenerManager(QMdiArea *workspace);
   ~QtSolverListenerManager() override;
   QJsonArray snapshot() const;
   void missionStarted();
   void missionFinished(bool stopped,bool failed);
   ISolverListener *CreateSolverListener(const std::string &name,const std::string &oldName,
      Real x,Real y,Real width,Real height,bool maximized) override;
private:
   QMdiArea *workspace;
   // Engine commands hold non-owning listener pointers. Keep the listener alive
   // even if its window is closed; no widget or engine pointer is owned by it.
   std::map<std::string,std::unique_ptr<QtSolverListener>> listeners;
};
