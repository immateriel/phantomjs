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
  cout << "WARNING: SocketServer setup" << endl;

  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF8"));

  //  QMetaObject::invokeMethod(main, "create_new_phantomjs", Qt::DirectConnection);
  this->main = main;
}

void SocketServer::sendConsoleMessage(const QString &message)
{
  cout << "WARNING: outdated sendConsoleMessage" << endl;
  cout << "WARNING: Allez on l'envoit au client ce message" << endl;
  cout << "WARNING: message: '" << message.toUtf8().data() << "'" << endl;
}

void SocketServer::client_disconnected()
{
  cout << "WARNING: outdated client_disconnected" << endl;
#if 0
  cout << "Le client il est parti" << endl;
  client_socket = NULL;
#endif
}

void SocketServer::doWork()
{
  cout << "SocketServer doWork..." << endl;

  QTcpServer server;
  bool status = server.listen(QHostAddress::Any, 12000);
  if (!status)
    {
      cerr << "Can't open TCP Server." << endl;
      exit(1);
    }

  while (true)
    {

      status = server.waitForNewConnection(-1);
      if (status)
	{
	  cout << "New connection available." << endl;
	  client_socket = server.nextPendingConnection();
	  if (client_socket == NULL)
	    {
	      cout << "WARNING: client socket is NULL." << endl;
	    }
	  else
	    {
	      QThread *thread = new QThread();
	      thread->start();

	      cout << "...1" << endl;
	      SocketClient *socketClient = new SocketClient();
	      socketClient->moveToThread(thread);

	      cout << "...2" << endl;
	      socketClient->client_socket = client_socket;
	      cout << "...3" << endl;
	      //	      Phantom *phantom = new Phantom();
	      //	      phantom->init();

	      //	      socketClient->setup(main->create_new_phantomjs());
	      socketClient->setup(main);
	      cout << "...4" << endl;
	      QMetaObject::invokeMethod(socketClient, "doWork", Qt::QueuedConnection);
	      cout << "...5" << endl;
	      cout << "client pris en charge" << endl;

	    }
	}
      else
	{
	  cout << "Problème waiting for a new connection." << endl;
	}

    }
}
