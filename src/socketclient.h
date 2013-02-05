#ifndef SOCKETCLIENT_H
#define SOCKETCLIENT_H

#include <QWebFrame>
#include <QTcpSocket>
#include <QString>

#include "phantom.h"
#include "webpage.h"
#include "main.h"

class SocketClient : public QObject
{
  Q_OBJECT

public:
  SocketClient();
  ~SocketClient();

public:
  void setup(Main *main);
  QTcpSocket *client_socket;

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

};
 
#endif
