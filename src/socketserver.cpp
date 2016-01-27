#include "socketserver.h"

#include <iostream>
#include <QDir>
#include <QTcpServer>
#include <QTextCodec>
#include <QTcpSocket>
#include <QObject>
#include <QAbstractSocket>
#include <QWebPage>
#include <QThread>
#include <QTimer>
using namespace std;
#include <unistd.h>

#include "socketclient.h"
#include "phantom.h"
#include "webpage.h"
#include "main.h"
#include "utils.h"

/*
Protocol:

Request:
- A header line which is consists of the string "XXX\n" where XXX is
an integer, the length of the javascript code that follows and that
needs to be executed.
- Then the javascript string code.
 */

#define SOCKETSERVER_DEBUG

SocketServer::SocketServer(QObject *parent)
{
  Utils::printDebugMessages = true;
}

SocketServer::~SocketServer()
{
}

void SocketServer::setup(Main *main, const QString &sHost, uint sPort)
{
  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF8"));
  this->main = main;
  this->serverHost = sHost;
  this->serverPort = sPort;
}

void SocketServer::sendConsoleMessage(const QString &message)
{
}

void SocketServer::doWork()
{
  QThread *currentThread = QThread::currentThread();
  quint64 currentThreadId = (quint64) (void*)currentThread;

  server = new QTcpServer(this);

  bool status = server->listen(QHostAddress(this->serverHost), this->serverPort);

#ifdef SOCKETSERVER_DEBUG
  qDebug() << "SocketServer: listening for connection on" << this->serverHost << ":" << this->serverPort << "with thread" << QString("0x%1").arg(currentThreadId, 0, 16);
#endif

  if (!status)
  {
#ifdef SOCKETSERVER_DEBUG
    qDebug() << "SocketServer: can't open TCP Server.";
#endif
    exit(1);
  }

  while (true)
  {

    status = server->waitForNewConnection(-1);
    if (status)
    {
#ifdef SOCKETSERVER_DEBUG
      qDebug() << "SocketServer: new connection available";
#endif
      QTcpSocket *client = server->nextPendingConnection();
      if (client == NULL)
      {
#ifdef SOCKETSERVER_DEBUG
        qDebug() << "SocketServer: client socket is NULL";
#endif
      }
      else
      {
#ifdef MULTIPLE_THREADS

        QThread *thread = new QThread();
        socketClient = new SocketClient(thread, client);
        quint64 threadId = (quint64) (void*)thread;
        threadInstancesMap[threadId] = thread;
#ifdef SOCKETSERVER_DEBUG
        qDebug() << "SocketServer: add thread instance" << QString("0x%1").arg(threadId, 0, 16) << ", total: " << threadInstancesMap.size();
#endif

#else
        server->close();
        socketClient = new SocketClient(NULL, client);

#ifdef SOCKETSERVER_DEBUG
        qDebug() << "SocketServer: add instance";
#endif

#endif

        socketClient->setup(main, this);

#ifdef MULTIPLE_THREADS
        socketClient->moveToThread(thread);
        QMetaObject::invokeMethod(socketClient, "doWork", Qt::QueuedConnection);
        thread->start();
#else
        socketClient->doWork();
#endif

      }
    }
    else
    {
#ifdef SOCKETSERVER_DEBUG
      qDebug() << "SocketServer: problem waiting for a new connection";
#endif
    }

  }
}

void SocketServer::disconnect(quint64 threadId)
{
#ifdef MULTIPLE_THREADS

  QThread *thread = threadInstancesMap[threadId];
  threadInstancesMap.remove(threadId);

#ifdef SOCKETSERVER_DEBUG
  qDebug() << "SocketServer: delete thread instance" << QString("0x%1").arg(threadId, 0, 16) << ", total: " << threadInstancesMap.size();
#endif

  thread->quit();
#else
  delete socketClient;
  server->listen(QHostAddress(this->serverHost), this->serverPort);
#endif
}
