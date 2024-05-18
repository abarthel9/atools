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

#include "fs/bgl/bglfile.h"

#include "io/binarystream.h"
#include "fs/bgl/section.h"
#include "fs/bgl/subsection.h"
#include "fs/bgl/header.h"
#include "fs/bgl/ap/airport.h"
#include "fs/bgl/nl/namelist.h"
#include "fs/bgl/nav/ilsvor.h"
#include "fs/bgl/nav/vor.h"
#include "fs/bgl/nav/tacan.h"
#include "fs/bgl/nav/ils.h"
#include "fs/bgl/nav/marker.h"
#include "fs/bgl/nav/ndb.h"
#include "fs/bgl/nav/waypoint.h"
#include "fs/bgl/boundary.h"
#include "fs/bgl/recordtypes.h"
#include "fs/scenery/sceneryarea.h"

#include <QList>
#include <QDebug>

#include <QFile>
#include <QFileInfo>

namespace atools {
namespace fs {
namespace bgl {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::hex;
using Qt::dec;
#endif

using atools::io::BinaryStream;

BglFile::BglFile(const NavDatabaseOptions *readerOptions)
  : size(0), options(readerOptions)
{
}

BglFile::~BglFile()
{
  deleteAllObjects();
}

void BglFile::readFile(const QString& filenameParam, const atools::fs::scenery::SceneryArea& area)
{
  deleteAllObjects();
  filename = filenameParam;

  QFile file(filename);

  if(file.size() < Header::HEADER_SIZE)
  {
    qWarning() << "File is too small:" << file.size();
    return;
  }

  if(file.open(QIODevice::ReadOnly))
  {
    BinaryStream stream(&file);

    size = stream.getFileSize();

    readHeader(&stream);
    if(!header.isValid())
      // Skip any obscure BGL files that do not contain a section structure or are too small
      return;

    readSections(&stream);

    if(options->isIncludedNavDbObject(type::BOUNDARY) && !area.isMsfsNavigraphNavdata())
      readBoundaryRecords(&stream);

    readRecords(&stream, area);

    file.close();
  }
}

bool BglFile::isValid()
{
  return header.isValid();
}

bool BglFile::hasContent()
{
  return !(airports.isEmpty() &&
           namelists.isEmpty() &&
           vors.isEmpty() &&
           tacans.isEmpty() &&
           ils.isEmpty() &&
           ndbs.isEmpty() &&
           marker.isEmpty() &&
           waypoints.isEmpty() &&
           boundaries.isEmpty());
}

void BglFile::readBoundaryRecords(BinaryStream *bs)
{
  // Scan all sections for boundaries
  for(Section& section : sections)
  {
    if(section.getType() == atools::fs::bgl::section::BOUNDARY)
    {
      // CTR, TMA and CTA: BNXWorld0.bgl
      // Prohibited, Dangerous, Restricted and MOA: BNXWorld1.bgl
      // Coastlines: BNXWorld2.bgl BNXWorld3. bgl BNXWorld4.bgl BNXWorld5.bgl
      // Remaining boundary files are BNXWorld1.bgl BNXWorld2.bgl BNXWorld3.bgl BNXWorld4.bgl BNXWorld5.bgl
      bs->seekg(section.getFirstSubsectionOffset());

      // Get the lowest offset from the special subsection offset list
      unsigned int minOffset = std::numeric_limits<unsigned int>::max();
      while(bs->tellg() < section.getStartOffset() + section.getTotalSubsectionSize())
      {
        unsigned int offset1 = bs->readUInt();
        bs->readUInt();
        unsigned int offset2 = bs->readUInt();
        unsigned int treeFlag = bs->readUInt();

        if(treeFlag > 0)
        {
          if(offset1 < minOffset)
            minOffset = offset1;
          if(offset2 < minOffset)
            minOffset = offset2;
        }
      }

      // Read from the first offset to the end of the file
      bs->seekg(minOffset);
      handleBoundaries(bs);
    }
  }
}

void BglFile::handleBoundaries(BinaryStream *bs)
{
  // Read records until end of the file
  int numRecs = 0;
  while(bs->tellg() < bs->getFileSize())
  {
    Record rec(options, bs);
    rec::RecordType type = rec.getId<rec::RecordType>();

    if(type == rec::BOUNDARY)
    {
      rec.seekToStart();
      const Record *r = createRecord<Boundary>(bs, &boundaries);
      if(r != nullptr)
        numRecs++;
    }
    else if(type != rec::GEOPOL)
      // Should only contain boundaries and geopol records
      qWarning().nospace() << "while reading boundaries: unexpected record "
                           << hex << "0x" << type << dec << " offset " << bs->tellg();

    rec.seekToEnd();
  }
  if(options->isVerbose())
    qDebug() << "Num boundary records" << numRecs;
}

void BglFile::readHeader(BinaryStream *bs)
{
  header = Header(options, bs);
  if(options->isVerbose())
    qDebug() << header;
}

void BglFile::readSections(BinaryStream *bs)
{
  // Read sections after header
  for(unsigned int i = 0; i < header.getNumSections(); i++)
  {
    Section s = Section(options, bs);

    // Add only supported sections to the list
    if(supportedSectionTypes.isEmpty() || supportedSectionTypes.contains(s.getType()))
    {
      if(options->isVerbose())
        qDebug() << "Section" << s;
      sections.append(s);
    }
    else if(options->isVerbose())
      qDebug() << "Unsupported section" << s;

  }

  // Read subsections for each section
  for(Section& section : sections)
  {
    // Ignore boundary and geopol since these are different
    if(section.getType() != atools::fs::bgl::section::BOUNDARY &&
       section.getType() != atools::fs::bgl::section::GEOPOL)
    {
      bs->seekg(section.getFirstSubsectionOffset());
      for(unsigned int i = 0; i < section.getNumSubsections(); i++)
      {
        Subsection s(options, bs, section);
        if(options->isVerbose())
          qDebug() << s;
        subsections.append(s);
      }
    }
  }
}

const Record *BglFile::handleIlsVor(BinaryStream *bs)
{
  // Read only type before creating concrete object
  IlsVor iv(options, bs);
  iv.seekToStart();

  switch(iv.getType())
  {
    case nav::TERMINAL:
    case nav::LOW:
    case nav::HIGH:
    case nav::VOT:
      if(options->isIncludedNavDbObject(type::VOR))
        return createRecord<Vor>(bs, &vors);

      break;

    case nav::ILS:
      if(options->isIncludedNavDbObject(type::ILS))
        return createRecord<Ils>(bs, &ils);

      break;

    default:
      if(options->getSimulatorType() != atools::fs::FsPaths::MSFS)
        qWarning() << "Unknown ILS/VOR type" << iv.getType();
  }
  return nullptr;
}

void BglFile::readRecords(BinaryStream *bs, const atools::fs::scenery::SceneryArea& area)
{
  bgl::flags::CreateFlags createFlags = bgl::flags::NONE;

  // Set flag if MSFS scenery area is only navdata and dummy airports
  bool msfsNavigraphNavdata = area.isMsfsNavigraphNavdata();
  createFlags.setFlag(bgl::flags::AIRPORT_MSFS_NAVIGRAPH_NAVDATA, msfsNavigraphNavdata);
  createFlags.setFlag(bgl::flags::AIRPORT_MSFS_DUMMY, area.isNavdata());

  // There should be no duplicate airport idents in the file. Otherwise bail out of reading this file.
  QSet<QString> airportIdents;

  for(Subsection& subsection : subsections)
  {
    section::SectionType type = subsection.getParent().getType();

    if(options->isVerbose())
    {
      qDebug() << "=======================";
      qDebug().nospace().noquote() << "Records of 0x" << hex << subsection.getFirstDataRecordOffset() << dec
                                   << " type " << sectionTypeStr(type);
    }

    bs->seekg(subsection.getFirstDataRecordOffset());

    int numRec = subsection.getNumDataRecords();

    if(type == section::NAME_LIST)
      // Name lists have only one record
      numRec = 1;

    for(int i = 0; i < numRec; i++)
    {
      const Record *rec = nullptr;

      switch(type)
      {
        case section::AIRPORT:
          if(options->isIncludedNavDbObject(type::AIRPORT))
          {
            // Will return null if ICAO is excluded in configuration
            // Read airport and all subrecords, like runways, com, approaches, waypoints and so on
            const Airport *ap = createRecord<Airport>(bs, &airports, createFlags);

            if(ap == nullptr)
              qWarning().nospace().noquote() << "Read airport is null at offset " << bs->tellg();
            else
            {
              if(airportIdents.contains(ap->getIdent()))
              {
                qWarning().nospace().noquote() << "Duplicate airport ident " << ap->getIdent() << " at offset " << bs->tellg();

                // Duplicates found. Bail out =================================================================
                // Example of malformed file UWLS.bgl
                return;
              }
              else
                airportIdents.insert(ap->getIdent());
            }

            rec = ap;
          }
          break;

        case section::AIRPORT_ALT:
          qWarning() << "Found alternate airport ID";
          if(options->isIncludedNavDbObject(type::AIRPORT))
            rec = createRecord<Airport>(bs, &airports, bgl::flags::NONE);
          break;

        case section::NAME_LIST:
          rec = createRecord<Namelist>(bs, &namelists);
          break;

        case section::P3D_TACAN:
          rec = createRecord<Tacan>(bs, &tacans);
          break;

        case section::ILS_VOR:
          if(!msfsNavigraphNavdata)
            rec = handleIlsVor(bs);
          break;

        case section::NDB:
          if(options->isIncludedNavDbObject(type::NDB) && !msfsNavigraphNavdata)
            rec = createRecord<Ndb>(bs, &ndbs);
          break;

        case section::MARKER:
          if(options->isIncludedNavDbObject(type::MARKER) && !msfsNavigraphNavdata)
            rec = createRecord<Marker>(bs, &marker);
          break;

        case section::WAYPOINT:
          if(options->isIncludedNavDbObject(type::WAYPOINT) && !msfsNavigraphNavdata)
            // Read waypoints and airways
            rec = createRecord<Waypoint>(bs, &waypoints);
          break;

        // MSFS sections not found yet
        case section::MSFS_DELETE_AIRPORT_NAV:
        case section::MSFS_DELETE_NAV:

        // Other sections that are not of interest here
        case section::BOUNDARY:
        case section::GEOPOL:
        case section::NONE:
        case section::COPYRIGHT:
        case section::GUID:
        case section::SCENERY_OBJECT:
        case section::VOR_ILS_ICAO_INDEX:
        case section::NDB_ICAO_INDEX:
        case section::WAYPOINT_ICAO_INDEX:
        case section::MODEL_DATA:
        case section::AIRPORT_SUMMARY:
        case section::EXCLUSION:
        case section::TIMEZONE:
        case section::TERRAIN_VECTOR_DB:
        case section::TERRAIN_ELEVATION:
        case section::TERRAIN_LAND_CLASS:
        case section::TERRAIN_WATER_CLASS:
        case section::TERRAIN_REGION:
        case section::POPULATION_DENSITY:
        case section::AUTOGEN_ANNOTATION:
        case section::TERRAIN_INDEX:
        case section::TERRAIN_TEXTURE_LOOKUP:
        case section::TERRAIN_SEASON_JAN:
        case section::TERRAIN_SEASON_FEB:
        case section::TERRAIN_SEASON_MAR:
        case section::TERRAIN_SEASON_APR:
        case section::TERRAIN_SEASON_MAY:
        case section::TERRAIN_SEASON_JUN:
        case section::TERRAIN_SEASON_JUL:
        case section::TERRAIN_SEASON_AUG:
        case section::TERRAIN_SEASON_SEP:
        case section::TERRAIN_SEASON_OCT:
        case section::TERRAIN_SEASON_NOV:
        case section::TERRAIN_SEASON_DEC:
        case section::TERRAIN_PHOTO_JAN:
        case section::TERRAIN_PHOTO_FEB:
        case section::TERRAIN_PHOTO_MAR:
        case section::TERRAIN_PHOTO_APR:
        case section::TERRAIN_PHOTO_MAY:
        case section::TERRAIN_PHOTO_JUN:
        case section::TERRAIN_PHOTO_JUL:
        case section::TERRAIN_PHOTO_AUG:
        case section::TERRAIN_PHOTO_SEP:
        case section::TERRAIN_PHOTO_OCT:
        case section::TERRAIN_PHOTO_NOV:
        case section::TERRAIN_PHOTO_DEC:
        case section::TERRAIN_PHOTO_NIGHT:
        case section::FAKE_TYPES:
        case section::ICAO_RUNWAY:
          break;
        default:
          qWarning().nospace().noquote() << "Unknown section type at offset " << bs->tellg() << ": " << type;

      }
      if(rec == nullptr)
        // Create empty record, just to skip it
        rec = createRecord<Record>(bs, nullptr);

      if(rec->getSize() < bs->getFileSize())
        rec->seekToEnd();
      else
        qWarning().nospace().noquote() << "Invalid record size " << rec->getSize()
                                       << " offset " << bs->tellg()
                                       << hex << " type 0x" << rec->getId() << dec;
    }
  }
}

void BglFile::deleteAllObjects()
{
  airports.clear();
  namelists.clear();
  ils.clear();
  tacans.clear();
  vors.clear();
  ndbs.clear();
  marker.clear();
  waypoints.clear();
  boundaries.clear();
  sections.clear();
  subsections.clear();

  qDeleteAll(allRecords);
  allRecords.clear();

  filename.clear();
  size = 0;
}

} // namespace bgl
} // namespace fs
} // namespace atools
