#ifndef SOCKETCLIENT_H
#define SOCKETCLIENT_H

#include <QWebFrame>
#include <QTcpSocket>
#include <QString>

#include "phantom.h"
#include "webpage.h"
#include "main.h"
#include "socketserver.h"

class SocketClient : public QObject
{
  Q_OBJECT

public:
  SocketClient(QThread *thread);
  ~SocketClient();

public:
  void setup(Main *main, SocketServer *socketServer);
  QTcpSocket *client_socket;
  SocketServer *socketServer;

public slots:
  void doWork();
  void client_disconnected();
  void sendConsoleMessage(const QString &message);
  void copyJsConsoleMessageToClientSocket(const QString &message);

signals:
  void evaluateJavaScript(const QString &code);

private:
  Main *main;
  Phantom *phantom;
  WebPage *webpage;
  quint64 threadId;
  QThread *thread;

};
 
#endif
