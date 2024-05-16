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

#include "fs/sc/simconnectdata.h"

#include "fs/weather/metar.h"
#include "geo/calculations.h"

#include <QDebug>
#include <QDataStream>
#include <QIODevice>

using atools::fs::weather::Metar;

namespace atools {
namespace fs {
namespace sc {

SimConnectData::SimConnectData()
{

}

SimConnectData::SimConnectData(const SimConnectData& other)
{
  *this = other;
}

SimConnectData::~SimConnectData()
{

}

bool SimConnectData::read(QIODevice *ioDevice)
{
  status = OK;

  QDataStream in(ioDevice);
  in.setVersion(QDataStream::Qt_5_5);
  in.setFloatingPointPrecision(QDataStream::SinglePrecision);

  if(magicNumber == 0)
  {
    if(ioDevice->bytesAvailable() < static_cast<qint64>(sizeof(magicNumber)))
      return false;

    in >> magicNumber;
    if(magicNumber != MAGIC_NUMBER_DATA)
    {
      qWarning() << "SimConnectData::read: invalid magic number" << magicNumber;
      status = INVALID_MAGIC_NUMBER;
      return false;
    }
  }

  if(packetSize == 0)
  {
    if(ioDevice->bytesAvailable() < static_cast<qint64>(sizeof(packetSize)))
      return false;

    in >> packetSize;
  }

  // Wait until the whole packet is available
  if(ioDevice->bytesAvailable() < packetSize)
    return false;

  in >> version;
  if(version != DATA_VERSION)
  {
    qWarning() << "SimConnectData::read: version mismatch" << version << "!=" << DATA_VERSION;
    status = VERSION_MISMATCH;
    return false;
  }
  in >> packetId;

  quint32 ts;
  in >> ts;
  packetTs = QDateTime::fromSecsSinceEpoch(ts, Qt::UTC);

  quint8 hasUser = 0;
  in >> hasUser;
  if(hasUser == 1)
    userAircraft.read(in);

  quint16 numAi = 0;
  in >> numAi;
  for(quint16 i = 0; i < numAi; i++)
  {
    SimConnectAircraft ap;
    ap.read(in);
    aiAircraft.append(ap);
  }

  // Read METARs ==============================================
  quint16 numMetar = 0;
  in >> numMetar;
  for(quint16 i = 0; i < numMetar; i++)
  {
    QString ident;
    readString(in, ident);

    float lonx, laty, altitude;
    quint32 minSinceEpoch;
    in >> lonx >> laty >> altitude >> minSinceEpoch;

    Metar metar(ident, atools::geo::Pos(lonx, laty, altitude), QDateTime::fromMSecsSinceEpoch(minSinceEpoch * 1000, Qt::UTC), QString());
    // Do not parse here - only METAR strings are transferred

    readLongString(in, ident);
    metar.setMetarForStation(ident);

    readLongString(in, ident);
    metar.setMetarForNearest(ident);

    readLongString(in, ident);
    metar.setMetarForInterpolated(ident);

    metar.setFsxP3dFormat();

    metars.append(metar);
  }

  return true;
}

int SimConnectData::write(QIODevice *ioDevice)
{
  status = OK;

  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  out << MAGIC_NUMBER_DATA << packetSize << DATA_VERSION << packetId << static_cast<quint32>(packetTs.toSecsSinceEpoch());

  bool userValid = userAircraft.getPosition().isValid();
  out << static_cast<quint8>(userValid);
  if(userValid)
    userAircraft.write(out);

  qsizetype numAi = std::min(static_cast<qsizetype>(65535), static_cast<qsizetype>(aiAircraft.size()));
  out << static_cast<quint16>(numAi);

  for(int i = 0; i < numAi; i++)
    aiAircraft.at(i).write(out);

  // Write METARs ==============================================
  qsizetype numMetar = std::min(static_cast<qsizetype>(65535), static_cast<qsizetype>(metars.size()));
  out << static_cast<quint16>(numMetar);

  for(int i = 0; i < numMetar; i++)
  {
    const Metar& metar = metars.at(i);
    writeString(out, metar.getRequestIdent());
    out << metar.getRequestPos().getLonX() << metar.getRequestPos().getLatY() << metar.getRequestPos().getAltitude()
        << static_cast<quint32>(metar.getTimestamp().toSecsSinceEpoch());
    writeLongString(out, metar.getStationMetar());
    writeLongString(out, metar.getNearestMetar());
    writeLongString(out, metar.getInterpolatedMetar());
  }

  // Go back and update size
  out.device()->seek(sizeof(MAGIC_NUMBER_DATA));
  int size = block.size() - static_cast<int>(sizeof(packetSize)) - static_cast<int>(sizeof(MAGIC_NUMBER_DATA));
  out << static_cast<quint32>(size);

  return SimConnectDataBase::writeBlock(ioDevice, block, status);
}

SimConnectAircraft *SimConnectData::getAiAircraftById(int id)
{
  if(aiAircraftIndex.contains(id))
    return &aiAircraft[aiAircraftIndex.value(id)];
  else
    return nullptr;
}

const SimConnectAircraft *SimConnectData::getAiAircraftConstById(int id) const
{
  if(aiAircraftIndex.contains(id))
    return &aiAircraft.at(aiAircraftIndex.value(id));
  else
    return nullptr;
}

SimConnectData SimConnectData::buildDebugForPosition(const geo::Pos& pos, const geo::Pos& lastPos, bool ground,
                                                     float vertSpeed, float tas, float fuelflow, float totalFuel, float ice,
                                                     float flightplanAlt, float magVar, bool jetFuel, bool helicopter)
{
  static QVector<float> lastHdgs;
  lastHdgs.fill(0.f, 10);

  SimConnectData data;
  data.userAircraft.position = pos;
  // data.userAircraft.position.setAltitude(1000);

  float headingTrue = 0.f;
  if(lastPos.isValid())
  {
    headingTrue = !lastPos.almostEqual(pos, atools::geo::Pos::POS_EPSILON_10M) ? lastPos.angleDegTo(pos) : 0.f;
    data.userAircraft.indicatedSpeedKts = tas;
    data.userAircraft.trueAirspeedKts = tas + 10;
    data.userAircraft.groundSpeedKts = tas + 20;
  }

  data.userAircraft.trackMagDeg = atools::geo::normalizeCourse(headingTrue - magVar);
  data.userAircraft.trackTrueDeg = headingTrue;
  data.userAircraft.headingMagDeg = atools::geo::normalizeCourse(headingTrue - magVar);
  data.userAircraft.headingTrueDeg = headingTrue;
  data.userAircraft.magVarDeg = magVar;

  data.userAircraft.pitotIcePercent = static_cast<quint8>(ice);
  data.userAircraft.structuralIcePercent = static_cast<quint8>(ice / 2);
  data.userAircraft.carbIcePercent = static_cast<quint8>(ice / 3);
  data.userAircraft.statIcePercent = static_cast<quint8>(ice / 4);
  data.userAircraft.windowIcePercent = static_cast<quint8>(ice / 5);
  data.userAircraft.aoaIcePercent = static_cast<quint8>(ice > 0.f ? 1 : 0);
  data.userAircraft.inletIcePercent = static_cast<quint8>(ice > 0.f ? 100 : 0);
  data.userAircraft.category = helicopter ? HELICOPTER : AIRPLANE;
  data.userAircraft.engineType = PISTON;
  data.userAircraft.zuluDateTime = QDateTime::currentDateTimeUtc();
  data.userAircraft.localDateTime = QDateTime::currentDateTime();

  data.userAircraft.airplaneTitle = "Beech Baron 58 Paint 1";
  data.userAircraft.airplaneType = "Beechcraft";
  data.userAircraft.airplaneModel = "BE58";
  data.userAircraft.airplaneReg = "N12345";
  data.userAircraft.airplaneAirline = "Airline";
  data.userAircraft.airplaneFlightnumber = "965";
  data.userAircraft.fromIdent = "EDDF";
  data.userAircraft.transponderCode = 00123; // Octal code (4095)

  data.userAircraft.verticalSpeedFeetPerMin = vertSpeed;

  data.userAircraft.windDirectionDegT = atools::geo::normalizeCourse(headingTrue + 45.f);
  data.userAircraft.windSpeedKts = 19.f;

  data.userAircraft.toIdent = "LIRF";
  data.userAircraft.altitudeAboveGroundFt = pos.getAltitude();
  data.userAircraft.indicatedAltitudeFt = pos.getAltitude();

  if(vertSpeed < 50.f)
    data.userAircraft.altitudeAutopilotFt = flightplanAlt * 0.5f;
  else if(vertSpeed > 50.f)
    data.userAircraft.altitudeAutopilotFt = flightplanAlt * 0.75f;
  else
    data.userAircraft.altitudeAutopilotFt = flightplanAlt;

  data.userAircraft.airplaneEmptyWeightLbs = 1500.f;
  data.userAircraft.airplaneTotalWeightLbs = 3000.f;
  data.userAircraft.airplaneMaxGrossWeightLbs = 4000.f;
  data.userAircraft.fuelTotalWeightLbs = totalFuel;
  data.userAircraft.fuelTotalQuantityGallons = atools::geo::fromLbsToGal(jetFuel, data.userAircraft.fuelTotalWeightLbs);
  data.userAircraft.fuelFlowPPH = fuelflow;
  data.userAircraft.fuelFlowGPH = atools::geo::fromLbsToGal(jetFuel, fuelflow);
  data.userAircraft.flags = IS_USER | (ground ? ON_GROUND : NONE);
  data.userAircraft.seaLevelPressureMbar = 1013.25f;
  data.userAircraft.ambientTemperatureCelsius = 10.f;
  data.userAircraft.totalAirTemperatureCelsius = 15.f;

  data.userAircraft.debug = true;

  return data;
}

void SimConnectData::updateIndexesAndKeys()
{
  userAircraft.updateAirplaneRegistrationKey();

  aiAircraftIndex.clear();
  for(int i = 0; i < aiAircraft.size(); i++)
  {
    atools::fs::sc::SimConnectAircraft& aircraft = aiAircraft[i];

    aircraft.updateAirplaneRegistrationKey();
    aiAircraftIndex.insert(aircraft.getId(), i);
  }

}

} // namespace sc
} // namespace fs
} // namespace atools
