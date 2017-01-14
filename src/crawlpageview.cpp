#include "crawlpageview.h"
#include <QDebug>


using namespace deliberate;

namespace openmielke
{


CrawlPageView::CrawlPageView(QObject *parent)
{
  qDebug() << Q_FUNC_INFO;
}

bool CrawlPageView::acceptNavigationRequest(const QUrl &, NavigationType, bool)
{
  qDebug() << Q_FUNC_INFO;
}

} // namespace
