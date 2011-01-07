#ifndef CRAWL_FETCHLOOP_H
#define CRAWL_FETCHLOOP_H

#include <QObject>
#include <QWebView>

class QTimer;

namespace crawl
{
class FetchLoop : public QObject
{
Q_OBJECT
public:

  FetchLoop (QObject *parent=0);
  
  void Fetch (const QUrl &);

private slots:

  void LoadFinished (bool ok);
  void StopLoading ();

signals:

  void FoundLink (const QString & link);
  void PageDone (bool ok);

private:

  bool DontFetch (const QUrl & url);
  void Done (bool ok);

  QWebView   *view;
  QStringList  dontFetch;

  QTimer  *loadTimeout;

};

} // namespace

#endif