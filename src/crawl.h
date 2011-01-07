#ifndef CRAWL_H
#define CRAWL_H

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
#include <QMainWindow>
#include "ui_crawl.h"
#include "config-edit.h"
#include "helpview.h"
#include "fetchloop.h"
#include <QUrl>

class QApplication;

using namespace deliberate;

namespace crawl 
{

class Crawl : public QMainWindow
{
Q_OBJECT

public:

  Crawl (QWidget *parent=0);

  void  Init (QApplication &ap);
  bool  Run ();
  bool  Again ();

  void  AddConfigMessages (const QStringList & cm) 
           { configMessages.append (cm); }

  void Show ();

  void closeEvent ( QCloseEvent *event);

private slots:

  void Quit ();
  void Restart ();
  void EditSettings ();
  void SetSettings ();
  void About ();
  void License ();
  void Exiting ();

  void LinkTo (const QUrl  & url);

  void StartCrawl ();
  void AddSeed ();
  void CatchLink (const QString & link);
  void PageDone (bool ok);

private:

  void Connect ();
  void CloseCleanup ();
  void CrawlNext ();

  QString Link (const QString & target);

  bool             initDone;
  QApplication    *app;
  Ui_CrawlMain    mainUi;
 
  ConfigEdit       configEdit;
  QStringList      configMessages;

  deliberate::HelpView        *helpView;
  bool             runAgain;

  QStringList   msgList;
  QList<QUrl>   seedList;
  QList<QUrl>   sendQueue;

  int           progress;

  QString       head;
  QString       htmlEmbed;

  FetchLoop     loop;

};

} // namespace

#endif
