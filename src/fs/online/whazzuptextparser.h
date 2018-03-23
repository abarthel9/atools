/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_WHAZZUPTEXTPARSER_H
#define ATOOLS_FS_WHAZZUPTEXTPARSER_H

#include "geo/pos.h"
#include "fs/online/onlinetypes.h"

#include <QDateTime>
#include <QString>

class QTextStream;

namespace atools {
namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {
namespace online {

/*
 * Reads a "whazzup.txt" file and stores all found data in the database.
 * Schema has to be created before.
 *
 * Supported formats are the ones used by VATSIM and IVAO.
 */
class WhazzupTextParser
{
public:
  WhazzupTextParser(atools::sql::SqlDatabase *sqlDb);
  virtual ~WhazzupTextParser();

  /* Read file content given in string and store results in database. Commit is executed when done. */
  void read(QString file, atools::fs::online::Format streamFormat);

  /* Read file content given in stream and store results in database. Commit is executed when done. */
  void read(QTextStream& stream, atools::fs::online::Format streamFormat);

  /* Create all queries */
  void initQueries();

  /* Delete all queries */
  void deInitQueries();

  /*  Time in minutes to wait before allowing manual Atis refresh by way of web page interface */
  int getAtisAllowMinutes() const
  {
    return atisAllowMin;
  }

  /* The last date and time this file has been updated. */
  QDateTime getLastUpdateTime() const
  {
    return update;
  }

  /* Time in minutes this file will be updated */
  int getReloadMinutes() const
  {
    return reload;
  }

  void reset()
  {
    curSection.clear();
    version = reload = atisAllowMin = 0;
    format = atools::fs::online::UNKNOWN;
    update.setSecsSinceEpoch(0);
  }

private:
  void parseGeneralSection(const QString& line);
  void parseSection(sql::SqlQuery *insertQuery, const QStringList& line);
  void parseServersSection(const QString& line);
  void parseVoiceSection(const QString& line);
  void parseAirportSection(const QString& line);

  QString curSection;
  atools::fs::online::Format format = atools::fs::online::UNKNOWN;

  /* Data format version */
  int version = 0;

  /* Time in minutes this file will be updated */
  int reload = 0;

  /* The last date and time this file has been updated. */
  QDateTime update;

  /*  Time in minutes to wait before allowing manual Atis refresh by way of web page interface */
  int atisAllowMin = 0;

  atools::sql::SqlDatabase *db;
  atools::sql::SqlQuery *clientInsertQuery = nullptr, *prefileInsertQuery = nullptr, *atcInsertQuery = nullptr,
                        *serverInsertQuery = nullptr, *voiceInsertQuery = nullptr, *airportInsertQuery = nullptr;

};

} // namespace online
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_WHAZZUPTEXTPARSER_H
