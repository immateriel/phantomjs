#include "socketclient.h"

#include <iostream>
#include <QDir>
#include <QTcpServer>
#include <QThread>
#include <QTextCodec>
#include <QTimer>
#include <QTcpSocket>
#include <QObject>
#include <QAbstractSocket>
using namespace std;
#include <unistd.h>

#include "phantom.h"
#include "cookiejar.h"
#include "networkaccessmanager.h"
#include "main.h"

//#define SOCKET_CLIENT_DEBUG

SocketClient::SocketClient(QThread *thread, QTcpSocket *client)
{
  this->thread = thread;
  this->client_socket = client;
  this->client_socket->setParent(this);
  // this->client_socket->moveToThread(thread);
}

SocketClient::~SocketClient()
{
}

void SocketClient::setup(Main *main, SocketServer *socketServer)
{
  this->main = main;
  this->socketServer = socketServer;

  this->threadId = (quint64) (void*)thread;

  QMetaObject::invokeMethod(main, "createPhantomJSInstance", Qt::QueuedConnection, Q_ARG(quint64, threadId));

  int tmp;
   
  while (!main->phantomInstancesMap.contains(threadId)){
#ifdef SOCKET_CLIENT_DEBUG	  
	qDebug()  << "SocketClient[" << threadId << "]: waiting for phantomjs instance";
#endif
    sleep(1);
    }

  this->phantom = main->phantomInstancesMap.value(threadId);
  this->phantom->socketClient = this;
  this->webpage = phantom->m_page;

#ifdef SOCKET_CLIENT_DEBUG
  qDebug() << "SocketClient[" << threadId << "]: setup";
#endif

  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF8"));

  connect(this->phantom->m_page, SIGNAL(javaScriptConsoleMessageSent(QString)),
	  this, SLOT(copyJsConsoleMessageToClientSocket(QString)),
	  Qt::QueuedConnection);

  //  connect(this->phantom->m_page, SIGNAL(javaScriptConsoleMessageSent(QString)),this,SLOT(sendConsoleMessage(QString)), Qt::DirectConnection);

      // Listen for Phantom exit(ing)
  connect(this->phantom, SIGNAL(aboutToExit(int)), this, SLOT(client_disconnected(void)), Qt::QueuedConnection);



}

void SocketClient::sendConsoleMessage(const QString &message)
{
#ifdef SOCKET_CLIENT_DEBUG
  qDebug() << "SocketClient[" << threadId << "]: send response « " << message.toUtf8().data() << " »";
#endif
  if (client_socket != NULL)
    {

      client_socket->write(message.toUtf8().data());
      client_socket->write("\n");
    }
}

void SocketClient::copyJsConsoleMessageToClientSocket(const QString &message)
{
#ifdef SOCKET_CLIENT_DEBUG
  qDebug() << "SocketClient[" << threadId << "]: copy JS message";
  // message.toUtf8().data() << "'" << endl;
#endif

  if (client_socket != NULL)
    {
      client_socket->write(message.toUtf8().data());
      client_socket->write("\n");
    }
}


void SocketClient::client_disconnected()
{
#ifdef SOCKET_CLIENT_DEBUG
  qDebug() << "SocketClient[" << threadId << "]: client disconnected";
#endif
  if (client_socket != NULL)
    {
      client_socket->close();
    }

  QMetaObject::invokeMethod(main, "deletePhantomJSInstance", Qt::QueuedConnection, Q_ARG(quint64, threadId));

  socketServer->disconnect(threadId);
}

void SocketClient::doWork()
{
#ifdef SOCKET_CLIENT_DEBUG
  qDebug() << "SocketClient[" << threadId << "]: do work";
#endif
  
  /* Read all data till all the input is read */
  QByteArray request_data;
  qint64 max_nb_bytes_by_read = 80;
  char buf[max_nb_bytes_by_read+1];

  QObject::connect(client_socket, SIGNAL(disconnected(void)),
  		   this, SLOT(client_disconnected(void)),
  		   Qt::QueuedConnection);

  /* While connected, wait for new request */
  while (client_socket != NULL)
    {
#ifdef SOCKET_CLIENT_DEBUG
      qDebug() << "SocketClient[" << threadId << "]: waiting for a new request of the client";
#endif

      /* First read the header which consists in a line of text 
	 telling how long in bytes is the request */
      while (true)
	{
//	  cout << "data read..." << endl;
	  qint64 status_read = client_socket->read(buf, max_nb_bytes_by_read);
	  if (status_read == 0)
	    {
#ifdef SOCKET_CLIENT_DEBUG
//	      cout << "SocketClient[" << threadId << "]: buf 0" << endl; 
#endif
	    }
	  else if (status_read == -1)
	    {
#ifdef SOCKET_CLIENT_DEBUG
//	      cout << "SocketClient[" << threadId << "]: status_read -1 error" << endl;
#endif
	    }
	  else
	    {
	      request_data.append(buf, status_read);
	    }

	  /* Once the newline character is read, the header has
	     been completely read. */
	  if (request_data.indexOf("\n") != -1)
	    {
#ifdef SOCKET_CLIENT_DEBUG
//	      cout << "SocketClient[" << threadId << "]: endline character found" << endl;
#endif
	      break;
	    }

#ifdef SOCKET_CLIENT_DEBUG
	  qDebug() << "SocketClient[" << threadId << "]: still waiting";
#endif

	  // if (status_read == 0 && !client_socket->waitForReadyRead())
	  //   break;

	  bool client_ready = false;

	  for (int k = 0; k < 10 && !client_ready; k++)
	    {

	      if (!client_socket->waitForReadyRead())
		{
#ifdef SOCKET_CLIENT_DEBUG
//		  cout << "SocketClient[" << threadId << "]: !client_socket->waitForReadyRead())" << endl;
		  qDebug() << "SocketClient[" << threadId << "]: still waiting";
#endif
		  // break;
		  //		  this->thread::msleep(100);
		  msleep(100);
		}
	      else
		{
		  client_ready = true;
#ifdef SOCKET_CLIENT_DEBUG
//		  cout << "SocketClient[" << threadId << "]: client_ready!" << endl;
#endif
		}

	    }
	  if (!client_ready)
	    {
#ifdef SOCKET_CLIENT_DEBUG
	      qDebug() << "SocketClient[" << threadId << "]: wait for too long, aborting";
#endif
	      break;
	    }

#ifdef SOCKET_CLIENT_DEBUG
//	  cout << "SocketClient[" << threadId << "]: waiting again" << endl;
#endif

	}
#ifdef SOCKET_CLIENT_DEBUG
//      cout << "SocketClient[" << threadId << "]: header end met" << endl;
#endif

      if (client_socket == NULL)
	{
#ifdef SOCKET_CLIENT_DEBUG
//	  cout << "SocketClient[" << threadId << "]: null socket" << endl;
#endif
	  client_disconnected();
	  break;
	}

      int index_newline;
      QByteArray header_data;
      int request_length = 0;
      if ((index_newline = request_data.indexOf("\n")) != -1)
	{
	  header_data = request_data.left(index_newline + 1);
//	  cout << "Header read: «" << header_data.data() << "» " << endl;
	  request_data  = request_data.right(request_data.size() - (index_newline + 1));
	  request_length = atoi(header_data.data());
#ifdef SOCKET_CLIENT_DEBUG
//	  cout << "SocketClient[" << threadId << "]: header read" << endl;
//	  cout << "SocketClient[" << threadId << "]: length of the request in bytes:"
//	       <<  request_length << endl;
	  // cout << "Rest of the request: «" << request_data.data() << "» " << endl;
#endif
	}
      else
	{
#ifdef SOCKET_CLIENT_DEBUG
	  qDebug() << "SocketClient[" << threadId << "]: no header found";
#endif
//	  cerr << "SocketClient[" << threadId << "]: incorrect header_data : «" << request_data.data() << "» " << endl;
	  client_disconnected();
	  return;
	}
	    
      /* Read the request */
      while (request_data.size() < request_length)
	{
	  qint64 status_read = client_socket->read(buf, max_nb_bytes_by_read);
	  if (status_read == 0)
	    {
#ifdef SOCKET_CLIENT_DEBUG
//	      cout << "SocketClient[" << threadId << "]: request read buf 0" << endl; 
#endif
	    }
	  else if (status_read == -1)
	    {
#ifdef SOCKET_CLIENT_DEBUG
//	      cout << "SocketClient[" << threadId << "]: request read status_read == -1" << endl; 
#endif
	    }
	  else
	    {
	      request_data.append(buf, status_read);
	    }

	  if (status_read == 0 && !client_socket->waitForReadyRead())
	    break;
	}
#ifdef SOCKET_CLIENT_DEBUG
      qDebug() << "SocketClient[" << threadId << "]: whole request read « " << request_data.data() << " »";
// , it is:«" << request_data.data () << "» " << endl;
	  qDebug() << "SocketClient[" << threadId << "]: evaluate javascript";
#endif
//      cout << "et en invokant? " << endl;
      QMetaObject::invokeMethod(webpage->mainFrame(), "evaluateJavaScript", Qt::QueuedConnection, Q_ARG(QString, QString(request_data.data())) );

      request_data.clear();
      request_length = 0;

#ifdef SOCKET_CLIENT_DEBUG
      qDebug() << "SocketClient[" << threadId << "]: work done";
#endif
	      
    }

//  cout << "Fini avec ce client" << endl;
  client_disconnected();


}


