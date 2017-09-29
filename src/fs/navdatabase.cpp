/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include "fs/navdatabase.h"
#include "sql/sqldatabase.h"
#include "sql/sqlscript.h"
#include "fs/db/datawriter.h"
#include "fs/scenery/sceneryarea.h"
#include "sql/sqlutil.h"
#include "fs/scenery/scenerycfg.h"
#include "fs/db/airwayresolver.h"
#include "fs/db/routeedgewriter.h"
#include "fs/progresshandler.h"
#include "fs/scenery/fileresolver.h"
#include "fs/scenery/addonpackage.h"
#include "fs/scenery/addoncomponent.h"
#include "fs/xp/xpdatacompiler.h"
#include "fs/db/databasemeta.h"

#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QStandardPaths>

namespace atools {
namespace fs {

// Number of progress steps besides scenery areas
const int PROGRESS_NUM_STEPS = 20;
const int PROGRESS_NUM_DB_REPORT_STEPS = 4;
const int PROGRESS_NUM_RESOLVE_AIRWAY_STEPS = 1;
const int PROGRESS_NUM_DEDUPLICATE_STEPS = 1;

using atools::sql::SqlDatabase;
using atools::sql::SqlScript;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::fs::scenery::SceneryCfg;
using atools::fs::scenery::SceneryArea;
using atools::fs::scenery::AddOnComponent;
using atools::fs::scenery::AddOnPackage;

NavDatabase::NavDatabase(const NavDatabaseOptions *readerOptions, sql::SqlDatabase *sqlDb,
                         NavDatabaseErrors *databaseErrors)
  : db(sqlDb), errors(databaseErrors), options(readerOptions)
{

}

void NavDatabase::create(const QString& codec)
{
  createInternal(codec);
  if(aborted)
    // Remove all (partial) changes
    db->rollback();
}

void NavDatabase::createSchema()
{
  createSchemaInternal(nullptr);
}

void NavDatabase::createSchemaInternal(ProgressHandler *progress)
{
  SqlScript script(db, true /* options->isVerbose()*/);

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Views"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/drop_view.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Routing and Search"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/drop_routing_search.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Navigation Aids"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/drop_nav.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Airport Facilites"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/drop_airport_facilities.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Approaches"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/drop_approach.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Airports"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/drop_airport.sql");

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Removing Metadata"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/drop_meta.sql");

  db->commit();

  if(progress != nullptr)
    if((aborted = progress->reportOther(tr("Creating Database Schema"))))
      return;

  script.executeScript(":/atools/resources/sql/fs/db/create_boundary_schema.sql");
  script.executeScript(":/atools/resources/sql/fs/db/create_nav_schema.sql");
  script.executeScript(":/atools/resources/sql/fs/db/create_ap_schema.sql");
  script.executeScript(":/atools/resources/sql/fs/db/create_route_schema.sql");
  script.executeScript(":/atools/resources/sql/fs/db/create_meta_schema.sql");
  script.executeScript(":/atools/resources/sql/fs/db/create_views.sql");
  db->commit();
}

bool NavDatabase::isSceneryConfigValid(const QString& filename, const QString& codec, QString& error)
{
  QFileInfo fi(filename);
  if(fi.exists())
  {
    if(fi.isReadable())
    {
      if(fi.isFile())
      {
        try
        {
          // Read the scenery file and check if it has at least one scenery area
          SceneryCfg cfg(codec);
          cfg.read(filename);

          return !cfg.getAreas().isEmpty();
        }
        catch(atools::Exception& e)
        {
          qWarning() << "Caught exception reading" << filename << ":" << e.what();
          error = e.what();
        }
        catch(...)
        {
          qWarning() << "Caught unknown exception reading" << filename;
          error = "Unknown exception while reading file";
        }
      }
      else
        error = tr("File is not a regular file");
    }
    else
      error = tr("File is not readable");
  }
  else
    error = tr("File does not exist");
  return false;
}

bool NavDatabase::isBasePathValid(const QString& filepath, QString& error, atools::fs::FsPaths::SimulatorType type)
{
  QFileInfo fi(filepath);
  if(fi.exists())
  {
    if(fi.isReadable())
    {
      if(fi.isDir())
      {
        if(type == atools::fs::FsPaths::XPLANE11)
        {
          QFileInfo dataDir(filepath + QDir::separator() + "Resources" + QDir::separator() + "default data");

          if(dataDir.exists() && dataDir.isDir() && dataDir.isReadable())
            return true;
          else
            error = tr("\"%1\" not found").arg(QString("Resources") + QDir::separator() + "default data");
        }
        else
        {
          // If path exists check for scenery directory
          QDir dir(filepath);
          QFileInfoList scenery = dir.entryInfoList({"scenery"}, QDir::Dirs);
          if(!scenery.isEmpty())
            return true;
          else
            error = tr("Does not contain a \"Scenery\" directory");
        }
      }
      else
        error = tr("Is not a directory");
    }
    else
      error = tr("Directory is not readable");
  }
  else
    error = tr("Directory does not exist");
  return false;
}

void NavDatabase::createInternal(const QString& codec)
{
  int numProgressReports = 0, numSceneryAreas = 0, xplaneExtraSteps = 0;
  SceneryCfg cfg(codec);

  QElapsedTimer timer;
  timer.start();

  if(options->isAutocommit())
    db->setAutocommit(true);

  int total = 0, routePartFraction = 0;
  if(options->getSimulatorType() == atools::fs::FsPaths::XPLANE11)
  {
    numProgressReports = atools::fs::xp::XpDataCompiler::calculateReportCount(*options);
    numSceneryAreas = 1; // X-Plane
    xplaneExtraSteps++; // prepare post process airways
    xplaneExtraSteps++; // post process airways

    xplaneExtraSteps--; // ILS id not executed
    xplaneExtraSteps--; // VORTAC merge not executed
    total = numProgressReports + numSceneryAreas + PROGRESS_NUM_STEPS + xplaneExtraSteps;
    // Around 9000 navaids - total / routePartFraction has to be lower than this
    routePartFraction = 20;
  }
  else
  {
    // Read scenery.cfg
    readSceneryConfig(cfg);

    // Count the files for exact progress reporting
    countFiles(cfg, &numProgressReports, &numSceneryAreas);
    total = numProgressReports + numSceneryAreas + PROGRESS_NUM_STEPS + xplaneExtraSteps;
    routePartFraction = 4;
  }

  if(options->isDatabaseReport())
    total += PROGRESS_NUM_DB_REPORT_STEPS;

  if(options->isResolveAirways())
    total += PROGRESS_NUM_RESOLVE_AIRWAY_STEPS;

  if(options->isDeduplicate())
    total += PROGRESS_NUM_DEDUPLICATE_STEPS;

  // Assume this one takes a quarter of the total number of steps
  int numRouteSteps = total / routePartFraction;
  if(options->isCreateRouteTables())
    total += numRouteSteps;

  ProgressHandler progress(options);
  progress.setTotal(total);

  createSchemaInternal(&progress);
  if(aborted)
    return;

  atools::fs::db::DatabaseMeta databaseMetadata(db);
  databaseMetadata.updateAll();

  // -----------------------------------------------------------------------
  // Create data writer which will read all BGL files and fill the database
  QScopedPointer<atools::fs::db::DataWriter> fsDataWriter;
  QScopedPointer<atools::fs::xp::XpDataCompiler> xpDataCompiler;
  SqlScript script(db, true /*options->isVerbose()*/);

  if(options->getSimulatorType() == atools::fs::FsPaths::XPLANE11)
  {
    // Create a single X-Plane scenery area
    atools::fs::scenery::SceneryArea area(1, 1, tr("X-Plane"), QString());

    // Prepare error collection
    if(errors != nullptr)
    {
      NavDatabaseErrors::SceneryErrors err;
      err.scenery = area;
      errors->sceneryErrors.append(err);
    }

    // Load X-Plane scenery database ======================================================
    xpDataCompiler.reset(new atools::fs::xp::XpDataCompiler(*db, *options, &progress, errors));

    if((aborted = progress.reportSceneryArea(&area)))
      return;

    if((aborted = xpDataCompiler->writeBasepathScenery()))
      return;

    if((aborted = xpDataCompiler->compileMagDeclBgl()))
      return;

    if(options->isIncludedNavDbObject(atools::fs::type::AIRPORT))
    {
      // X-Plane 11/Custom Scenery/KSEA Demo Area/Earth nav data/apt.dat
      if((aborted = xpDataCompiler->compileCustomApt())) // Add-on
        return;

      // X-Plane 11/Custom Scenery/Global Airports/Earth nav data/apt.dat
      if((aborted = xpDataCompiler->compileCustomGlobalApt()))
        return;

      // X-Plane 11/Resources/default scenery/default apt dat/Earth nav data/apt.dat
      // Mandatory
      if((aborted = xpDataCompiler->compileDefaultApt()))
        return;
    }

    if(options->isIncludedNavDbObject(atools::fs::type::WAYPOINT))
    {
      // In resources or Custom Data - mandatory
      if((aborted = xpDataCompiler->compileEarthFix()))
        return;

      // Optional user data
      if((aborted = xpDataCompiler->compileUserFix()))
        return;
    }

    if(options->isIncludedNavDbObject(atools::fs::type::VOR) ||
       options->isIncludedNavDbObject(atools::fs::type::NDB) ||
       options->isIncludedNavDbObject(atools::fs::type::MARKER) ||
       options->isIncludedNavDbObject(atools::fs::type::ILS))
    {
      // In resources or Custom Data - mandatory
      if((aborted = xpDataCompiler->compileEarthNav()))
        return;

      // Optional user data
      if((aborted = xpDataCompiler->compileUserNav()))
        return;
    }

    if(options->isIncludedNavDbObject(atools::fs::type::AIRWAY))
    {
      // In resources or Custom Data - mandatory
      if((aborted = xpDataCompiler->compileEarthAirway()))
        return;

      if((aborted = runScript(&progress, "fs/db/xplane/prepare_airway.sql", tr("Preparing Airways"))))
        return;

      if((aborted = xpDataCompiler->postProcessEarthAirway()))
        return;
    }

    if(options->isIncludedNavDbObject(atools::fs::type::APPROACH))
    {
      if((aborted = xpDataCompiler->compileCifp()))
        return;
    }

    xpDataCompiler->close();

    // Remove scenery from error list if nothing happened
    if(errors != nullptr && !errors->sceneryErrors.isEmpty())
    {
      const NavDatabaseErrors::SceneryErrors& err = errors->sceneryErrors.first();

      if(err.fileErrors.isEmpty() && err.sceneryErrorsMessages.isEmpty())
        errors->sceneryErrors.clear();
    }
  }
  else
  {
    // Load FSX / P3D scenery database ======================================================
    fsDataWriter.reset(new atools::fs::db::DataWriter(*db, *options, &progress));

    fsDataWriter->readMagDeclBgl();

    for(const atools::fs::scenery::SceneryArea& area : cfg.getAreas())
    {
      if((area.isActive() || options->isReadInactive()) &&
         options->isIncludedLocalPath(area.getLocalPath()))
      {
        if((aborted = progress.reportSceneryArea(&area)))
          return;

        NavDatabaseErrors::SceneryErrors err;
        if(errors != nullptr)
          // Prepare structure for error collection
          fsDataWriter->setSceneryErrors(&err);
        else
          fsDataWriter->setSceneryErrors(nullptr);

        // Read all BGL files in the scenery area into classes of the bgl namespace and
        // write the contents to the database
        fsDataWriter->writeSceneryArea(area);

        if((!err.fileErrors.isEmpty() || !err.sceneryErrorsMessages.isEmpty()) && errors != nullptr)
        {
          err.scenery = area;
          errors->sceneryErrors.append(err);
        }

        if((aborted = fsDataWriter->isAborted()))
          return;
      }
    }
    db->commit();
    fsDataWriter->close();
  }

  // ===========================================================================
  // Loading is done here - now continue with the post process steps

  if((aborted = runScript(&progress, "fs/db/create_indexes_post_load.sql", tr("Creating indexes"))))
    return;

  if(options->isDeduplicate())
  {
    // Delete duplicates before any foreign keys ids are assigned
    if((aborted = runScript(&progress, "fs/db/delete_duplicates.sql", tr("Clean up"))))
      return;
  }

  if(options->isResolveAirways())
  {
    if((aborted = progress.reportOther(tr("Creating airways"))))
      return;

    // Read airway_point table, connect all waypoints and write the ordered result into the airway table
    atools::fs::db::AirwayResolver resolver(db, progress);

    if((aborted = resolver.run()))
      return;
  }

  if(options->getSimulatorType() != atools::fs::FsPaths::XPLANE11)
  {
    // Create VORTACs
    if((aborted = runScript(&progress, "fs/db/update_vor.sql", tr("Merging VOR and TACAN to VORTAC"))))
      return;
  }

  // Set the nav_ids (VOR, NDB) in the waypoint table and update the airway counts
  if((aborted = runScript(&progress, "fs/db/update_wp_ids.sql", tr("Updating waypoints"))))
    return;

  // Set the nav_ids (VOR, NDB) in the approach table
  if((aborted = runScript(&progress, "fs/db/update_approaches.sql", tr("Updating approaches"))))
    return;

  if(options->getSimulatorType() != atools::fs::FsPaths::XPLANE11)
  {
    // The ids are already updated when reading the X-Plane data
    // Set runway end ids into the ILS
    if((aborted = runScript(&progress, "fs/db/update_ils_ids.sql", tr("Updating ILS"))))
      return;
  }

  // update the ILS count in the airport table
  if((aborted = runScript(&progress, "fs/db/update_num_ils.sql", tr("Updating ILS Count"))))
    return;

  // Prepare the search table
  if((aborted = runScript(&progress, "fs/db/populate_nav_search.sql", tr("Collecting navaids for search"))))
    return;

  // Fill tables for automatic flight plan calculation
  if((aborted = runScript(&progress, "fs/db/populate_route_node.sql", tr("Populating routing tables"))))
    return;

  if(options->isCreateRouteTables())
  {
    if((aborted = progress.reportOther(tr("Creating route edges for VOR and NDB"))))
      return;

    // Create a network of VOR and NDB stations that allow radio navaid routing
    atools::fs::db::RouteEdgeWriter edgeWriter(db, progress, numRouteSteps);
    if((aborted = edgeWriter.run()))
      return;
  }

  if((aborted = runScript(&progress, "fs/db/populate_route_edge.sql", tr("Creating route edges waypoints"))))
    return;

  if((aborted = runScript(&progress, "fs/db/finish_schema.sql", tr("Creating indexes for search"))))
    return;

  databaseMetadata.setAiracCycle(xpDataCompiler->getAiracCycle());
  databaseMetadata.updateAll();

  // Done here - now only some options statistics and reports are left

  if(options->isDatabaseReport())
  {
    // Do a report of problems rather than failing totally during loading
    if(fsDataWriter != nullptr)
      fsDataWriter->logResults();

    QDebug info(QtInfoMsg);
    atools::sql::SqlUtil util(db);

    if((aborted = progress.reportOther(tr("Creating table statistics"))))
      return;

    qDebug() << "printTableStats";
    info << endl;
    util.printTableStats(info);

    if((aborted = progress.reportOther(tr("Creating report on values"))))
      return;

    qDebug() << "createColumnReport";
    info << endl;
    util.createColumnReport(info);

    if((aborted = progress.reportOther(tr("Creating report on duplicates"))))
      return;

    info << endl;
    qDebug() << "reportDuplicates airport";
    util.reportDuplicates(info, "airport", "airport_id", {"ident"});
    info << endl;
    qDebug() << "reportDuplicates vor";
    util.reportDuplicates(info, "vor", "vor_id", {"ident", "region", "lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates ndb";
    util.reportDuplicates(info, "ndb", "ndb_id", {"ident", "type", "frequency", "region", "lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates waypoint";
    util.reportDuplicates(info, "waypoint", "waypoint_id", {"ident", "type", "region", "lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates ils";
    util.reportDuplicates(info, "ils", "ils_id", {"ident", "lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates marker";
    util.reportDuplicates(info, "marker", "marker_id", {"type", "heading", "lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates helipad";
    util.reportDuplicates(info, "helipad", "helipad_id", {"lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates parking";
    util.reportDuplicates(info, "parking", "parking_id", {"lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates start";
    util.reportDuplicates(info, "start", "start_id", {"lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates runway";
    util.reportDuplicates(info, "runway", "runway_id", {"heading", "lonx", "laty"});
    info << endl;
    qDebug() << "reportDuplicates bgl_file";
    util.reportDuplicates(info, "bgl_file", "bgl_file_id", {"filename"});
    info << endl;

    if((aborted = progress.reportOther(tr("Creating report on coordinate duplicates"))))
      return;

    reportCoordinateViolations(info, util, {"airport", "vor", "ndb", "marker", "waypoint"});
  }

  // Send the final progress report
  progress.reportFinish();

  qDebug() << "Time" << timer.elapsed() / 1000 << "seconds";
}

bool NavDatabase::runScript(ProgressHandler *progress, const QString& scriptFile, const QString& message)
{
  SqlScript script(db, true /*options->isVerbose()*/);

  if((aborted = progress->reportOther(message)))
    return true;

  script.executeScript(":/atools/resources/sql/" + scriptFile);
  db->commit();
  return false;
}

void NavDatabase::readSceneryConfig(atools::fs::scenery::SceneryCfg& cfg)
{
  // Get entries from scenery.cfg file
  cfg.read(options->getSceneryFile());

  FsPaths::SimulatorType sim = options->getSimulatorType();

  if(options->isReadAddOnXml() && (sim == atools::fs::FsPaths::P3D_V3 || sim == atools::fs::FsPaths::P3D_V4))
  {
    // Read the Prepar3D add on packages and add them to the scenery list ===============================
    QString documents(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first());

    int simNum = sim == atools::fs::FsPaths::P3D_V3 ? 3 : 4;

    // Add both path alternatives since documentation is not clear
    QStringList addonPaths;
    // Mentioned in the SDK on "Add-on Packages" -> "Distributing an Add-on Package"
    addonPaths.append(documents + QDir::separator() + QString("Prepar3D v%1 Add-ons").arg(simNum));

    // Mentioned in the SDK on "Add-on Instructions for Developers" -> "Add-on Directory Structure"
    addonPaths.append(documents + QDir::separator() + QString("Prepar3D v%1 Files").arg(simNum) +
                      QDir::separator() + QLatin1Literal("add-ons"));

    // Calculate maximum area number
    int areaNum = std::numeric_limits<int>::min();
    for(const SceneryArea& area : cfg.getAreas())
      areaNum = std::max(areaNum, area.getAreaNumber());

    QVector<AddOnComponent> noLayerComponents;
    QStringList noLayerPaths;

    for(const QString& addonPath : addonPaths)
    {
      QDir addonDir(addonPath);
      if(addonDir.exists())
      {
        // Read addon directories as they appear in the file system
        for(QFileInfo addonEntry : addonDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
        {
          QFileInfo addonFile(addonEntry.absoluteFilePath() + QDir::separator() + QLatin1Literal("add-on.xml"));
          if(addonFile.exists() && addonFile.isFile())
          {
            qInfo() << "Found addon file" << addonFile.filePath();

            AddOnPackage package(addonFile.filePath());
            qInfo() << "Name" << package.getName() << "Description" << package.getDescription();

            for(const AddOnComponent& component : package.getComponents())
            {
              qInfo() << "Component" << component.getLayer()
                      << "Name" << component.getName()
                      << "Description" << component.getPath();

              QDir compPath(component.getPath());

              if(compPath.isRelative())
                // Convert relative path to absolute based on add-on file directory
                compPath = package.getBaseDirectory() + QDir::separator() + compPath.path();

              if(compPath.dirName().toLower() == "scenery")
                // Remove if it points to scenery directory
                compPath.cdUp();

              compPath.makeAbsolute();

              areaNum++;

              if(!compPath.exists())
                qWarning() << "Path does not exist" << compPath;

              if(component.getLayer() == -1)
              {
                // Add entries without layers later at the end of the list
                // Layer is only used if add-on does not provide a layer
                noLayerComponents.append(component);
                noLayerPaths.append(compPath.path());
              }
              else
                cfg.appendArea(SceneryArea(areaNum, component.getLayer(), component.getName(), compPath.path()));
            }
          }
          else
            qWarning() << Q_FUNC_INFO << addonFile.filePath() << "does not exist or is not a directory";
        }
      }
      else
        qWarning() << Q_FUNC_INFO << addonDir << "does not exist";
    }

    // Bring added add-on.xml in order with the rest sort by layer
    cfg.sortAreas();

    // Calculate maximum layer and area number
    int lastLayer = std::numeric_limits<int>::min();
    int lastArea = std::numeric_limits<int>::min();
    for(const SceneryArea& area : cfg.getAreas())
    {
      lastArea = std::max(lastArea, area.getAreaNumber());
      lastLayer = std::max(lastLayer, area.getLayer());
    }

    for(int i = 0; i < noLayerComponents.size(); i++)
      cfg.appendArea(SceneryArea(++lastArea, ++lastLayer, noLayerComponents.at(i).getName(), noLayerPaths.at(i)));
  }
}

void NavDatabase::reportCoordinateViolations(QDebug& out, atools::sql::SqlUtil& util,
                                             const QStringList& tables)
{
  for(QString table : tables)
  {
    qDebug() << "reportCoordinateViolations" << table;
    util.reportRangeViolations(out, table, {table + "_id", "ident"}, "lonx", -180.f, 180.f);
    util.reportRangeViolations(out, table, {table + "_id", "ident"}, "laty", -90.f, 90.f);
  }
}

void NavDatabase::countFiles(const atools::fs::scenery::SceneryCfg& cfg, int *numFiles, int *numSceneryAreas)
{
  qDebug() << "Counting files";

  for(const atools::fs::scenery::SceneryArea& area : cfg.getAreas())
  {
    if(area.isActive() && options->isIncludedLocalPath(area.getLocalPath()))
    {
      atools::fs::scenery::FileResolver resolver(*options, true);

      *numFiles += resolver.getFiles(area);
      (*numSceneryAreas)++;
    }
  }
  qDebug() << "Counting files done." << *numFiles << "files to process";
}

} // namespace fs
} // namespace atools
