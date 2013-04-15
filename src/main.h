#ifndef MAIN_H
#define MAIN_H

#include <QObject>
#include <QMap>

class Phantom;

class Main : public QObject
{
  Q_OBJECT

public:
  Main(QObject *parent = 0);
  ~Main();
  static Main *instance();
  Phantom *create_new_phantomjs();
  Phantom *phantom;
  QMap <quint64, Phantom*> phantomInstancesMap;
  QMap <quint64, QThread*> threadInstancesMap;

public slots:
  void createPhantomJSInstance(quint64 threadId);
  void addThreadInstance(quint64 threadId, QThread *thread);
  void deletePhantomJSInstance(quint64 threadId);
  int createPhantomJSInstance2();
};

#endif
