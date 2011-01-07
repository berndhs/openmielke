#include "crawl.h"

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

#include <QApplication>
#include "deliberate.h"
#include "version.h"
#include "helpview.h"
#include <QSize>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QCursor>

using namespace deliberate;

namespace crawl
{

Crawl::Crawl (QWidget *parent)
  :QMainWindow (parent),
   initDone (false),
   app (0),
   configEdit (this),
   helpView (0),
   runAgain (false)
{
  mainUi.setupUi (this);
  mainUi.actionRestart->setEnabled (false);
  helpView = new HelpView (this);
  Connect ();
}

void
Crawl::Init (QApplication &ap)
{
  app = &ap;
  connect (app, SIGNAL (lastWindowClosed()), this, SLOT (Exiting()));
  htmlEmbed = QString ("<html>\n<head>\n%1\n</head>\n"
                       "<body>\n%2\n</body>\n</html>\n");
  htmlEmbed = Settings().value ("view/htmlembed",htmlEmbed).toString();
  Settings().setValue ("view/htmlembed",htmlEmbed);
  Settings().sync();
  msgList.append (Link ("http://maui.ipv6.berndnet"));
  mainUi.webView->page()
              ->setLinkDelegationPolicy (QWebPage::DelegateAllLinks);
  initDone = true;
}

bool
Crawl::Again ()
{
  bool again = runAgain;
  runAgain = false;
  return again;
}

bool
Crawl::Run ()
{
  runAgain = false;
  if (!initDone) {
    Quit ();
    return false;
  }
  qDebug () << " Start Crawl";
  QSize defaultSize = size();
  QSize newsize = Settings().value ("sizes/main", defaultSize).toSize();
  resize (newsize);
  Settings().setValue ("sizes/main",newsize);
  msgList.append ("nothing");
  Show ();
  show ();
  return true;
}

void
Crawl::Show ()
{
  msgList.clear();
  for (int i=0; i<seedList.count(); i++) {
    msgList.append (Link (seedList.at(i).toString()));
  }
  QString html = htmlEmbed
                        .arg (head)
                        .arg (msgList.join ("<br>\n"));
  mainUi.webView->setHtml (html);
  qDebug () << html;
}

void
Crawl::Connect ()
{
  connect (mainUi.actionQuit, SIGNAL (triggered()), 
           this, SLOT (Quit()));
  connect (mainUi.actionSettings, SIGNAL (triggered()),
           this, SLOT (EditSettings()));
  connect (mainUi.actionAbout, SIGNAL (triggered()),
           this, SLOT (About ()));
  connect (mainUi.actionLicense, SIGNAL (triggered()),
           this, SLOT (License ()));
  connect (mainUi.actionRestart, SIGNAL (triggered()),
           this, SLOT (Restart ()));
  connect (mainUi.webView, SIGNAL (linkClicked (const QUrl &)),
           this, SLOT (LinkTo (const QUrl &)));
  connect (mainUi.crawlButton, SIGNAL (clicked()),
           this, SLOT (StartCrawl ()));
  connect (mainUi.addButton, SIGNAL (clicked()),
           this, SLOT (AddSeed ()));
}

void
Crawl::Restart ()
{
  qDebug () << " restart called ";
  runAgain = true;
  Quit ();
}


void
Crawl::Quit ()
{
  CloseCleanup ();
  if (app) {
    app->quit();
  }
}

void
Crawl::closeEvent (QCloseEvent *event)
{
  Q_UNUSED (event)
  CloseCleanup ();
}

void
Crawl::CloseCleanup ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
}

void
Crawl::EditSettings ()
{
  configEdit.Exec ();
  SetSettings ();
}

void
Crawl::SetSettings ()
{
  Settings().sync ();
}

void
Crawl::About ()
{
  QString version (deliberate::ProgramVersion::Version());
  QStringList messages;
  messages.append (version);
  messages.append (configMessages);

  QMessageBox  box;
  box.setText (version);
  box.setDetailedText (messages.join ("\n"));
  QTimer::singleShot (30000, &box, SLOT (accept()));
  box.exec ();
}

void
Crawl::Exiting ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
}

void
Crawl::License ()
{
  if (helpView) {
    helpView->Show ("qrc:/help/LICENSE.txt");
  }
}

QString
Crawl::Link (const QString & target)
{
  return QString ("<a href=\"%1\">%1</a>").arg (target);
}

void
Crawl::LinkTo (const QUrl & url)
{
  qDebug () << url;
  if (url.isValid()) {
    mainUi.webView->load (url);
  }
}

void
Crawl::AddSeed ()
{
  QString newurl = mainUi.seedUrlEdit->text();
  seedList.append (QUrl (newurl));
  Show ();
}

void
Crawl::StartCrawl ()
{
  qDebug () << " start crawling";
}



} // namespace

