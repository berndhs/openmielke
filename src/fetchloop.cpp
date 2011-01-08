#include "fetchloop.h"


/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2011, Bernd Stramm
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/


#include <QWebFrame>
#include <QWebPage>
#include <QWebElement>
#include <QWebElementCollection>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QDebug>

namespace openmielke
{

FetchLoop::FetchLoop (QObject *parent, QWebView * spyView)
  :QObject (parent)
{
  if (spyView) {
    view = spyView;
  } else {
    view = new QWebView;
  }
  net = new QNetworkAccessManager (this);
  connect (view, SIGNAL (loadFinished (bool)),
          this, SLOT (LoadFinished (bool)));
  dontFetch << ".exe" 
            << ".pdf"
            << ".gz"
            << ".bz"
            << ".rar"
            << ".tgz"
            << ".tex"
            << ".doc"
            << ".jpg"
            << ".png"
            << ".gif"
            << ".iso" 
            << ".img"
            ;
  loadTimeout = new QTimer (this);
  connect (loadTimeout, SIGNAL (timeout()),
           this, SLOT (StopLoading()));
  connect (net, SIGNAL (finished (QNetworkReply *)),
           this, SLOT (ReadReply (QNetworkReply *)));
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
      loadTimeout->stop ();
      loadTimeout->start (30*1000);
      net->get (QNetworkRequest (startUrl));
    }
  } else {
    Done (false);
  }
}

void
FetchLoop::ReadReply (QNetworkReply * reply)
{
  if (reply == 0) {
    return;
  }
  int maxd (10*1024*1024);
qDebug () << " ReadReply network error " << reply->error ();
  QByteArray data = reply->read (maxd);
  qDebug () << " data size " << data.size () 
         << " for url " << reply->url();
  if (data.size() >= maxd) {
    Done (false);
    return ;
  }
  baseUrl = reply->url().toString();
  view->setContent (data, QString(), reply->url());
  //view->setContent (data);
}

void
FetchLoop::Done (bool ok)
{
  qDebug () << " done " << ok;
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
  Done (false);
}

void
FetchLoop::LoadFinished (bool ok)
{
  if (!ok) {
    Done (false);
    return;
  }
  QWebPage * page = view->page ();
  if (page == 0) {
    Done (false);
    return;
  }
  QWebFrame * mainFrame = page->mainFrame();
  if (mainFrame == 0) {
    Done (false);
    return;
  }
  QList<QWebFrame*> frames = mainFrame->childFrames();
  frames.prepend (mainFrame);
qDebug () << " page from " << baseUrl << " frame count " << frames.count();
  for (int f=0; f<frames.count(); f++) {
    QWebFrame * frame = frames.at(f);
    if (frame) {
      QWebElementCollection links = frame->findAllElements ("A");
      int localLinkCount (0);
      foreach (QWebElement elt, links) {
        if (localLinkCount++ > 1000) {
          break;
        }
        QString linkText (elt.attribute ("href"));
        QUrl linkUrl (linkText);
        if (linkUrl.scheme().length() < 1) {
          QString sep (linkText.startsWith ('/') 
                       || baseUrl.endsWith('/') ? "" : "/");
          linkText.prepend (baseUrl + sep);
        }
        if (linkText.length() > 0) {
          emit FoundLink (linkText);
        }
      }
    }
  }
  Done (true);
}

} // namespace
