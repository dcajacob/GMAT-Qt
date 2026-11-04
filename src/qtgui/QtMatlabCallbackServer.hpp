#pragma once
#include <QObject>
#include <QTcpServer>
#include <QJsonObject>

// Optimizer evaluations and fixed iteration notifications. This service does not accept MATLAB-to-GMAT
// scripts, object mutation requests, or arbitrary command strings.
class QtMatlabCallbackServer final : public QObject
{
public:
   QtMatlabCallbackServer();
   ~QtMatlabCallbackServer() override;
   void start();
private:
   QJsonObject evaluate(const QJsonObject &request);
   QTcpServer server;
   QByteArray published;
   QString token;
   bool processing = false;
};
