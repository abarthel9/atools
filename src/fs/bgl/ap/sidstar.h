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

#ifndef ATOOLS_BGL_AP_SIDSTAR_H
#define ATOOLS_BGL_AP_SIDSTAR_H

#include "fs/bgl/record.h"

#include <QList>
#include <QHash>
#include <QString>

namespace atools {
namespace fs {
namespace bgl {

class ApproachLeg;

class SidStar :
  public atools::fs::bgl::Record
{
public:
  /*
   * Read arrival/departure and all subrecords.
   */
  SidStar(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *stream);
  virtual ~SidStar() override;

  const QHash<QString, QList<atools::fs::bgl::ApproachLeg> >& getEnrouteTransitions() const
  {
    return enrouteTransitions;
  }

  const QHash<QString, QList<atools::fs::bgl::ApproachLeg> >& getRunwayTransitionLegs() const
  {
    return runwayTransitionLegs;
  }

  const QList<atools::fs::bgl::ApproachLeg>& getCommonRouteLegs() const
  {
    return commonRouteLegs;
  }

  const QString& getIdent() const
  {
    return ident;
  }

  char getSuffix() const
  {
    return suffix;
  }

  /* true if all legs have valid values (type, course, etc. ) */
  bool isValid() const override;

  /*
   * Short description for logging purposes
   */
  QString getDescription() const;

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::SidStar& record);

  QList<atools::fs::bgl::ApproachLeg> commonRouteLegs;
  QHash<QString, QList<atools::fs::bgl::ApproachLeg> > enrouteTransitions;
  QHash<QString, QList<atools::fs::bgl::ApproachLeg> > runwayTransitionLegs;

  QString ident;
  char suffix = 0;
};

} /* namespace bgl */
} /* namespace fs */
} /* namespace atools */

#endif /* ATOOLS_BGL_AP_SIDSTAR_H */
