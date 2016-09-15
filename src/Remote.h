#include <QtNetwork>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class Remote: public QObject
{
Q_OBJECT

public:
  Remote(QObject * parent = 0);
  ~Remote();

public slots:
  void acceptConnection();
  void startRead();

signals:
	void remote(QString *);

private:
  QTcpServer server;
  QTcpSocket* client;
};
