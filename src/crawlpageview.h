#ifndef CRAWLPAGEVIEW_H
#define CRAWLPAGEVIEW_H

#include <QObject>

using namespace deliberate;

namespace openmielke
{

class CrawlPageView : public QWebEnginePage
{
public:
  CrawlPageView(QObject * parent=Q_NULLPTR);

  bool acceptNavigationRequest(const QUrl &, NavigationType , bool );
};

} // namespace
#endif // CRAWLPAGEVIEW_H
