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
#include "main.h"

/*
 */

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

  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF8"));

  connect(this->phantom->m_page, SIGNAL(javaScriptConsoleMessageSent(QString)),
	  this, SLOT(copyJsConsoleMessageToClientSocket(QString)),
	  Qt::QueuedConnection);

}

void SocketClient::sendConsoleMessage(const QString &message)
{
//  cout << "SocketClient: message '" << message.toUtf8().data() << "'" << endl;
  if (client_socket != NULL)
    {
      client_socket->write(message.toUtf8().data());
      client_socket->write("\n");
    }
}

void SocketClient::copyJsConsoleMessageToClientSocket(const QString &message)
{
//  cout << "SocketClient: message '" << message.toUtf8().data() << "'" << endl;
  if (client_socket != NULL)
    {
      client_socket->write(message.toUtf8().data());
      client_socket->write("\n");
    }
}


void SocketClient::client_disconnected()
{
  cout << "SocketClient: client disconnected" << endl;
  if (client_socket != NULL)
    {
      client_socket->close();
    }


  QMetaObject::invokeMethod(main, "deletePhantomJSInstance", Qt::QueuedConnection, Q_ARG(quint64, threadId));

  socketServer->deleteThreadInstance(threadId);
}

void SocketClient::doWork()
{
  cout << "SocketClient: doWork" << endl;

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
//      cout << "waiting for a new request of the client" << endl;

      /* First read the header which consists in a line of text 
	 telling how long in bytes is the request */
      while (true)
	{
//	  cout << "data read..." << endl;
	  qint64 status_read = client_socket->read(buf, max_nb_bytes_by_read);
	  if (status_read == 0)
	    {
//	      cout << "buf 0" << endl; 
	    }
	  else if (status_read == -1)
	    {
//	      cout << "error" << endl;
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
//	      cout << "endline character found" << endl;
	      break;
	    }

//	  cout << "waiting2" << endl;

	  // if (status_read == 0 && !client_socket->waitForReadyRead())
	  //   break;

	  if (!client_socket->waitForReadyRead())
	    {
//	      cout << "!client_socket->waitForReadyRead())" << endl;
	      break;
	    }
//	  cout << "waiting3" << endl;

	}
//      cout << "Header end met" << endl;

      if (client_socket == NULL)
	{
	  cout << "SocketClient: Socket null" << endl;
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
//	  cout << "Length of the request in bytes:"
//	       <<  request_length << endl;
//	  cout << "Rest of the request: «" << request_data.data() << "» " << endl;
	}
      else
	{
	  cerr << "SocketClient: Error, no header found" << endl;
	  client_disconnected();
	  return;
	}
	    
      /* Read the request */
      while (request_data.size() < request_length)
	{
	  qint64 status_read = client_socket->read(buf, max_nb_bytes_by_read);
	  if (status_read == 0)
	    {
//	      cout << "buf 0" << endl; 
	    }
	  else if (status_read == -1)
	    {
//	      cout << "error" << endl;
	    }
	  else
	    {
	      request_data.append(buf, status_read);
	    }

	  if (status_read == 0 && !client_socket->waitForReadyRead())
	    break;
	}
//      cout << "OK whole request read, it is:«" << request_data.data () << "» " << endl;

//      cout << "et en invokant? " << endl;
      QMetaObject::invokeMethod(webpage->mainFrame(), "evaluateJavaScript", Qt::QueuedConnection, Q_ARG(QString, QString(request_data.data())) );

      request_data.clear();
      request_length = 0;

//      cout << "JS evaluated" << endl;
	      
    }

//  cout << "Fini avec ce client" << endl;
  client_disconnected();


}


