/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "util/properties.h"

#include <QTextStream>
#include <QIODevice>
#include <QDataStream>

namespace atools {
namespace util {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::endl;
#endif

Properties::Properties()
{

}

Properties::Properties(const QByteArray& bytes)
{
  fromByteArray(bytes);
}

Properties::Properties(const QHash<QString, QString>& other)
  : QHash(other)
{

}

void Properties::write(QTextStream& stream) const
{
  for(auto it = begin(); it != end(); ++it)
    stream << it.key() << "=" << it.value() << endl;
}

void Properties::read(QTextStream& stream)
{
  while(!stream.atEnd())
  {
    QString line = stream.readLine();
    int idx = line.indexOf('#');
    if(idx >= 0)
      line.truncate(idx);

    if(line.isEmpty())
      continue;

    insert(line.section('=', 0, 0).trimmed(), line.section('=', 1, 1).trimmed());
  }
}

QByteArray Properties::asByteArray() const
{
  QByteArray bytes;
  QDataStream out(&bytes, QIODevice::WriteOnly);
  out << *this;
  return bytes;
}

void Properties::fromByteArray(const QByteArray& bytes)
{
  QDataStream in(bytes);
  in >> *this;
}

QString Properties::writeString() const
{
  QString string;
  QTextStream stream(&string, QIODevice::WriteOnly);
  write(stream);
  return string;
}

void Properties::readString(QString string)
{
  QTextStream stream(&string, QIODevice::ReadOnly);
  read(stream);
}

void Properties::registerMetaTypes()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  qRegisterMetaTypeStreamOperators<atools::util::Properties>();
#endif
}

} // namespace util
} // namespace atools
