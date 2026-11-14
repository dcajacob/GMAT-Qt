#pragma once
#include <QObject>
#include <QTcpServer>
#include <QJsonObject>
#include <QHash>
#include "MatlabCallbackBridge.hpp"
class QTcpSocket;

// Optimizer evaluations and fixed iteration notifications. This service does not accept MATLAB-to-GMAT
// scripts, object mutation requests, or arbitrary command strings.
class QtMatlabCallbackServer final : public QObject
{
public:
   QtMatlabCallbackServer();
   ~QtMatlabCallbackServer() override;
   void start();
private:
   QJsonObject evaluate(const QJsonObject &request,QTcpSocket *socket);
   void processSocket(QTcpSocket *socket);
   MatlabCallbackReply runMatlabJob(const MatlabCallbackRequest &request,QTcpSocket *socket,
      double requestId,const std::function<void()> &validate);
   struct PendingMatlabJob;
   QHash<QString,PendingMatlabJob*> matlabJobs;
   QTcpServer server;
   QByteArray published;
   QString token;
   int processing = 0,matlabWaits = 0;
};
