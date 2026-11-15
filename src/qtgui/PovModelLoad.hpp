// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <QString>
#include <functional>
#include <memory>
#include <osg/Node>
#include <osg/ref_ptr>

class QObject;

// Owned by a GUI caller. Only a detached CPU model crosses back to that
// caller's thread; the worker never accesses a widget or a live scene graph.
// Destroying the handle cancels work without waiting on the GUI thread.
class PovModelLoad final {
public:
   using Completion=std::function<void(osg::ref_ptr<osg::Node>,const QString &)>;
   PovModelLoad(const QString &path,bool normalize,QObject *context,Completion completed);
   ~PovModelLoad();
   PovModelLoad(const PovModelLoad &)=delete;
   PovModelLoad &operator=(const PovModelLoad &)=delete;
private:
   struct State;
   std::shared_ptr<State> state;
};
