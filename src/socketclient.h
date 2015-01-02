#ifndef SOCKETCLIENT_H
#define SOCKETCLIENT_H

#include <QWebFrame>
#include <QTcpSocket>
#include <QString>
#include <QWaitCondition>
#include <QMutex>

#include "phantom.h"
#include "webpage.h"
#include "main.h"
#include "socketserver.h"
#include "utils.h"

class SocketClient : public QObject
{
  Q_OBJECT

public:
  SocketClient(QThread *thread, QTcpSocket *client);
  ~SocketClient();

public:
  void setup(Main *main, SocketServer *socketServer);

  SocketServer *socketServer;

    // Causes the current thread to sleep for msecs milliseconds.
    static void msleep(unsigned long msecs)
    {
        QMutex mutex;
        mutex.lock();
 
        QWaitCondition waitCondition;
        waitCondition.wait(&mutex, msecs);
 
        mutex.unlock();
    }

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
  QTcpSocket *client_socket;

};
 
#endif
