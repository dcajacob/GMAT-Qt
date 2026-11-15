#pragma once
#include <QObject>
#include <QPointer>
#include <QString>
#include <deque>
#include <functional>

class SpaceTrackAccount final : public QObject
{
public:
   struct Credentials { QString identity,password; };
   using Completion=std::function<void(const QString &error)>;
   explicit SpaceTrackAccount(QString storageKey="org.gmat.SpaceTrack",QObject *parent=nullptr);
   static SpaceTrackAccount &instance();
   Credentials credentials() const { return current; }
   void load(QObject *context,Completion complete);
   void save(Credentials credentials,QObject *context,Completion complete);
   void useForSession(Credentials credentials);
   void forget(QObject *context,Completion complete);
private:
   enum class Operation { Read,Write,Remove };
   struct Pending {
      Operation operation;
      Credentials credentials;
      QPointer<QObject> context;
      Completion complete;
      quint64 revision;
   };
   void run(Operation operation,Credentials credentials,QObject *context,Completion complete);
   void startNext();
   QString key;
   Credentials current;
   bool loaded=false;
   quint64 generation=0;
   std::deque<Pending> pending;
   bool running=false;
};
