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
}

SocketServer::~SocketServer()
{
}

void SocketServer::setup(Main *main)
{
  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF8"));
  this->main = main;
}

void SocketServer::sendConsoleMessage(const QString &message)
{
}

void SocketServer::doWork()
{
  cout << "SocketServer: doWork" << endl;

  QTcpServer server;
  bool status = server.listen(QHostAddress::Any, 12000);
  if (!status)
    {
      cerr << "SocketServer: Can't open TCP Server." << endl;
      exit(1);
    }

  while (true)
    {

      status = server.waitForNewConnection(-1);
      if (status)
	{
	  cout << "SocketServer: New connection available." << endl;
	  client_socket = server.nextPendingConnection();
	  if (client_socket == NULL)
	    {
	      cout << "WARNING: client socket is NULL." << endl;
	    }
	  else
	    {
	      QThread *thread = new QThread();

	      thread->start();

	      SocketClient *socketClient = new SocketClient(thread);

	      quint64 thread_id = (quint64) (void*)socketClient;
	      threadInstancesMap[thread_id] = thread;

	      socketClient->moveToThread(thread);

	      socketClient->client_socket = client_socket;

	      socketClient->setup(main, this);
	      QMetaObject::invokeMethod(socketClient, "doWork", Qt::QueuedConnection);

	    }
	}
      else
	{
	  cout << "SocketServer: Problem waiting for a new connection." << endl;
	}

    }
}

void SocketServer::deleteThreadInstance(quint64 threadId)
{
  cout << "SocketServer: deleteThreadInstance() for threadId" << threadId << endl;
  QThread *thread = threadInstancesMap[threadId];
  threadInstancesMap.remove(threadId);
  thread->quit();
}
