#ifndef CRAWL_FETCHLOOP_H
#define CRAWL_FETCHLOOP_H


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


#include <QObject>
#include <QWebView>

class QTimer;
class QNetworkAccessManager;

namespace openmielke
{
class FetchLoop : public QObject
{
Q_OBJECT
public:

  FetchLoop (QObject *parent, QWebView * spyView);
  
  void Fetch (const QUrl & startUrl, 
              bool reportSingle=false);

private slots:

  void LoadFinished (bool ok);
  void StopLoading ();
  void ReadReply (QNetworkReply * reply);

signals:

  void FoundLink (const QString & link);
  void Keywords (const QString & urlString, const QString & words);
  void ReportLinks (const QString sourceLink,
                    const QStringList & resultList);
  void PageDone (bool ok);

private:

  bool DontFetch (const QUrl & url);
  void Done (bool ok);
  void GetLinks (QWebFrame * frame);
  void GetMeta (QWebFrame * frame);
  void ForwardTo (const QUrl & url);

  QWebView                 * view;
  QNetworkAccessManager    * net;
  QStringList                dontFetch;
  QString                    baseUrl;
  bool                       reportOne;
  QString                    startLink;
  QStringList                foundLinks;

  QTimer  *loadTimeout;

};

} // namespace

#endif