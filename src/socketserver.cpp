#include "socketserver.h"

#include <iostream>
#include <QDir>
#include <QTcpServer>
#include <QTextCodec>
#include <QTcpSocket>
#include <QObject>
#include <QAbstractSocket>
using namespace std;
#include <unistd.h>

/*
Protocol:

Request:
- A header line which is consists of the string "XXX\n" where XXX is
an integer, the length of the javascript code that follows and that
needs to be executed.
- Then the javascript string code.
 */

SocketServer::SocketServer()
{
}

SocketServer::~SocketServer()
{
}

void SocketServer::setup(QWebFrame *webframe)
{
  this->webframe = webframe;
  
  connect(this, SIGNAL(evaluateJavaScript(const QString &code)),
	  webframe, SLOT(QVariant& evaluateJavaScript(const QString &code)),
	  Qt::QueuedConnection);

  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF8"));
}

void SocketServer::sendConsoleMessage(const QString &message)
{
  cout << "Allez on l'envoit au client ce message" << endl;
  cout << "message: '" << message.toUtf8().data() << "'" << endl;
  if (client_socket != NULL)
    {
      client_socket->write(message.toUtf8().data());
      client_socket->write("\n");
      //      client_socket->flush();
    }
}

void SocketServer::client_disconnected()
{
  cout << "Le client il est parti" << endl;
  client_socket = NULL;
}

void SocketServer::doWork()
{
  cout << "doWork..." << endl;

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
	      /* Read all data till all the input is read */
	      QByteArray request_data;
	      qint64 max_nb_bytes_by_read = 80;
	      char buf[max_nb_bytes_by_read+1];
	      // testServer.client_read_channel_closed = false;
	      // QObject::connect(client_socket, SIGNAL(readChannelFinished(void)),
	      // 		     &testServer, SLOT(client_read_channel_close(void)));

	      QObject::connect(client_socket, SIGNAL(disconnected(void)),
			       this, SLOT(client_disconnected(void)));

	      /* While connected, wait for new request */
	      while (client_socket != NULL)
		{
		  cout << "waiting for a new request of the client" << endl;

	      /* First read the header which consists in a line of text 
		 telling how long in bytes is the request */
	      while (true)
		{
		  cout << "data read..." << endl;
		  qint64 status_read = client_socket->read(buf, max_nb_bytes_by_read);
		  if (status_read == 0)
		    {
		      cout << "buf 0" << endl; 
		    }
		  else if (status_read == -1)
		    {
		      cout << "error" << endl;
		    }
		  else
		    {
		      request_data.append(buf, status_read);
		    }

		
		  cout << "waiting" << endl;

		  /* Once the newline character is read, the header has
		     been completely read. */
		  if (request_data.indexOf("\n") != -1)
		    {
		      cout << "endline character found" << endl;
		      break;
		    }

		  cout << "waiting2" << endl;

		  // if (status_read == 0 && !client_socket->waitForReadyRead())
		  //   break;

		  if (!client_socket->waitForReadyRead())
		    {
		      cout << "!client_socket->waitForReadyRead())" << endl;
		      break;
		    }
		  cout << "waiting3" << endl;

		}
	      cout << "Header end met" << endl;

	      if (client_socket == NULL)
		{
		  cout << "Socket null" << endl;
		  break;
		}

	      int index_newline;
	      QByteArray header_data;
	      int request_length = 0;
	      if ((index_newline = request_data.indexOf("\n")) != -1)
		{
		  header_data = request_data.left(index_newline + 1);
		  cout << "Header read: «" << header_data.data() << "» " << endl;
		  request_data  = request_data.right(request_data.size() - (index_newline + 1));
		  request_length = atoi(header_data.data());
		  cout << "Length of the request in bytes:"
		       <<  request_length << endl;
		  cout << "Rest of the request: «" << request_data.data() << "» " << endl;
		}
	      else
		{
		  cerr << "Error, no header found" << endl;
		  exit(1);
		}
	    
	      /* Read the request */
	      while (request_data.size() < request_length)
		{
		  qint64 status_read = client_socket->read(buf, max_nb_bytes_by_read);
		  if (status_read == 0)
		    {
		      cout << "buf 0" << endl; 
		    }
		  else if (status_read == -1)
		    {
		      cout << "error" << endl;
		    }
		  else
		    {
		      request_data.append(buf, status_read);
		    }

		  if (status_read == 0 && !client_socket->waitForReadyRead())
		    break;
		}
	      cout << "OK whole request read, it is:«" << request_data.data () << "» " << endl;

	      // Evaluating the request

	      // Should have the same behaviour than on console, wait for the end of the evaluation before going on...

	      // cout << "emitting..." << endl;
	      // emit( evaluateJavaScript(QString(request_data.data())) );

	      cout << "et en invokant? " << endl;
	      //	      QMetaObject::invokeMethod(webframe, "evaluateJavaScript", Qt::QueuedConnection, Q_ARG(QString, QString("console.log('lala c est la bonne maniere ');")) );
	      QMetaObject::invokeMethod(webframe, "evaluateJavaScript", Qt::QueuedConnection, Q_ARG(QString, QString(request_data.data())) );

	      request_data.clear();
	      request_length = 0;

	      cout << "JS evaluated" << endl;

	      // bool statusError = injectJsInFrame(const QString &jsFilePath, const QString &libraryPath, web, const bool startingScript = false);

	      // webframe->evaluateJavaScript(request_data.data(), QDir::currentPath());

	      //            m_page->evaluateJavaScript(request_data.data());


	      // Le problème c'est sans doute qu'il faut attendre que la page soit chargée...

	      // for (int i = 0; i < 10; i++)
	      //   {
	      // 	usleep(1000 * 1000);
	      //   }
	    

	      // QString(JS_EVAL_USER_INPUT).arg(
	      //     QString(userInput).replace('"', "\\\"")), QString("phantomjs://repl-input"));


	      // // buf = client_socket->readAll();
	      // // request_data.append(buf);
	      // QString s = QString::fromUtf8(request_data.data());
	      // cout << "Request read:" << endl << (char*)s.toUtf8().data() << endl;
	    
	      // QString response("Hé ! Les loyers sont trop chers !\n");
	      // // TODO: check that all the bytes have been written...
	      // qint64 nb_written = client_socket->write(response.toUtf8().data());
	      // cout << "bytes written: " << nb_written << endl;
	      // if (nb_written == -1)
	      //   {
	      // 	string str;
	      // 	str = qPrintable( client_socket->errorString() );
	      // 	cout << "error and error is: " << str << endl;
	      //   }
	      
		}

	      cout << "Fini avec ce client" << endl;
#if 0
	      client_socket->flush();
	      client_socket->close();
	      client_socket = NULL;
#endif

	    }
	}
      else
	{
	  cout << "Problème waiting for a new connection." << endl;
	}

    }
}
