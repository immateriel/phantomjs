#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#include <QWebFrame>
#include <QTcpSocket>
#include <QTcpServer>
#include <QString>

#include "phantom.h"
#include "main.h"

class SocketServer : public QObject
{
  Q_OBJECT

public:
  SocketServer(QObject *parent = 0);
  ~SocketServer();

public:
  void setup(Main *main,const QString &sHost, uint sPort);

public slots:
  void doWork();
  void sendConsoleMessage(const QString &message);
  void disconnect(quint64 threadId);

signals:
  void evaluateJavaScript(const QString &code);

private:
  Phantom *phantom;
  Main *main;
  QWebFrame *webframe;
  QMap <quint64, QThread*> threadInstancesMap;
  uint serverPort;
  QString serverHost;
  SocketClient *socketClient;
  QTcpServer *server;
};
 
#endif
