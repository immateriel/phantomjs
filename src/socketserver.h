#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#include <QWebFrame>
#include <QTcpSocket>
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
  void setup(Main *main);

public slots:
  void doWork();
  void client_disconnected();
  void sendConsoleMessage(const QString &message);
  void deleteThreadInstance(quint64 threadId);

signals:
  void evaluateJavaScript(const QString &code);

private:
  Phantom *phantom;
  Main *main;
  QTcpSocket *client_socket;
  QWebFrame *webframe;
  QMap <quint64, QThread*> threadInstancesMap;

};
 
#endif
