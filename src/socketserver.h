#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#include <QWebFrame>
#include <QTcpSocket>
#include <QString>

class SocketServer : public QObject
{
  Q_OBJECT

public:
  SocketServer();
  ~SocketServer();

public:
  void setup(QWebFrame *webframe);

public slots:
  void doWork();
  void client_disconnected();
  void sendConsoleMessage(const QString &message);

signals:
  void evaluateJavaScript(const QString &code);

private:
  QTcpSocket *client_socket;
  QWebFrame *webframe;

};
 
#endif
