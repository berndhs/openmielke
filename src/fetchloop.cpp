#include "fetchloop.h"


/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2017, Bernd Stramm
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


#include <QWebEnginePage>
#include <QWebEnginePage>
#include <QWebElement>
#include <QWebElementCollection>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QDebug>

namespace openmielke
{

FetchLoop::FetchLoop (QObject *parent, QWebEngineView * spyView)
  :QObject (parent)
{
  if (spyView) {
    view = spyView;
  } else {
    view = new QWebEngineView;
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
FetchLoop::Fetch (const QUrl & startUrl, bool reportSingle)
{
  Q_UNUSED (reportSingle);
  foundLinks.clear ();
  startLink = startUrl.toString();
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
FetchLoop::ForwardTo (const QUrl & url)
{
  net->get (QNetworkRequest (url));
}

void
FetchLoop::ReadReply (QNetworkReply * reply)
{
  if (reply == 0) {
    return;
  }
  int maxd (10*1024*1024);
  qDebug () << " ReadReply network error " << reply->error ();
  qDebug () << "       url " << reply->url();
  if (reply->hasRawHeader("Location")) {
    // forward 
    QByteArray newurl = reply->rawHeader ("Location");
    qDebug () << "Forward to " << newurl;
    ForwardTo (QUrl (QString(newurl)));
    return;
  }
  QByteArray data = reply->read (maxd);
  qDebug () << " data size " << data.size () 
         << " for url " << reply->url();
  if (data.size() >= maxd) {
    Done (false);
    return ;
  }
  baseUrl = reply->url().toString();
  #if 0
  view->setContent (data, QString(), reply->url());
  #else
  view->setContent (data);
  #endif
  //view->setContent (data);
}

void
FetchLoop::Done (bool ok)
{
  qDebug () << " done " << ok << " reportOne " 
           << reportOne << " count " 
           << foundLinks.count();
  loadTimeout->stop ();
  if (ok && !reportOne) {
    emit ReportLinks (startLink, foundLinks);
  }
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
  QWebEnginePage * page = view->page ();
  if (page == 0) {
    Done (false);
    return;
  }
  QWebEnginePage * mainFrame = page->mainFrame();
  if (mainFrame == 0) {
    Done (false);
    return;
  }
  QList<QWebEnginePage*> frames = mainFrame->childFrames();
  frames.prepend (mainFrame);
  for (int f=0; f<frames.count(); f++) {
    QWebEnginePage * frame = frames.at(f);
    if (frame) {
      GetMeta (frame);
      GetLinks (frame);
    }
  }
  Done (true);
}

void
FetchLoop::GetLinks (QWebEnginePage * frame)
{
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
      if (reportOne) {
        emit Found1Link (linkText);
      } else {
        foundLinks.append (linkText);
      }
    }
  }
}

void
FetchLoop::GetMeta (QWebEnginePage * frame)
{
  QString words;
  QWebElementCollection titles = frame->findAllElements ("TITLE");
  foreach (QWebElement title, titles ) {
    QString headline = title.toPlainText();
    words.append (headline);
  }
  QWebElementCollection metas = frame->findAllElements ("META");
  foreach (QWebElement meta, metas) {
    QString name = meta.attribute ("name");
    qDebug () << " meta is called " << name << " has value " << meta.attribute("content");
    if (name == "keywords") {
      words += " ";
      words += meta.attribute ("content");
    }
  }
qDebug () << " GetMeta " << startLink << " found " << words;
  emit Keywords (startLink, words);
}

} // namespace
