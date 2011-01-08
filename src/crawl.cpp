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
#include "special-list.h"
#include <QSize>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QCursor>

using namespace deliberate;

namespace openmielke
{

Crawl::Crawl (QWidget *parent)
  :QMainWindow (parent),
   initDone (false),
   app (0),
   configEdit (this),
   helpView (0),
   runAgain (false),
   loop (0),
   blackList (0)
{
  mainUi.setupUi (this);
  myPage = new QWebPage (this);
  mainUi.seedView->setPage (myPage);
  loop = new FetchLoop (this, mainUi.strollView);
  blackList = new SpecialList;
  blackList->Init ();
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
  initDone = true;
  mainUi.workLabel->setText ("Idle");
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
  mainUi.seedView->setContent (html.toUtf8());
  mainUi.seedView->page()
              ->setLinkDelegationPolicy (QWebPage::DelegateAllLinks);
  mainUi.strollView->page()
              ->setLinkDelegationPolicy (QWebPage::DelegateAllLinks);
  mainUi.linkCount->setValue (seedList.count());
qDebug () << " page delegation policy " << mainUi.seedView->page()->linkDelegationPolicy();
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
  connect (mainUi.seedView, SIGNAL (linkClicked (const QUrl &)),
           this, SLOT (LinkTo (const QUrl &)));
  connect (mainUi.crawlButton, SIGNAL (clicked()),
           this, SLOT (StartCrawl ()));
  connect (mainUi.addButton, SIGNAL (clicked()),
           this, SLOT (AddSeed ()));

  connect (mainUi.seedView, SIGNAL (linkClicked(const QUrl &)),
           this, SLOT (LinkClicked (const QUrl &)));
  connect (mainUi.seedView->page (), SIGNAL (linkClicked(const QUrl &)),
           this, SLOT (PageLinkClicked (const QUrl &)));
  connect (mainUi.strollView, SIGNAL (linkClicked(const QUrl &)),
           this, SLOT (LinkClicked (const QUrl &)));
  connect (mainUi.strollView->page (), SIGNAL (linkClicked(const QUrl &)),
           this, SLOT (PageLinkClicked (const QUrl &)));

  connect (loop, SIGNAL (FoundLink (const QString &)),
           this, SLOT (CatchLink (const QString &)));
  connect (loop, SIGNAL (PageDone (bool)),
           this, SLOT (PageDone (bool)));
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
Crawl::Link (const QString & target, const QString & scheme)
{
  QUrl url (target);
  if (scheme.length() > 0) {
    url.setScheme (scheme);
  }
  return QString ("<a href=\"%2\" name=\"%1\">%1</a>")
                  .arg (target)
                  .arg (url.toString());
}

void
Crawl::LinkTo (const QUrl & url)
{
  qDebug () << url;
  if (url.isValid()) {
    mainUi.seedView->load (url);
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
  sendQueue = seedList;
  seedList.clear ();
  mainUi.sendProgress->setMaximum (sendQueue.count());
  progress = 0;
  mainUi.sendProgress->setValue (progress);
  CrawlNext ();
}

void
Crawl::CrawlNext ()
{
  if (!sendQueue.isEmpty ()) {
    QUrl nextUrl = sendQueue.takeFirst ();
    mainUi.currentFetch->setText (nextUrl.toString());
    loop->Fetch (nextUrl);
    mainUi.workLabel->setText ("working...");
  } else {
    mainUi.workLabel->setText ("Done");
  }
}

void
Crawl::CatchLink (const QString & link)
{
  qDebug () << " got link " << link;
  QUrl url (link);
  if (url.scheme() == "http" || url.scheme() == "https") {
    if (!oldLinks.contains (url) && !blackList->IsKnown(url)) {
      seedList.append (url);
      oldLinks.insert (url);
    }
  }
}

void
Crawl::PageDone (bool ok)
{
  qDebug () << " Page Done " << ok;
  Show ();
  progress++;
  mainUi.sendProgress->setValue (progress);
  CrawlNext ();
}

void
Crawl::LinkClicked (const QUrl & url)
{
  qDebug () << " view clicked on " << url;
}

void
Crawl::PageLinkClicked (const QUrl & url)
{
  qDebug () << " page clicked on " << url;
}


} // namespace

