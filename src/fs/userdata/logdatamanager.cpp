/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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

#include "fs/userdata/logdatamanager.h"

#include "fs/gpx/gpxio.h"
#include "sql/sqltransaction.h"
#include "sql/sqlutil.h"
#include "sql/sqlexport.h"
#include "sql/sqldatabase.h"
#include "util/csvreader.h"
#include "geo/pos.h"
#include "zip/gzip.h"
#include "geo/calculations.h"
#include "exception.h"
#include "sql/sqlcolumn.h"

#include <QDateTime>
#include <QDir>
#include <QStringBuilder>

namespace atools {
namespace fs {
namespace userdata {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::endl;
#endif

using atools::geo::Pos;
using atools::sql::SqlUtil;
using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlExport;
using atools::sql::SqlRecord;
using atools::sql::SqlTransaction;
using atools::sql::SqlColumn;

/* *INDENT-OFF* */
namespace csv {
/* Column indexes in CSV format */
enum Index
{
  // LOGBOOK_ID,- not in CSV  // logbook_id
  FIRST_COL,
  AIRCRAFT_NAME = FIRST_COL,  // aircraft_name
  AIRCRAFT_TYPE,              // aircraft_type
  AIRCRAFT_REGISTRATION,      // aircraft_registration
  FLIGHTPLAN_NUMBER,          // flightplan_number
  FLIGHTPLAN_CRUISE_ALTITUDE, // flightplan_cruise_altitude
  FLIGHTPLAN_FILE,            // flightplan_file
  PERFORMANCE_FILE,           // performance_file
  BLOCK_FUEL,                 // block_fuel
  TRIP_FUEL,                  // trip_fuel
  USED_FUEL,                  // used_fuel
  IS_JETFUEL,                 // is_jetfuel
  GROSSWEIGHT,                // grossweight
  DISTANCE,                   // distance
  DISTANCE_FLOWN,             // distance_flown
  DEPARTURE_IDENT,            // departure_ident
  DEPARTURE_NAME,             // departure_name
  DEPARTURE_RUNWAY,           // departure_runway
  DEPARTURE_LONX,             // departure_lonx
  DEPARTURE_LATY,             // departure_laty
  DEPARTURE_ALT,              // departure_alt
  DEPARTURE_TIME,             // departure_time
  DEPARTURE_TIME_SIM,         // departure_time_sim
  DESTINATION_IDENT,          // destination_ident
  DESTINATION_NAME,           // destination_name
  DESTINATION_RUNWAY,         // destination_runway
  DESTINATION_LONX,           // destination_lonx
  DESTINATION_LATY,           // destination_laty
  DESTINATION_ALT,            // destination_alt
  DESTINATION_TIME,           // destination_time
  DESTINATION_TIME_SIM,       // destination_time_sim
  ROUTE_STRING,               // route_string
  SIMULATOR,                  // simulator
  DESCRIPTION,                // description
  FLIGHTPLAN,                 // flightplan
  AIRCRAFT_PERF,              // aircraft_perf
  AIRCRAFT_TRAIL,             // aircraft_trail
  LAST_COL = AIRCRAFT_TRAIL
};

const static int MIN_NUM_COLS = DESTINATION_TIME_SIM + 1;

const static QString HEADER_LINE = "aircraftname,aircrafttype,aircraftregistration,flightplannumber,";
const static QString HEADER_LINE2 = "aircraft_name,aircraft_type,aircraft_registration,flightplan_number,";

/* Map index to column names. Needed to keep the export order independent of the column order in the table.
 * Columns display names are used in CSV and are not to be translated. */
const static QHash<int, SqlColumn> COL_MAP =
{
  // LOGBOOK_ID,- not in CSV    logbook_id
  {AIRCRAFT_NAME,              SqlColumn(AIRCRAFT_NAME,              "aircraft_name",              "Aircraft Name"             )},
  {AIRCRAFT_TYPE,              SqlColumn(AIRCRAFT_TYPE,              "aircraft_type",              "Aircraft Type"             )},
  {AIRCRAFT_REGISTRATION,      SqlColumn(AIRCRAFT_REGISTRATION,      "aircraft_registration",      "Aircraft Registration"     )},
  {FLIGHTPLAN_NUMBER,          SqlColumn(FLIGHTPLAN_NUMBER,          "flightplan_number",          "Flightplan Number"         )},
  {FLIGHTPLAN_CRUISE_ALTITUDE, SqlColumn(FLIGHTPLAN_CRUISE_ALTITUDE, "flightplan_cruise_altitude", "Flightplan Cruise Altitude")},
  {FLIGHTPLAN_FILE,            SqlColumn(FLIGHTPLAN_FILE,            "flightplan_file",            "Flightplan File"           )},
  {PERFORMANCE_FILE,           SqlColumn(PERFORMANCE_FILE,           "performance_file",           "Performance File"          )},
  {BLOCK_FUEL,                 SqlColumn(BLOCK_FUEL,                 "block_fuel",                 "Block Fuel"                )},
  {TRIP_FUEL,                  SqlColumn(TRIP_FUEL,                  "trip_fuel",                  "Trip Fuel"                 )},
  {USED_FUEL,                  SqlColumn(USED_FUEL,                  "used_fuel",                  "Used Fuel"                 )},
  {IS_JETFUEL,                 SqlColumn(IS_JETFUEL,                 "is_jetfuel",                 "Is Jetfuel"                )},
  {GROSSWEIGHT,                SqlColumn(GROSSWEIGHT,                "grossweight",                "Grossweight"               )},
  {DISTANCE,                   SqlColumn(DISTANCE,                   "distance",                   "Distance"                  )},
  {DISTANCE_FLOWN,             SqlColumn(DISTANCE_FLOWN,             "distance_flown",             "Distance Flown"            )},
  {DEPARTURE_IDENT,            SqlColumn(DEPARTURE_IDENT,            "departure_ident",            "Departure Ident"           )},
  {DEPARTURE_NAME,             SqlColumn(DEPARTURE_NAME,             "departure_name",             "Departure Name"            )},
  {DEPARTURE_RUNWAY,           SqlColumn(DEPARTURE_RUNWAY,           "departure_runway",           "Departure Runway"          )},
  {DEPARTURE_LONX,             SqlColumn(DEPARTURE_LONX,             "departure_lonx",             "Departure Lonx"            )},
  {DEPARTURE_LATY,             SqlColumn(DEPARTURE_LATY,             "departure_laty",             "Departure Laty"            )},
  {DEPARTURE_ALT,              SqlColumn(DEPARTURE_ALT,              "departure_alt",              "Departure Alt"             )},
  {DEPARTURE_TIME,             SqlColumn(DEPARTURE_TIME,             "departure_time",             "Departure Time"            )},
  {DEPARTURE_TIME_SIM,         SqlColumn(DEPARTURE_TIME_SIM,         "departure_time_sim",         "Departure Time Sim"        )},
  {DESTINATION_IDENT,          SqlColumn(DESTINATION_IDENT,          "destination_ident",          "Destination Ident"         )},
  {DESTINATION_NAME,           SqlColumn(DESTINATION_NAME,           "destination_name",           "Destination Name"          )},
  {DESTINATION_RUNWAY,         SqlColumn(DESTINATION_RUNWAY,         "destination_runway",         "Destination Runway"        )},
  {DESTINATION_LONX,           SqlColumn(DESTINATION_LONX,           "destination_lonx",           "Destination Lonx"          )},
  {DESTINATION_LATY,           SqlColumn(DESTINATION_LATY,           "destination_laty",           "Destination Laty"          )},
  {DESTINATION_ALT,            SqlColumn(DESTINATION_ALT,            "destination_alt",            "Destination Alt"           )},
  {DESTINATION_TIME,           SqlColumn(DESTINATION_TIME,           "destination_time",           "Destination Time"          )},
  {DESTINATION_TIME_SIM,       SqlColumn(DESTINATION_TIME_SIM,       "destination_time_sim",       "Destination Time Sim"      )},
  {ROUTE_STRING,               SqlColumn(ROUTE_STRING,               "route_string",               "Route String"              )},
  {SIMULATOR,                  SqlColumn(SIMULATOR,                  "simulator",                  "Simulator"                 )},
  {DESCRIPTION,                SqlColumn(DESCRIPTION,                "description",                "Description"               )},
  {FLIGHTPLAN,                 SqlColumn(FLIGHTPLAN,                 "flightplan",                 "Flightplan"                )},
  {AIRCRAFT_PERF,              SqlColumn(AIRCRAFT_PERF,              "aircraft_perf",              "Aircraft Perf"             )},
  {AIRCRAFT_TRAIL,             SqlColumn(AIRCRAFT_TRAIL,             "aircraft_trail",             "Aircraft Trail"            )}
};
}
/* *INDENT-ON* */

const static QStringList CLEANUP_COLUMNS({"departure_ident", "destination_ident", "distance_flown"});

LogdataManager::LogdataManager(sql::SqlDatabase *sqlDb)
  : DataManagerBase(sqlDb, "logbook", "logbook_id",
                    {":/atools/resources/sql/fs/logbook/create_logbook_schema.sql"},
                    ":/atools/resources/sql/fs/logbook/create_logbook_schema_undo.sql",
                    ":/atools/resources/sql/fs/logbook/drop_logbook_schema.sql"), cache(MAX_CACHE_ENTRIES)
{

}

LogdataManager::~LogdataManager()
{

}

int LogdataManager::importCsv(const QString& filepath)
{
  int numImported = 0;
  QFile file(filepath);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    int id = getCurrentId() + 1;
    atools::sql::DataManagerUndoHandler undoHandler(this, id);
    QString idBinding(":" % idColumnName);

    // Autogenerate id - exclude logbook_id from insert
    SqlQuery insertQuery(db);
    insertQuery.prepare(SqlUtil(db).buildInsertStatement(tableName, QString(), QStringList(), true /* namedBindings */));

    atools::util::CsvReader reader;

    QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    stream.setCodec("UTF-8");
#endif

    int lineNum = 1;
    while(!stream.atEnd())
    {
      QString line = stream.readLine();

      if(lineNum == 1)
      {
        QString header = QString(line).simplified().replace(' ', QString()).replace('"', QString()).toLower();
        if(header.startsWith(csv::HEADER_LINE) || header.startsWith(csv::HEADER_LINE2))
        {
          lineNum++;
          // Ignore header
          continue;
        }
      }

      // Skip empty lines but add them if within an escaped field
      if(line.isEmpty() && !reader.isInEscape())
        continue;

      reader.readCsvLine(line);
      if(reader.isInEscape())
        // Still in an escaped line so continue to read unchanged until " shows the end of the field
        continue;

      const QStringList& values = reader.getValues();

      if(values.size() < csv::MIN_NUM_COLS)
        throw atools::Exception(tr("File contains invalid data.\n\"%1\"\nLine %2.").arg(line).arg(lineNum));

      if(at(values, csv::DEPARTURE_IDENT).isEmpty() && at(values, csv::DESTINATION_IDENT).isEmpty())
        throw atools::Exception(tr("File is not valid. Neither departure nor destination ident is set.\n\"%1\"\nLine %2.").arg(line).arg(
                                  lineNum));

      insertQuery.bindValue(idBinding, id++);

      // Aircraft ===============================================================
      insertQuery.bindValue(":aircraft_name", at(values, csv::AIRCRAFT_NAME));
      insertQuery.bindValue(":aircraft_type", at(values, csv::AIRCRAFT_TYPE));
      insertQuery.bindValue(":aircraft_registration", at(values, csv::AIRCRAFT_REGISTRATION));

      // Flightplan ===============================================================
      insertQuery.bindValue(":flightplan_number", at(values, csv::FLIGHTPLAN_NUMBER));
      if(!at(values, csv::FLIGHTPLAN_CRUISE_ALTITUDE).isEmpty())
        insertQuery.bindValue(":flightplan_cruise_altitude", atFloat(values, csv::FLIGHTPLAN_CRUISE_ALTITUDE, true));
      insertQuery.bindValue(":flightplan_file", at(values, csv::FLIGHTPLAN_FILE));

      // Trip ===============================================================
      insertQuery.bindValue(":performance_file", at(values, csv::PERFORMANCE_FILE));
      if(!at(values, csv::BLOCK_FUEL).isEmpty())
        insertQuery.bindValue(":block_fuel", atFloat(values, csv::BLOCK_FUEL, true));
      if(!at(values, csv::TRIP_FUEL).isEmpty())
        insertQuery.bindValue(":trip_fuel", atFloat(values, csv::TRIP_FUEL, true));
      if(!at(values, csv::USED_FUEL).isEmpty())
        insertQuery.bindValue(":used_fuel", atFloat(values, csv::USED_FUEL, true));
      if(!at(values, csv::IS_JETFUEL).isEmpty())
        insertQuery.bindValue(":is_jetfuel", atInt(values, csv::IS_JETFUEL, true));
      if(!at(values, csv::GROSSWEIGHT).isEmpty())
        insertQuery.bindValue(":grossweight", atFloat(values, csv::GROSSWEIGHT, true));
      if(!at(values, csv::DISTANCE).isEmpty())
        insertQuery.bindValue(":distance", atFloat(values, csv::DISTANCE, true));
      if(!at(values, csv::DISTANCE_FLOWN).isEmpty())
        insertQuery.bindValue(":distance_flown", atFloat(values, csv::DISTANCE_FLOWN, true));

      // Departure ===============================================================
      insertQuery.bindValue(":departure_ident", at(values, csv::DEPARTURE_IDENT));
      insertQuery.bindValue(":departure_name", at(values, csv::DEPARTURE_NAME));
      insertQuery.bindValue(":departure_runway", at(values, csv::DEPARTURE_RUNWAY));

      if(!at(values, csv::DEPARTURE_LONX).isEmpty() && !at(values, csv::DEPARTURE_LATY).isEmpty())
      {
        Pos departPos = validateCoordinates(line, at(values, csv::DEPARTURE_LONX), at(values, csv::DEPARTURE_LATY),
                                            lineNum, true /* checkNull */);

        if(departPos.isValid())
        {
          insertQuery.bindValue(":departure_lonx", departPos.getLonX());
          insertQuery.bindValue(":departure_laty", departPos.getLatY());
        }
      }
      if(!at(values, csv::DEPARTURE_ALT).isEmpty())
        insertQuery.bindValue(":departure_alt", atFloat(values, csv::DEPARTURE_ALT, true));

      insertQuery.bindValue(":departure_time", QDateTime::fromString(at(values, csv::DEPARTURE_TIME, true), Qt::ISODate));
      insertQuery.bindValue(":departure_time_sim", QDateTime::fromString(at(values, csv::DEPARTURE_TIME_SIM, true), Qt::ISODate));

      // Destination ===============================================================
      insertQuery.bindValue(":destination_ident", at(values, csv::DESTINATION_IDENT));
      insertQuery.bindValue(":destination_name", at(values, csv::DESTINATION_NAME));
      insertQuery.bindValue(":destination_runway", at(values, csv::DESTINATION_RUNWAY));

      if(!at(values, csv::DESTINATION_LONX).isEmpty() && !at(values, csv::DESTINATION_LATY).isEmpty())
      {
        Pos destPos = validateCoordinates(line, at(values, csv::DESTINATION_LONX), at(values, csv::DESTINATION_LATY),
                                          lineNum, true /* checkNull */);
        if(destPos.isValid())
        {
          insertQuery.bindValue(":destination_lonx", destPos.getLonX());
          insertQuery.bindValue(":destination_laty", destPos.getLatY());
        }
      }

      if(!at(values, csv::DESTINATION_ALT).isEmpty())
        insertQuery.bindValue(":destination_alt", atFloat(values, csv::DESTINATION_ALT, true));

      insertQuery.bindValue(":destination_time", QDateTime::fromString(at(values, csv::DESTINATION_TIME, true), Qt::ISODate));
      insertQuery.bindValue(":destination_time_sim", QDateTime::fromString(at(values, csv::DESTINATION_TIME_SIM, true), Qt::ISODate));

      // Other ===============================================================
      insertQuery.bindValue(":route_string", at(values, csv::ROUTE_STRING));
      insertQuery.bindValue(":simulator", at(values, csv::SIMULATOR));
      insertQuery.bindValue(":description", at(values, csv::DESCRIPTION));

      // Add files as Gzipped BLOBS ===========================================
      insertQuery.bindValue(":flightplan", atools::zip::gzipCompress(at(values, csv::FLIGHTPLAN, true /* nowarn */).toUtf8()));
      insertQuery.bindValue(":aircraft_perf", atools::zip::gzipCompress(at(values, csv::AIRCRAFT_PERF, true /* nowarn */).toUtf8()));
      insertQuery.bindValue(":aircraft_trail", atools::zip::gzipCompress(at(values, csv::AIRCRAFT_TRAIL, true /* nowarn */).toUtf8()));

      // Fill null fields with empty strings to avoid issues when searching
      // Also turn empty BLOBs to NULL
      fixEmptyFields(insertQuery);

      insertQuery.exec();
      undoHandler.inserted();

      // Reset unassigned fields to null
      insertQuery.clearBoundValues();

      lineNum++;
      numImported++;
    }

    file.close();
    undoHandler.finish();
  } // if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));

  return numImported;
}

// https://www.x-plane.com/manuals/desktop/index.html#keepingalogbook
// Each time an aircraft is flown in X-Plane, the program logs the flight time in a digital logbook.
// By default, X‑Plane creates a text file called “X-Plane Pilot.txt” in the ‘X-Plane 11/Output/logbooks directory’.
// Inside this text file are the following details of previous flights:
// 0	 Dates of flights
// 1	 Tail numbers of aircraft
// 2	 Aircraft types
// 3	 Airports of departure and arrival
// 4	 Number of landings
// 5	 Duration of flights
// 6	 Time spent flying cross-country, in IFR conditions, and at night
// 7	 Total time of all flights
//
// I
// 1 Version
// 2 170722    EDFE    EDFE   2   0.3   0.0   0.0   0.0  N172SP  Cessna_172SP
// 2 170722    EDXW    EDXW   1   0.2   0.0   0.0   0.2   N45XS  Baron_58
// 2 170724    PHNL    PHNL   0   0.1   0.0   0.0   0.0  N172SP  Cessna_172SP
// 2 170724    PHNL    PHNL   1   0.1   0.0   0.0   0.0  N172SP  Cessna_172SP
// 2 170724    KBFI    KBFI   0   0.1   0.0   0.0   0.0  N172SP  Cessna_172SP
// ...
// 2 190612    EDXW    EDXW   0   0.2   0.0   0.0   0.0    SF34
// 2 190612    EDXW    EDXW   2   0.2   0.0   0.0   0.0  G-LSCL  SF34
// 2 190612    EDXW    EDXW   2   0.2   0.0   0.0   0.0  Car_690B_TurboCommander
// 2 190620    CYPS    CYPS   0   0.1   0.1   0.0   0.0  N410LT  E1000G
// 2 190620    CYPS     BQ8   3   0.5   0.0   0.0   0.0  C-JEFF  E1000G
// 2 190620    FHAW    FHAW   0   0.1   0.0   0.0   0.0  N7779E  Car_B1900D
// 99
int LogdataManager::importXplane(const QString& filepath,
                                 const std::function<void(atools::geo::Pos& pos, QString& name,
                                                          const QString& ident)>& fetchAirport)
{
  using atools::at;
  using atools::atFloat;
  using atools::atInt;

  enum XPlaneLogbookIdx
  {
    PREFIX,
    DATE,
    DEPARTURE,
    DESTINATION,
    NUM_LANDINGS,
    TIME,
    TIME_CROSS_COUNTRY,
    TIME_IFR,
    TIME_NIGHT,
    TAIL_NUMBER,
    AIRCRAFT_TYPE
  };

  // Autogenerate id
  SqlQuery insertQuery(db);
  insertQuery.prepare(SqlUtil(db).buildInsertStatement(tableName, QString(), QStringList(), true /* namedBindings */));

  int numImported = 0;
  QFile file(filepath);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    int id = getCurrentId() + 1;
    atools::sql::DataManagerUndoHandler undoHandler(this, id);
    QString idBinding(":" % idColumnName);

    QString filename = QFileInfo(filepath).fileName();

    QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    stream.setCodec("UTF-8");
#endif
    int lineNum = 1;
    while(!stream.atEnd())
    {
      QString readline = stream.readLine();
      if(readline == "99") // Check for end of file marker
        break;

      QStringList line = readline.simplified().split(" ");

      if(line.size() >= 9) // Reg and type might be omitted
      {
        // 2 190620    FHAW    FHAW   0   0.1   0.0   0.0   0.0  N7779E  Car_B1900D
        if(line.at(PREFIX) != "2")
          qWarning() << Q_FUNC_INFO << "Unknown prefix" << line.at(PREFIX) << "at line" << lineNum;

        // Time ========================
        int travelTimeSecs = atools::roundToInt(atFloat(line, TIME, true) * 3600.f);
        QDateTime departureTime = QDateTime::fromString("20" % at(line, DATE), "yyyyMMdd");
        QDateTime destinationTime = departureTime.addSecs(travelTimeSecs);

        // Resolve departure and destination ================================
        QString departure = at(line, DEPARTURE), departureName;
        atools::geo::Pos departurePos;

        // Get name and coordinates from database
        fetchAirport(departurePos, departureName, departure);

        insertQuery.bindValue(idBinding, id++);

        // Departure =====================================================
        insertQuery.bindValue(":departure_ident", departure);
        insertQuery.bindValue(":departure_name", departureName);

        if(departurePos.isValid())
        {
          // Leave position null, otherwise
          insertQuery.bindValue(":departure_lonx", departurePos.getLonX());
          insertQuery.bindValue(":departure_laty", departurePos.getLatY());
          insertQuery.bindValue(":departure_alt", departurePos.getAltitude());
        }

        insertQuery.bindValue(":departure_time_sim", departureTime);
        insertQuery.bindValue(":departure_time", departureTime);

        // Destination =====================================================
        // Get name and coordinates from database
        QString destination = at(line, DESTINATION), destinationName;
        atools::geo::Pos destinationPos;
        fetchAirport(destinationPos, destinationName, destination);

        insertQuery.bindValue(":destination_ident", destination);
        insertQuery.bindValue(":destination_name", destinationName);

        if(destinationPos.isValid())
        {
          insertQuery.bindValue(":destination_lonx", destinationPos.getLonX());
          insertQuery.bindValue(":destination_laty", destinationPos.getLatY());
          insertQuery.bindValue(":destination_alt", destinationPos.getAltitude());
        }
        insertQuery.bindValue(":destination_time_sim", destinationTime);
        insertQuery.bindValue(":destination_time", destinationTime);

        // Aircraft ====================================================
        if(TAIL_NUMBER < line.size())
        {
          QString tailNum = at(line, TAIL_NUMBER);
          insertQuery.bindValue(":aircraft_registration", tailNum.replace("_", " "));
        }

        if(AIRCRAFT_TYPE < line.size())
        {
          QString aircraftType = at(line, AIRCRAFT_TYPE);
          insertQuery.bindValue(":aircraft_type", aircraftType.replace("_", " "));
        }

        // ===================================================================
        if(departurePos.isValid() && destinationPos.isValid())
          insertQuery.bindValue(":distance", atools::geo::meterToNm(departurePos.distanceMeterTo(destinationPos)));

        insertQuery.bindValue(":simulator", "X-Plane 11");

        // Description ===================================================================
        /*: The text "Imported from X-Plane logbook" has to match the one in LogdataController::importXplane */
        QString description(tr("Imported from X-Plane logbook %1\n"
                               "Number of landings: %2\n"
                               "Cross country time: %3\n"
                               "IFR time: %4\n"
                               "Night time: %5").
                            arg(filename).
                            arg(atInt(line, NUM_LANDINGS, true)).
                            arg(atFloat(line, TIME_CROSS_COUNTRY, true), 0, 'f', 1).
                            arg(atFloat(line, TIME_IFR, true), 0, 'f', 1).
                            arg(atFloat(line, TIME_NIGHT, true), 0, 'f', 1));
        insertQuery.bindValue(":description", description);

        // Fill null fields with empty strings to avoid issues when searching
        // Also turn empty BLOBs to NULL
        fixEmptyFields(insertQuery);

        insertQuery.exec();
        undoHandler.inserted();

        insertQuery.clearBoundValues();
      } // if(line.size() >= 10)

      lineNum++;
    } // while(!stream.atEnd())

    file.close();
    undoHandler.finish();
  } // if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));

  return numImported;

}

int LogdataManager::exportCsv(const QString& filepath, const QVector<int>& ids, bool exportPlan, bool exportPerf, bool exportGpx,
                              bool header, bool append)
{
  bool endsWithEol = atools::fileEndsWithEol(filepath);
  int numExported = 0;
  QFile file(filepath);
  if(file.open((append ? QIODevice::Append : QIODevice::WriteOnly) | QIODevice::Text))
  {
    // Build a list of columns in fixed order as defined in the enum to
    // avoid issues with different column order in table
    QStringList columns;
    for(int i = csv::FIRST_COL; i <= csv::LAST_COL; i++)
    {
      const SqlColumn col = csv::COL_MAP.value(i);
      if(col.getName() != idColumnName)
        columns.append(col.getSelectStmt());
    }

    // Use query wrapper to automatically use passed ids or all rows
    SqlUtil util(db);
    QueryWrapper query(util.buildSelectStatement(tableName, columns), db, ids, idColumnName);

    QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    stream.setCodec("UTF-8");
#endif
    stream.setRealNumberNotation(QTextStream::FixedNotation);

    if(!endsWithEol && append)
      // Add needed linefeed for append
      stream << endl;

    SqlExport sqlExport;
    sqlExport.setEndline(false);
    sqlExport.setHeader(header);
    sqlExport.setNumberPrecision(5);

    // Add callbacks to converting Gzipped BLOBs to strings
    // Convert to empty string if export should be skipped
    sqlExport.addConversionFunc(exportPlan ? blobConversionFunction : blobConversionFunctionEmpty,
                                csv::COL_MAP.value(csv::FLIGHTPLAN).getDisplayName());
    sqlExport.addConversionFunc(exportPerf ? blobConversionFunction : blobConversionFunctionEmpty,
                                csv::COL_MAP.value(csv::AIRCRAFT_PERF).getDisplayName());
    sqlExport.addConversionFunc(exportGpx ? blobConversionFunction : blobConversionFunctionEmpty,
                                csv::COL_MAP.value(csv::AIRCRAFT_TRAIL).getDisplayName());

    bool first = true;
    query.exec();
    while(query.next())
    {
      if(first && header)
      {
        // Write header
        first = false;
        stream << sqlExport.getResultSetHeader(query.query.record()) << endl;
      }
      SqlRecord record = query.query.record();

      // Write row
      stream << sqlExport.getResultSetRow(record) << endl;
      numExported++;
    }

    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
  return numExported;
}

QString LogdataManager::blobConversionFunctionEmpty(const QVariant&)
{
  return QString();
}

QString LogdataManager::blobConversionFunction(const QVariant& value)
{
  if(value.isValid() && !value.isNull() && value.type() == QVariant::ByteArray)
    return QString(atools::zip::gzipDecompress(value.toByteArray()));

  return QString();
}

void LogdataManager::updateSchema()
{
  addColumnIf("route_string", "varchar(1024)");
  addColumnIf("flightplan", "blob");
  addColumnIf("aircraft_perf", "blob");
  addColumnIf("aircraft_trail", "blob");

  repairDateTime("departure_time");
  repairDateTime("destination_time");

  DataManagerBase::updateUndoSchema();
}

void LogdataManager::repairDateTime(const QString& column)
{
  int offsetMin = QDateTime::currentDateTime().offsetFromUtc() / 60;
  int offsetH = offsetMin / 60;
  int offsetM = offsetMin % 60;

  QString offsetStr = QString("%1%2:%3").arg(offsetMin < 0 ? "-" : "+").arg(offsetH, 2, 10, QChar('0')).arg(offsetM, 2, 10, QChar('0'));

  SqlTransaction transaction(db);

  int num = 0;
  SqlQuery update(db);
  update.prepare("update logbook set " % column % " = :datetime where logbook_id = :id");

  // WRONG: 2023-02-08T22:01:31.360
  // REPLACEMENT: 2023-02-07T19:44:31.764-08:00
  SqlQuery query("select logbook_id, " % column % "  from logbook where " % column % " like '____-__-__T__:__:__.___'", db);
  query.exec();
  while(query.next())
  {
    update.bindValue(":id", query.value(0));
    update.bindValue(":datetime", QString(query.value(1).toString().trimmed() % offsetStr));
    update.exec();
    num++;
  }

  if(num > 0)
    qDebug() << Q_FUNC_INFO << "Updated" << num << "rows with new date in logbook." << column << "using timezone" << offsetStr;
  transaction.commit();
}

void LogdataManager::clearGeometryCache()
{
  cache.clear();
}

void LogdataManager::preCleanup()
{
  sql::DataManagerBase::preCleanup(CLEANUP_COLUMNS);
}

void LogdataManager::postCleanup()
{
  sql::DataManagerBase::postCleanup(CLEANUP_COLUMNS);
}

QString LogdataManager::getCleanupPreview(bool departureAndDestEqual, bool departureOrDestEmpty, float minFlownDistance,
                                          const QVector<SqlColumn>& columns)
{
  return "select " % SqlColumn::getColumnList(columns) % " from " % tableName % " where " %
         cleanupWhere(departureAndDestEqual, departureOrDestEmpty, minFlownDistance);
}

int LogdataManager::cleanupLogEntries(bool departureAndDestEqual, bool departureOrDestEmpty, float minFlownDistance)
{
  // Avoid long running queries
  db->analyze();

  // Fetch ids and delete
  QSet<int> ids;
  SqlUtil(getDatabase()).getIds(ids, tableName, idColumnName, cleanupWhere(departureAndDestEqual, departureOrDestEmpty, minFlownDistance));

  postCleanup();

  // Also takes care of undo/redo
  deleteRows(ids);
  db->analyze();

  return ids.size();
}

QString LogdataManager::cleanupWhere(bool departureAndDestEqual, bool departureOrDestEmpty, float minFlownDistance)
{
  QStringList queryWhere;
  if(departureAndDestEqual)
    queryWhere.append("(departure_ident <> '' and destination_ident <> '' and departure_ident = destination_ident)");

  if(departureOrDestEmpty)
    queryWhere.append("(departure_ident = '' or destination_ident = '' or " // Empty
                      "departure_ident glob '[0-9][0-9][0-9][0-9][NS][0-9]*[0-9][EW]' or " // Off-airport
                      "destination_ident glob '[0-9][0-9][0-9][0-9][NS][0-9]*[0-9][EW]')");

  if(minFlownDistance >= 0.f)
    queryWhere.append(QString("(distance_flown <= %1)").arg(minFlownDistance));

#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << queryWhere;
#endif

  return queryWhere.join(" or ");
}

bool LogdataManager::hasRouteAttached(int id)
{
  return hasBlob(id, "flightplan");
}

bool LogdataManager::hasPerfAttached(int id)
{
  return hasBlob(id, "aircraft_perf");
}

bool LogdataManager::hasTrackAttached(int id)
{
  return hasBlob(id, "aircraft_trail");
}

const gpx::GpxData *LogdataManager::getGpxData(int id)
{
  loadGpx(id);
  return cache.object(id);
}

void LogdataManager::loadGpx(int id)
{
  if(!cache.contains(id))
  {
    gpx::GpxData *entry = new gpx::GpxData;
    atools::fs::gpx::GpxIO().loadGpxGz(*entry, getValue(id, "aircraft_trail").toByteArray());
    cache.insert(id, entry);
  }
}

void LogdataManager::getFlightStatsTime(QDateTime& earliest, QDateTime& latest, QDateTime& earliestSim,
                                        QDateTime& latestSim)
{
  SqlQuery query("select min(departure_time), max(departure_time), "
                 "min(departure_time_sim), max(departure_time_sim) from " % tableName, db);
  query.exec();
  if(query.next())
  {
    earliest = query.valueDateTime(0);
    latest = query.valueDateTime(1);
    earliestSim = query.valueDateTime(2);
    latestSim = query.valueDateTime(3);
  }
}

void LogdataManager::getFlightStatsDistance(float& distTotal, float& distMax, float& distAverage)
{
  SqlQuery query("select sum(distance), max(distance), avg(distance) from " % tableName, db);
  query.exec();
  if(query.next())
  {
    distTotal = query.valueFloat(0);
    distMax = query.valueFloat(1);
    distAverage = query.valueFloat(2);
  }
}

void LogdataManager::getFlightStatsAirports(int& numDepartAirports, int& numDestAirports)
{
  SqlQuery query("select count(distinct departure_ident), count(distinct destination_ident) from " % tableName, db);
  query.exec();
  if(query.next())
  {
    numDepartAirports = query.valueInt(0);
    numDestAirports = query.valueInt(1);
  }
}

void LogdataManager::getFlightStatsAircraft(int& numTypes, int& numRegistrations, int& numNames, int& numSimulators)
{
  SqlQuery query("select count(distinct aircraft_type), count(distinct aircraft_registration), "
                 "count(distinct aircraft_name), count(distinct simulator) "
                 "from " % tableName, db);
  query.exec();
  if(query.next())
  {
    numTypes = query.valueInt(0);
    numRegistrations = query.valueInt(1);
    numNames = query.valueInt(2);
    numSimulators = query.valueInt(3);
  }
}

void LogdataManager::getFlightStatsSimulator(QVector<std::pair<int, QString> >& numSimulators)
{
  SqlQuery query("select count(1), simulator from " % tableName % " group by simulator order by count(1) desc", db);
  query.exec();
  while(query.next())
    numSimulators.append(std::make_pair(query.valueInt(0), query.valueStr(1)));
}

void LogdataManager::fixEmptyStrField(sql::SqlRecord& rec, const QString& name)
{
  if(rec.contains(name) && rec.isNull(name))
    rec.setValue(name, "");
}

void LogdataManager::fixEmptyStrField(sql::SqlQuery& query, const QString& name)
{
  if(query.boundValue(name, true /* ignoreInvalid */).isNull())
    query.bindValue(name, "");
}

void LogdataManager::fixEmptyBlobField(sql::SqlRecord& rec, const QString& name)
{
  if(rec.contains(name) && (rec.value(name).toByteArray().isEmpty()))
    rec.setNull(name);
}

void LogdataManager::fixEmptyBlobField(sql::SqlQuery& query, const QString& name)
{
  if(query.boundValue(name, true /* ignoreInvalid */).toByteArray().isEmpty())
    query.bindValue(name, QVariant(QVariant::ByteArray));
}

void LogdataManager::fixEmptyFields(sql::SqlRecord& rec)
{
  if(rec.contains("distance") && rec.isNull("distance"))
    rec.setValue("distance", 0.f);

  fixEmptyStrField(rec, "aircraft_name");
  fixEmptyStrField(rec, "aircraft_type");
  fixEmptyStrField(rec, "aircraft_registration");
  fixEmptyStrField(rec, "route_string");
  fixEmptyStrField(rec, "description");
  fixEmptyStrField(rec, "simulator");
  fixEmptyStrField(rec, "departure_ident");
  fixEmptyStrField(rec, "destination_ident");

  fixEmptyBlobField(rec, "flightplan");
  fixEmptyBlobField(rec, "aircraft_perf");
  fixEmptyBlobField(rec, "aircraft_trail");
}

void LogdataManager::fixEmptyFields(sql::SqlQuery& query)
{
  if(query.boundValue(":distance", true).isNull())
    query.bindValue(":distance", 0.f);

  fixEmptyStrField(query, ":aircraft_name");
  fixEmptyStrField(query, ":aircraft_type");
  fixEmptyStrField(query, ":aircraft_registration");
  fixEmptyStrField(query, ":route_string");
  fixEmptyStrField(query, ":description");
  fixEmptyStrField(query, ":simulator");
  fixEmptyStrField(query, ":departure_ident");
  fixEmptyStrField(query, ":destination_ident");

  fixEmptyBlobField(query, ":flightplan");
  fixEmptyBlobField(query, ":aircraft_perf");
  fixEmptyBlobField(query, ":aircraft_trail");
}

void LogdataManager::getFlightStatsTripTime(float& timeMaximum, float& timeAverage, float& timeTotal,
                                            float& timeMaximumSim, float& timeAverageSim, float& timeTotalSim)
{
  SqlQuery query(db);

  query.exec("select max(time_real), avg(time_real), sum(time_real) "
             "from (select strftime('%s', destination_time) - strftime('%s', departure_time) as time_real "
             "from " % tableName % ") where time_real > 0");
  if(query.next())
  {
    int idx = 0;
    timeMaximum = query.valueFloat(idx++) / 3600.f;
    timeAverage = query.valueFloat(idx++) / 3600.f;
    timeTotal = query.valueFloat(idx++) / 3600.f;
  }
  query.finish();

  query.exec("select max(time_sim), avg(time_sim), sum(time_sim) "
             "from (select strftime('%s', destination_time_sim) - strftime('%s', departure_time_sim) as time_sim "
             "from " % tableName % ") where time_sim > 0");
  if(query.next())
  {
    int idx = 0;
    timeMaximumSim = query.valueFloat(idx++) / 3600.f;
    timeAverageSim = query.valueFloat(idx++) / 3600.f;
    timeTotalSim = query.valueFloat(idx++) / 3600.f;
  }
  query.finish();
}

} // namespace userdata
} // namespace fs
} // namespace atools
