#include "crawl.h"

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
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QRegExp>
#include <QXmlStreamWriter>

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
  connect (mainUi.cycleButton, SIGNAL (clicked()),
           this, SLOT (Cycle ()));
  connect (mainUi.actionSaveXML, SIGNAL (triggered()),
           this, SLOT (SaveXML()));
  connect (mainUi.actionSaveArado, SIGNAL (triggered()),
           this, SLOT (SaveArado ()));
           

  connect (mainUi.seedView, SIGNAL (linkClicked(const QUrl &)),
           this, SLOT (LinkClicked (const QUrl &)));
  connect (mainUi.seedView->page (), SIGNAL (linkClicked(const QUrl &)),
           this, SLOT (PageLinkClicked (const QUrl &)));
  connect (mainUi.strollView, SIGNAL (linkClicked(const QUrl &)),
           this, SLOT (LinkClicked (const QUrl &)));
  connect (mainUi.strollView->page (), SIGNAL (linkClicked(const QUrl &)),
           this, SLOT (PageLinkClicked (const QUrl &)));
  connect (mainUi.resultView, SIGNAL (linkClicked(const QUrl &)),
           this, SLOT (ResultClicked (const QUrl &)));

  connect (loop, SIGNAL (FoundLink (const QString &)),
           this, SLOT (CatchLink (const QString &)));
  connect (loop, SIGNAL (ReportLinks (const QString &, const QStringList &)),
           this, SLOT (CatchReport (const QString &, const QStringList &)));
  connect (loop, SIGNAL (PageDone (bool)),
           this, SLOT (PageDone (bool)));
  connect (loop, SIGNAL (Keywords (const QString &, const QString &)),
           this, SLOT (CatchKeywords (const QString &, const QString &)));
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

void
Crawl::Show ()
{
  ShowSeeds ();
  ShowResults ();
  mainUi.strollView->page()
              ->setLinkDelegationPolicy (QWebPage::DelegateAllLinks);
qDebug () << " page delegation policy " << mainUi.seedView->page()->linkDelegationPolicy();
}

void
Crawl::ShowSeeds ()
{
  QStringList msgList;
  for (int i=0; i<sendQueue.count(); i++) {
    msgList.append (Link (sendQueue.at(i).toString()));
  }
  QString seedHtml = htmlEmbed
                        .arg (head)
                        .arg (msgList.join ("<br>\n"));
  mainUi.seedView->setContent (seedHtml.toUtf8());
  mainUi.seedView->page()
              ->setLinkDelegationPolicy (QWebPage::DelegateAllLinks);
  mainUi.resultView->page()
              ->setLinkDelegationPolicy (QWebPage::DelegateAllLinks);
}

void
Crawl::ShowResults ()
{
  QUrl url (findSource);
  QString origScheme = url.scheme();
  QString path = url.path();
  path.prepend (origScheme);
  url.setPath (path);
  url.setScheme ("crawlkeys");
  QString keyStyle ("style=\"color:#10f0f0\"");
  QString keyLink (QString("<a href=\"%1\" %2>keywords</a>")
                     .arg (url.toString())
                     .arg (keyStyle));
  QString headLine = QString("From Seed %1 %2 get %3 links:")
                              .arg (findSource)
                              .arg (keyLink)
                              .arg (foundList.count());
  foundReport.append (headLine);
  for (int i=0; i<foundList.count(); i++) {
    foundReport.append (ResultLink (foundList.at(i).toString()));
  }
  QString resultHtml = htmlEmbed
                         .arg (head)
                         .arg (foundReport.join ("<br>\n"));
  mainUi.resultView->setContent (resultHtml.toUtf8());
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

QString
Crawl::ResultLink (const QString & target)
{
  QUrl url (target);
  QString origScheme = url.scheme();
  QString path = url.path();
  path.prepend (origScheme);
  url.setPath (path);
  QString seedStyle ("style=\"color:green\"");
  QString targetStyle ("style=\"colod:blue\"");
  QString targetLink (QString("<a href=\"%1\" %2>%1</a>")
                  .arg (target)
                  .arg (targetStyle));
  url.setScheme ("crawlseed");
  QString seedLink (QString("<a href=\"%1\" %3>seed</a>")
                     .arg (url.toString())
                     .arg (seedStyle));
  url.setScheme ("crawlsave");
  QString saveLink (QString("<a href=\"%1\" %3>save</a>")
                     .arg (url.toString())
                     .arg (seedStyle)); 
  return QString ("%1--%2--%3")
                 .arg (saveLink)
                 .arg (seedLink)
                 .arg (targetLink);
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
  sendQueue.append (QUrl (newurl));
  ShowSeeds ();
}

void
Crawl::StartCrawl ()
{
  qDebug () << " start crawling";
  foundList.clear ();
  foundReport.clear ();
  mainUi.sendProgress->setMaximum (sendQueue.count());
  progress = 0;
  mainUi.sendProgress->setValue (progress);
  getSiteMap = mainUi.sitemapCheck->isChecked();
  CrawlNext ();
}

void
Crawl::Cycle ()
{
  sendQueue = foundList;
  Show ();
  StartCrawl ();
}

void
Crawl::CrawlNext ()
{
  if (!sendQueue.isEmpty ()) {
    QUrl nextUrl = sendQueue.takeFirst ();
    mainUi.currentFetch->setText (nextUrl.toString());
    if (getSiteMap) {
      siteMapHost = nextUrl.host();
    }
    keywords.clear ();
    loop->Fetch (nextUrl, false);
    mainUi.workLabel->setText ("working...");
    ShowSeeds ();
  } else {
    mainUi.workLabel->setText ("Done");
  }
}

void
Crawl::CatchLink (const QString & link, bool report)
{
  QUrl url (link);
  if (url.scheme() == "http" || url.scheme() == "https") {
    if (!oldLinks.contains (url) && !blackList->IsKnown(url)) {
      if (getSiteMap) {
        if (url.host() == siteMapHost) {
          foundList.append (url);
        }
      } else {
        foundList.append (url);
      }
      oldLinks.insert (url);
      if (report) {
        ShowResults ();
      }
    }
  }
}

void
Crawl::CatchKeywords (const QString & startUrl, const QString & words)
{
qDebug () << " keywords of " << startUrl << " before " << keywords[startUrl];
  keywords[startUrl] += words.split (QRegExp("\\s"));
qDebug () << " keywords of " << startUrl << " after " << keywords[startUrl];
}

void
Crawl::CatchReport (const QString & sourceLink,
                    const QStringList & linkList)
{
  int nl = linkList.count();
  findSource = sourceLink;
  for (int l=0; l < nl; l++) {
    CatchLink (linkList.at(l), false);
  }
  ShowResults ();
}

void
Crawl::PageDone (bool ok)
{
  qDebug () << " Page Done " << ok;
  progress++;
  mainUi.sendProgress->setValue (progress);
  CrawlNext ();
}

void
Crawl::ResultClicked (const QUrl & url)
{
  qDebug () << " result view clicked on " << url;
  QString scheme = url.scheme();
  if (scheme == "crawlseed") {
    qDebug () << " crawlseed clicked on " << url;
    sendQueue.append (RetrieveUrl (url));
    ShowSeeds ();
  } else if (scheme == "crawlsave") {
    qDebug () << " Save " << url;
    saveList.append (RetrieveUrl (url));
  } else if (scheme == "crawlkeys") {
    QString urlString = RetrieveUrl (url).toString();
    qDebug () << " keywords for " << urlString;
    qDebug () << keywords[urlString];
    QString html ("<html><body>%1</body></html>");
    mainUi.strollView->setHtml (html.arg(keywords[urlString].join("<br>\n")));
  } else {
    QDesktopServices::openUrl (url);
  }
}

QUrl
Crawl::RetrieveUrl (const QUrl & savedUrl)
{
  QUrl origUrl (savedUrl);
  QStringList parts = savedUrl.path().split('/');
  parts.removeFirst();
  QString oldScheme = parts.takeFirst();
  origUrl.setScheme (oldScheme);
  origUrl.setPath (parts.join("/"));
  return origUrl;
}

void
Crawl::SaveXML ()
{
  QString filename = QFileDialog::getSaveFileName (this,
                 "Save URL List");
  if (filename.length() < 1) {
    return;
  }
  QFile file (filename);
  bool ok = file.open (QFile::WriteOnly);
  if (!ok) {
    QMessageBox box;
    box.setText (tr("Cannot write file \"%1\"").arg(filename));
    QTimer::singleShot (15000, &box, SLOT(reject()));
    box.exec ();
    return;
  }
  QXmlStreamWriter xmlout (&file);
  xmlout.setAutoFormatting (true);
  xmlout.setAutoFormattingIndent (1);
  xmlout.writeStartDocument ();
  xmlout.writeStartElement ("urllist");
  int nu = saveList.count ();
  for (int u=0; u<nu; u++) {
    xmlout.writeTextElement ("url",saveList.at(u).toString());
  }
  xmlout.writeEndElement (); // urllist
  xmlout.writeEndDocument ();
  file.close ();
}

void
Crawl::SaveArado ()
{
  QString filename = QFileDialog::getSaveFileName (this,
                 "Save Arado URL List");
  if (filename.length() < 1) {
    return;
  }
  QFile file (filename);
  bool ok = file.open (QFile::WriteOnly);
  if (!ok) {
    QMessageBox box;
    box.setText (tr("Cannot write file \"%1\"").arg(filename));
    QTimer::singleShot (15000, &box, SLOT(reject()));
    box.exec ();
    return;
  }
  QXmlStreamWriter xmlout (&file);
  xmlout.setAutoFormatting (true);
  xmlout.setAutoFormattingIndent (1);
  xmlout.writeStartDocument ();
  xmlout.writeStartElement ("arado");
  int nu = saveList.count ();
  for (int u=0; u<nu; u++) {
    xmlout.writeStartElement ("aradourl");
    xmlout.writeTextElement ("url",saveList.at(u).toString());
    xmlout.writeEndElement (); // aradourl
  }
  xmlout.writeEndElement (); // arado
  xmlout.writeEndDocument ();
  file.close();
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

