
#include "special-list.h"

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

#include <QRegExp>
#include <QDebug>

namespace pescador
{

SpecialList::SpecialList ()
{
}

void
SpecialList::Init ()
{
  knownHosts << "google.com" << "www.google.com"
             << "amazon.com"
             << "doubleclick.net";
}

bool
SpecialList::IsKnown (const QUrl & suspect)
{
  QString host = suspect.host ().trimmed ();
  while (host.length() > 0 && host.endsWith('/')) {
    host.chop(1);
  }
  if (host.length() < 6) {
    return false;
  }
  QRegExp pat (QString ("*%1").arg(host));
  pat.setPatternSyntax(QRegExp::Wildcard);
qDebug () << " checking for host pattern " << pat.pattern();
qDebug () << "  in " << knownHosts;
  int index = knownHosts.indexOf (pat);
qDebug () << "      index " << index;
  if (index >= 0) {
    return true;
  }
  return false;
}

} // namespace
