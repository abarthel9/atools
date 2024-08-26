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

#include "fs/xp/xpmorareader.h"

#include "fs/common/morareader.h"

namespace atools {
namespace fs {
namespace xp {

XpMoraReader::XpMoraReader(atools::sql::SqlDatabase& sqlDb, const NavDatabaseOptions& opts,
                           ProgressHandler *progressHandler, atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : XpReader(sqlDb, opts, progressHandler, navdatabaseErrors)
{
}

XpMoraReader::~XpMoraReader()
{
}

void XpMoraReader::read(const QStringList& line, const XpReaderContext& context)
{
  ctx = &context;

  // Add all lines with correct size to field
  if(line.size() == 32)
    lines.append(line);
}

void XpMoraReader::finish(const XpReaderContext& context)
{
  Q_UNUSED(context)

  // Read from text lines and write to MORA database table
  atools::fs::common::MoraReader morareader(db);
  morareader.fillDbFromFile(lines, context.curFileId);
}

void XpMoraReader::reset()
{
  lines.clear();
}

} // namespace xp
} // namespace fs
} // namespace atools
