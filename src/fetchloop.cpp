#include "fetchloop.h"
#include <QWebFrame>
#include <QWebPage>
#include <QWebElement>
#include <QWebElementCollection>
#include <QTimer>
#include <QDebug>

namespace crawl
{

FetchLoop::FetchLoop (QObject *parent)
  :QObject (parent)
{
  view = new QWebView;
  connect (view, SIGNAL (loadFinished (bool)),
          this, SLOT (LoadFinished (bool)));
  dontFetch << ".exe" 
            << ".pdf"
            << ".gz"
            << ".bz"
            << ".jpg"
            << ".png"
            << ".gif"
            ;
  loadTimeout = new QTimer (this);
  connect (loadTimeout, SIGNAL (timeout()),
           this, SLOT (StopLoading()));
}

void
FetchLoop::Fetch (const QUrl & startUrl)
{
  if (startUrl.isValid ()) {
    qDebug () << " load " << startUrl;
    if (DontFetch (startUrl)) {
      qDebug () << " not fetching " << startUrl;
      Done (true);
      return;
    } else {
      loadTimeout->start (30*1000);
      view->load (startUrl);
    }
  } else {
    Done (false);
  }
}

void
FetchLoop::Done (bool ok)
{
  loadTimeout->stop ();
  emit PageDone (ok);
}


bool
FetchLoop::DontFetch (const QUrl & url)
{
  QString slink = url.toString().toLower();
  for (int i=0; i < dontFetch.count(); i++) {
    if (slink.endsWith (dontFetch.at(i))) {
      return true;
    }
  }
  return false;
}

void
FetchLoop::StopLoading ()
{
  loadTimeout->stop ();
  view->stop ();
}

void
FetchLoop::LoadFinished (bool ok)
{
  if (!ok) {
    Done (false);
qDebug () << " Load False";
    return;
  }
  QWebPage * page = view->page ();
  if (page == 0) {
    Done (false);
qDebug () << " No Page";
    return;
  }
  QWebFrame * mainFrame = page->mainFrame();
  if (mainFrame == 0) {
    Done (false);
qDebug () << " No Frame";
    return;
  }
  QWebElementCollection links = mainFrame->findAllElements ("A");
  foreach (QWebElement elt, links) {
    QString linkText (elt.attribute ("href"));
    if (linkText.length() > 0) {
      emit FoundLink (linkText);
    }
  }
  Done (true);
}

} // namespace
