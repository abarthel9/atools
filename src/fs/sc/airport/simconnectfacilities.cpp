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

#include "fs/sc/airport/simconnectfacilities.h"

#include "fs/bgl/ap/com.h"
#include "fs/bgl/ap/parking.h"

#include <QVariant>

namespace atools {
namespace fs {
namespace sc {
namespace airport {

const QVariant Airport::getTowerFrequency() const
{
  for(const FrequencyFacility& frequency : frequencies)
  {
    if(frequency.type == atools::fs::bgl::com::TOWER)
      return frequency.frequency;
  }
  return QVariant(QVariant::Int);
}

const QVariant Airport::getAtisFrequency() const
{
  for(const FrequencyFacility& frequency : frequencies)
  {
    if(frequency.type == atools::fs::bgl::com::ATIS)
      return frequency.frequency;
  }
  return QVariant(QVariant::Int);
}

const QVariant Airport::getAwosFrequency() const
{
  for(const FrequencyFacility& frequency : frequencies)
  {
    if(frequency.type == atools::fs::bgl::com::AWOS)
      return frequency.frequency;
  }
  return QVariant(QVariant::Int);
}

const QVariant Airport::getAsosFrequency() const
{
  for(const FrequencyFacility& frequency : frequencies)
  {
    if(frequency.type == atools::fs::bgl::com::ASOS)
      return frequency.frequency;
  }
  return QVariant(QVariant::Int);

}

const QVariant Airport::getUnicomFrequency() const
{
  for(const FrequencyFacility& frequency : frequencies)
  {
    if(frequency.type == atools::fs::bgl::com::UNICOM)
      return frequency.frequency;
  }
  return QVariant(QVariant::Int);
}

int Airport::getNumParkingGate() const
{
  int num = 0;
  for(const TaxiParkingFacility& parking : taxiParkings)
  {
    if(bgl::isGate(static_cast<bgl::ap::ParkingType>(parking.type)))
      num++;
  }
  return num;
}

int Airport::getNumParkingGaRamp() const
{
  int num = 0;
  for(const TaxiParkingFacility& parking : taxiParkings)
  {
    if(bgl::isRamp(static_cast<bgl::ap::ParkingType>(parking.type)))
      num++;
  }
  return num;
}

int Airport::getNumParkingCargo() const
{
  int num = 0;
  for(const TaxiParkingFacility& parking : taxiParkings)
  {
    if(bgl::isCargo(static_cast<bgl::ap::ParkingType>(parking.type)))
      num++;
  }
  return num;
}

int Airport::getNumParkingMilCargo() const
{
  int num = 0;
  for(const TaxiParkingFacility& parking : taxiParkings)
  {
    if(bgl::isMilCargo(static_cast<bgl::ap::ParkingType>(parking.type)))
      num++;
  }
  return num;
}

int Airport::getNumParkingMilCombat() const
{
  int num = 0;
  for(const TaxiParkingFacility& parking : taxiParkings)
  {
    if(bgl::isMilCombat(static_cast<bgl::ap::ParkingType>(parking.type)))
      num++;
  }
  return num;
}

int Airport::getNumRunwayHard() const
{
  int num = 0;
  for(const Runway& runway : runways)
  {
    if(runway.isHard())
      num++;
  }
  return num;
}

int Airport::getNumRunwaySoft() const
{
  int num = 0;
  for(const Runway& runway : runways)
  {
    if(runway.isSoft())
      num++;
  }
  return num;
}

int Airport::getNumRunwayWater() const
{
  int num = 0;
  for(const Runway& runway : runways)
  {
    if(runway.isWater())
      num++;
  }
  return num;
}

int Airport::getNumRunwayEndVasi() const
{
  int num = 0;
  for(const Runway& runway : runways)
  {
    // Primary end left/right
    if(runway.getVasiFacilities().at(0).type > 0 || runway.getVasiFacilities().at(1).type > 0)
      num++;

    // Secondary end left/right
    if(runway.getVasiFacilities().at(2).type > 0 || runway.getVasiFacilities().at(3).type > 0)
      num++;
  }
  return num;
}

int Airport::getNumRunwayEndAls() const
{
  int num = 0;
  for(const Runway& runway : runways)
  {
    // Primary end
    if(runway.getApproachLightFacilities().at(0).system > 0)
      num++;

    // Secondary end
    if(runway.getApproachLightFacilities().at(1).system > 0)
      num++;
  }
  return num;
}

int Airport::getNumRunwayEndIls() const
{
  int num = 0;
  for(const Runway& runway : runways)
  {
    if(strlen(runway.getFacility().primaryIlsIcao) > 0)
      num++;

    if(strlen(runway.getFacility().secondaryIlsIcao) > 0)
      num++;
  }
  return num;
}

int Airport::getLongestRunwayIndex() const
{
  int index = -1;
  float longest = 0.f;

  for(int i = 0; i < runways.size(); i++)
  {
    const Runway& runway = runways.at(i);

    if(runway.getFacility().length > longest)
    {
      longest = runway.getFacility().length;
      index = i;
    }
  }

  return index;
}

const QVariant Airport::getLargestParkingGate() const
{
  int index = -1;
  float maxRadius = 0.f;

  for(int i = 0; i < taxiParkings.size(); i++)
  {
    const TaxiParkingFacility& parking = taxiParkings.at(i);

    if(bgl::isGate(static_cast<bgl::ap::ParkingType>(parking.type)) && parking.radius > maxRadius)
    {
      maxRadius = parking.radius;
      index = i;
    }
  }

  if(index >= 0)
    return atools::fs::bgl::Parking::parkingTypeToStr(static_cast<atools::fs::bgl::ap::ParkingType>(taxiParkings.at(index).type));
  else
    return QVariant(QVariant::String);
}

const QVariant Airport::getLargestParkingRamp() const
{
  int index = -1;
  float maxRadius = 0.f;

  for(int i = 0; i < taxiParkings.size(); i++)
  {
    const TaxiParkingFacility& parking = taxiParkings.at(i);

    if(bgl::isRamp(static_cast<bgl::ap::ParkingType>(parking.type)) && parking.radius > maxRadius)
    {
      maxRadius = parking.radius;
      index = i;
    }
  }

  if(index >= 0)
    return atools::fs::bgl::Parking::parkingTypeToStr(static_cast<atools::fs::bgl::ap::ParkingType>(taxiParkings.at(index).type));
  else
    return QVariant(QVariant::String);
}

bool Runway::isHard() const
{
  Surface surface = static_cast<Surface>(runway.surface);
  if(surface == CONCRETE)
    return true;
  else if(surface == ASPHALT)
    return true;
  else if(surface == BITUMINUS)
    return true;
  else if(surface == TARMAC)
    return true;

  return false;
}

bool Runway::isWater() const
{
  Surface surface = static_cast<Surface>(runway.surface);
  return surface == WATER || surface == WATER_FSX || surface == POND || surface == LAKE || surface == RIVER || surface == OCEAN;
}

bool Runway::isSoft() const
{
  return !isWater() && !isHard();
}

QVariant surfaceToDb(Surface surface)
{
  switch(surface)
  {
    case CONCRETE:
      return "C";

    case WRIGHT_FLYER_TRACK:
    case HARD_TURF:
    case GRASS_BUMPY:
    case SHORT_GRASS:
    case LONG_GRASS:
    case GRASS:
    case FOREST:
      return "G"; // Grass

    case WATER_FSX:
    case OCEAN:
    case POND:
    case LAKE:
    case RIVER:
    case WASTE_WATER:
    case WATER:
      return "W"; // Water

    case URBAN:
    case ASPHALT:
      return "A"; // Asphalt

    case SNOW:
      return "SN";

    case ICE:
      return "I";

    case DIRT:
      return "D";

    case CORAL:
      return "CR";

    case GRAVEL:
      return "GR";

    case PAINT:
    case OIL_TREATED:
      return "OT";

    case STEEL_MATS:
      return "SM";

    case BITUMINUS:
      return "B";

    case BRICK:
      return "BR";

    case MACADAM:
      return "M";

    case PLANKS:
      return "PL";

    case SAND:
      return "S";

    case SHALE:
      return "SH";

    case TARMAC:
      return "T";

    case UNKNOWN:
    case UNDEFINED:
      return "UNKNOWN";
  }
  return "UNKNOWN";
}

} // namespace airport
} // namespace sc
} // namespace fs
} // namespace atools
