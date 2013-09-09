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

/*
 */

#define SOCKET_CLIENT_DEBUG 1
// #undef SOCKET_CLIENT_DEBUG

SocketClient::SocketClient(QThread *thread)
{
  this->thread = thread;
}

SocketClient::~SocketClient()
{
}

void SocketClient::setup(Main *main, SocketServer *socketServer)
{
  this->main = main;
  this->socketServer = socketServer;

  quint64 thread_id = (quint64) (void*)this;
  this->threadId = thread_id;


  QMetaObject::invokeMethod(main, "createPhantomJSInstance", Qt::QueuedConnection, Q_ARG(quint64, thread_id));

  int tmp;
  
  while (!main->phantomInstancesMap.contains(thread_id)){
    sleep(1);
  }

  this->phantom = ((*main).phantomInstancesMap).value(thread_id);

  this->phantom->socketClient = this;
  this->webpage = phantom->m_page;
  
//    CookieJar *cookieJar=CookieJar::create(this->phantom);
//  cookieJar->setParent(this->phantom);
//  cout << "SocketClient[" << threadId << "]: set CookieJar" << endl;
//	this->webpage->m_networkAccessManager->setCookieJar(cookieJar);
//  this->webpage->m_networkAccessManager->cookieJar->setParent(this->phantom());
  
#ifdef SOCKET_CLIENT_DEBUG
  cout << "SocketClient[" << threadId << "]: setup" << endl;
#endif

  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF8"));

  connect(this->phantom->m_page, SIGNAL(javaScriptConsoleMessageSent(QString)),
	  this, SLOT(copyJsConsoleMessageToClientSocket(QString)),
	  Qt::QueuedConnection);

      // Listen for Phantom exit(ing)
      connect(this->phantom, SIGNAL(aboutToExit(int)), this, SLOT(client_disconnected));

}

void SocketClient::sendConsoleMessage(const QString &message)
{
#ifdef SOCKET_CLIENT_DEBUG
  cout << "SocketClient[" << threadId << "]: send response « " << message.toUtf8().data() << " »" << endl;
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
  cout << "SocketClient[" << threadId << "]: message sent 2" << endl; //'" << message.toUtf8().data() << "'" << endl;
#endif

  if (client_socket != NULL)
    {
      client_socket->write(message.toUtf8().data());
      client_socket->write("\n");
    }
}


void SocketClient::client_disconnected()
{
  cout << "SocketClient[" << threadId << "]: client disconnected" << endl;
  if (client_socket != NULL)
    {
      client_socket->close();
    }

  QMetaObject::invokeMethod(main, "deletePhantomJSInstance", Qt::QueuedConnection, Q_ARG(quint64, threadId));

  socketServer->deleteThreadInstance(threadId);
}

void SocketClient::doWork()
{
#ifdef SOCKET_CLIENT_DEBUG
  cout << "SocketClient[" << threadId << "]: do work" << endl;
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
      cout << "SocketClient[" << threadId << "]: waiting for a new request of the client" << endl;
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

		
//	  cout << "waiting" << endl;

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
	  cout << "SocketClient[" << threadId << "]: still waiting" << endl;
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
		  cout << "SocketClient[" << threadId << "]: still waiting" << endl;
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
	      cout << "SocketClient[" << threadId << "]: wait for too long, aborting" << endl;
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
	  cerr << "SocketClient[" << threadId << "]: no header found" << endl;
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
      cout << "SocketClient[" << threadId << "]: whole request read « " << request_data.data() << " »" << endl;
#endif
// , it is:«" << request_data.data () << "» " << endl;
	  cout << "SocketClient[" << threadId << "]: evaluate javascript" << endl;
//      cout << "et en invokant? " << endl;
      QMetaObject::invokeMethod(webpage->mainFrame(), "evaluateJavaScript", Qt::QueuedConnection, Q_ARG(QString, QString(request_data.data())) );

      request_data.clear();
      request_length = 0;

#ifdef SOCKET_CLIENT_DEBUG
      cout << "SocketClient[" << threadId << "]: work done" << endl;
#endif
	      
    }

//  cout << "Fini avec ce client" << endl;
  client_disconnected();


}


