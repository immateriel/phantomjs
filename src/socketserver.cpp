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

SocketServer::SocketServer(QObject *parent)
{
    Utils::printDebugMessages = true;

}

SocketServer::~SocketServer()
{
}

void SocketServer::setup(Main *main,const QString &sHost, uint sPort)
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
  qDebug() << "SocketServer: listening for connection on"<<this->serverHost<<":"<<this->serverPort;

  QTcpServer server;
  bool status = server.listen(QHostAddress(this->serverHost), this->serverPort);

  if (!status)
    {
      qDebug() << "SocketServer: can't open TCP Server.";
      exit(1);
    }

  while (true)
    {

      status = server.waitForNewConnection(-1);
      if (status)
	{
	  qDebug() << "SocketServer: new connection available";
	  client_socket = server.nextPendingConnection();
	  if (client_socket == NULL)
	    {
	      qDebug() << "SocketServer: client socket is NULL";
	    }
	  else
	    {
	      QThread *thread = new QThread();

//		  client_socket->moveToThread(thread);
	      thread->start();

	      SocketClient *socketClient = new SocketClient(thread);

	      quint64 thread_id = (quint64) (void*)socketClient;

	      threadInstancesMap[thread_id] = thread;

	      qDebug() << "SocketServer: add thread instance"<<thread_id<<", total: " << threadInstancesMap.size();

	      socketClient->moveToThread(thread);

	      socketClient->client_socket = client_socket;

	      socketClient->setup(main, this);
	      QMetaObject::invokeMethod(socketClient, "doWork", Qt::QueuedConnection);

	    }
	}
      else
	{
	  qDebug() << "SocketServer: problem waiting for a new connection";
	}

    }
}

void SocketServer::deleteThreadInstance(quint64 threadId)
{
	
  QThread *thread = threadInstancesMap[threadId];
  threadInstancesMap.remove(threadId);

  qDebug() << "SocketServer: delete thread instance"<<threadId<<", total: " << threadInstancesMap.size();
  	  
  thread->quit();
}
