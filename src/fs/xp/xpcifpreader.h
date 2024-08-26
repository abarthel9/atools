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

#ifndef ATOOLS_FS_XP_CIFPREADER_h
#define ATOOLS_FS_XP_CIFPREADER_h

#include "fs/xp/xpreader.h"

#include "sql/sqlrecord.h"

namespace atools {

namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {
class NavDatabaseErrors;
class NavDatabaseOptions;
class ProgressHandler;

namespace common {
class AirportIndex;
class ProcedureWriter;
}

namespace xp {

/*
 * Reads a CIFP file and writes all approaches,transitons, SIDs and STARs into the database.
 */
class XpCifpReader :
  public atools::fs::xp::XpReader
{
public:
  XpCifpReader(atools::sql::SqlDatabase& sqlDb, atools::fs::common::AirportIndex *airportIndexParam,
               const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler,
               atools::fs::NavDatabaseErrors *navdatabaseErrors);
  virtual ~XpCifpReader() override;

  XpCifpReader(const XpCifpReader& other) = delete;
  XpCifpReader& operator=(const XpCifpReader& other) = delete;

  virtual void read(const QStringList& line, const XpReaderContext& context) override;
  virtual void finish(const XpReaderContext& context) override;
  virtual void reset() override;

private:
  atools::fs::common::ProcedureWriter *procWriter = nullptr;
};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_CIFPREADER_h
