#*****************************************************************************
# Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#****************************************************************************

# =============================================================================
# Set these environment variables for configuration
# see the build folder for templates

# Path to GIT executable - revision will be set to "UNKNOWN" if not set
# C:\\Git\\bin\\git
GIT_PATH=$$(ATOOLS_GIT_PATH)

# Path to SimConnect SDK - will be omitted in build if not set
# "C:\Program Files (x86)\Microsoft Games\Microsoft Flight Simulator X SDK\SDK\Core Utilities Kit\SimConnect SDK"
SIMCONNECT_PATH=$$(ATOOLS_SIMCONNECT_PATH)

# =============================================================================

QT += sql xml svg core widgets network
QT -= gui

CONFIG += c++14

# Use to debug release builds - set on command line
#CONFIG+=force_debug_info

INCLUDEPATH += $$PWD/src

DEFINES += QT_NO_CAST_FROM_BYTEARRAY
DEFINES += QT_NO_CAST_TO_ASCII

unix {
  isEmpty(GIT_PATH) : GIT_PATH=git
}

win32 {
  DEFINES += _USE_MATH_DEFINES
  DEFINES += NOMINMAX

  !isEmpty(SIMCONNECT_PATH) {
    DEFINES += SIMCONNECT_BUILD
    INCLUDEPATH += $$SIMCONNECT_PATH + "\inc"
    LIBS += $$SIMCONNECT_PATH + "\lib\SimConnect.lib"
  }
}

macx {
  # Compatibility down to OS X Mountain Lion 10.8
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8
}

isEmpty(GIT_PATH) {
  GIT_REVISION='\\"UNKNOWN\\"'
} else {
  GIT_REVISION='\\"$$system('$$GIT_PATH' rev-parse --short HEAD)\\"'
}

DEFINES += GIT_REVISION_ATOOLS=$$GIT_REVISION

message(-----------------------------------)
message(GIT_PATH: $$GIT_PATH)
message(GIT_REVISION: $$GIT_REVISION)
message(SIMCONNECT_PATH: $$SIMCONNECT_PATH)
message(DEFINES: $$DEFINES)
message(-----------------------------------)

TARGET = atools
TEMPLATE = lib
CONFIG += staticlib

HEADERS += \
    src/atools.h \
    src/exception.h \
    src/fs/ap/airportloader.h \
    src/fs/bgl/ap/airport.h \
    src/fs/bgl/ap/approach.h \
    src/fs/bgl/ap/approachleg.h \
    src/fs/bgl/ap/approachtypes.h \
    src/fs/bgl/ap/apron.h \
    src/fs/bgl/ap/apron2.h \
    src/fs/bgl/ap/apronedgelight.h \
    src/fs/bgl/ap/com.h \
    src/fs/bgl/ap/del/deleteairport.h \
    src/fs/bgl/ap/del/deletecom.h \
    src/fs/bgl/ap/del/deleterunway.h \
    src/fs/bgl/ap/del/deletestart.h \
    src/fs/bgl/ap/fence.h \
    src/fs/bgl/ap/helipad.h \
    src/fs/bgl/ap/jetway.h \
    src/fs/bgl/ap/parking.h \
    src/fs/bgl/ap/rw/runway.h \
    src/fs/bgl/ap/rw/runwayapplights.h \
    src/fs/bgl/ap/rw/runwayend.h \
    src/fs/bgl/ap/rw/runwayvasi.h \
    src/fs/bgl/ap/start.h \
    src/fs/bgl/ap/taxipath.h \
    src/fs/bgl/ap/taxipoint.h \
    src/fs/bgl/ap/transition.h \
    src/fs/bgl/bglbase.h \
    src/fs/bgl/bglfile.h \
    src/fs/bgl/bglposition.h \
    src/fs/bgl/boundary.h \
    src/fs/bgl/boundarysegment.h \
    src/fs/bgl/converter.h \
    src/fs/bgl/header.h \
    src/fs/bgl/nav/airwaysegment.h \
    src/fs/bgl/nav/airwaywaypoint.h \
    src/fs/bgl/nav/dme.h \
    src/fs/bgl/nav/glideslope.h \
    src/fs/bgl/nav/ils.h \
    src/fs/bgl/nav/ilsvor.h \
    src/fs/bgl/nav/localizer.h \
    src/fs/bgl/nav/marker.h \
    src/fs/bgl/nav/navbase.h \
    src/fs/bgl/nav/ndb.h \
    src/fs/bgl/nav/tacan.h \
    src/fs/bgl/nav/vor.h \
    src/fs/bgl/nav/waypoint.h \
    src/fs/bgl/nl/namelist.h \
    src/fs/bgl/nl/namelistentry.h \
    src/fs/bgl/record.h \
    src/fs/bgl/recordtypes.h \
    src/fs/bgl/section.h \
    src/fs/bgl/sectiontype.h \
    src/fs/bgl/subsection.h \
    src/fs/bgl/util.h \
    src/fs/common/airportindex.h \
    src/fs/common/binarygeometry.h \
    src/fs/common/globereader.h \
    src/fs/common/magdecreader.h \
    src/fs/common/metadatawriter.h \
    src/fs/common/morareader.h \
    src/fs/common/procedurewriter.h \
    src/fs/common/xpgeometry.h \
    src/fs/db/airwayresolver.h \
    src/fs/db/ap/airportfilewriter.h \
    src/fs/db/ap/airportwriter.h \
    src/fs/db/ap/approachlegwriter.h \
    src/fs/db/ap/approachwriter.h \
    src/fs/db/ap/apronlightwriter.h \
    src/fs/db/ap/apronwriter.h \
    src/fs/db/ap/comwriter.h \
    src/fs/db/ap/deleteairportwriter.h \
    src/fs/db/ap/deleteprocessor.h \
    src/fs/db/ap/fencewriter.h \
    src/fs/db/ap/helipadwriter.h \
    src/fs/db/ap/legbasewriter.h \
    src/fs/db/ap/parkingwriter.h \
    src/fs/db/ap/rw/runwayendwriter.h \
    src/fs/db/ap/rw/runwaywriter.h \
    src/fs/db/ap/startwriter.h \
    src/fs/db/ap/taxipathwriter.h \
    src/fs/db/ap/transitionlegwriter.h \
    src/fs/db/ap/transitionwriter.h \
    src/fs/db/databasemeta.h \
    src/fs/db/datawriter.h \
    src/fs/db/dbairportindex.h \
    src/fs/db/meta/bglfilewriter.h \
    src/fs/db/meta/sceneryareawriter.h \
    src/fs/db/nav/airwaysegmentwriter.h \
    src/fs/db/nav/boundarywriter.h \
    src/fs/db/nav/ilswriter.h \
    src/fs/db/nav/markerwriter.h \
    src/fs/db/nav/ndbwriter.h \
    src/fs/db/nav/tacanwriter.h \
    src/fs/db/nav/vorwriter.h \
    src/fs/db/nav/waypointwriter.h \
    src/fs/db/routeedgewriter.h \
    src/fs/db/runwayindex.h \
    src/fs/db/writerbase.h \
    src/fs/db/writerbasebasic.h \
    src/fs/dfd/dfdcompiler.h \
    src/fs/fspaths.h \
    src/fs/lb/logbook.h \
    src/fs/lb/logbookentry.h \
    src/fs/lb/logbookentryfilter.h \
    src/fs/lb/logbookloader.h \
    src/fs/lb/types.h \
    src/fs/navdatabase.h \
    src/fs/navdatabaseerrors.h \
    src/fs/navdatabaseoptions.h \
    src/fs/navdatabaseprogress.h \
    src/fs/ns/navserver.h \
    src/fs/ns/navservercommon.h \
    src/fs/ns/navserverworker.h \
    src/fs/online/onlinedatamanager.h \
    src/fs/online/onlinetypes.h \
    src/fs/online/statustextparser.h \
    src/fs/online/whazzuptextparser.h \
    src/fs/perf/aircraftperf.h \
    src/fs/perf/aircraftperfconstants.h \
    src/fs/perf/aircraftperfhandler.h \
    src/fs/pln/flightplan.h \
    src/fs/pln/flightplanconstants.h \
    src/fs/pln/flightplanentry.h \
    src/fs/pln/flightplanio.h \
    src/fs/progresshandler.h \
    src/fs/sc/connecthandler.h \
    src/fs/sc/datareaderthread.h \
    src/fs/sc/simconnectaircraft.h \
    src/fs/sc/simconnectapi.h \
    src/fs/sc/simconnectdata.h \
    src/fs/sc/simconnectdatabase.h \
    src/fs/sc/simconnectdummy.h \
    src/fs/sc/simconnecthandler.h \
    src/fs/sc/simconnectreply.h \
    src/fs/sc/simconnecttypes.h \
    src/fs/sc/simconnectuseraircraft.h \
    src/fs/sc/weatherrequest.h \
    src/fs/sc/xpconnecthandler.h \
    src/fs/scenery/addoncfg.h \
    src/fs/scenery/addoncomponent.h \
    src/fs/scenery/addonpackage.h \
    src/fs/scenery/fileresolver.h \
    src/fs/scenery/sceneryarea.h \
    src/fs/scenery/scenerycfg.h \
    src/fs/userdata/userdatamanager.h \
    src/fs/util/coordinates.h \
    src/fs/util/fsutil.h \
    src/fs/util/morsecode.h \
    src/fs/util/tacanfrequencies.h \
    src/fs/weather/metar.h \
    src/fs/weather/metarparser.h \
    src/fs/weather/weathernetdownload.h \
    src/fs/weather/weathernetsingle.h \
    src/fs/weather/weathertypes.h \
    src/fs/weather/xpweatherreader.h \
    src/fs/xp/airwaypostprocess.h \
    src/fs/xp/scenerypacks.h \
    src/fs/xp/xpairportwriter.h \
    src/fs/xp/xpairspacewriter.h \
    src/fs/xp/xpairwaywriter.h \
    src/fs/xp/xpcifpwriter.h \
    src/fs/xp/xpconstants.h \
    src/fs/xp/xpdatacompiler.h \
    src/fs/xp/xpfixwriter.h \
    src/fs/xp/xpnavwriter.h \
    src/fs/xp/xpwriter.h \
    src/geo/calculations.h \
    src/geo/line.h \
    src/geo/linestring.h \
    src/geo/pos.h \
    src/geo/rect.h \
    src/geo/simplespatialindex.h \
    src/gui/actionstatesaver.h \
    src/gui/actiontextsaver.h \
    src/gui/application.h \
    src/gui/consoleapplication.h \
    src/gui/dialog.h \
    src/gui/errorhandler.h \
    src/gui/filehistoryhandler.h \
    src/gui/griddelegate.h \
    src/gui/helphandler.h \
    src/gui/itemviewzoomhandler.h \
    src/gui/mapposhistory.h \
    src/gui/palettesettings.h \
    src/gui/tools.h \
    src/gui/translator.h \
    src/gui/widgetstate.h \
    src/gui/widgetutil.h \
    src/io/binarystream.h \
    src/io/fileroller.h \
    src/io/inireader.h \
    src/logging/loggingconfig.h \
    src/logging/loggingguiabort.h \
    src/logging/logginghandler.h \
    src/logging/loggingtypes.h \
    src/logging/loggingutil.h \
    src/settings/settings.h \
    src/sql/sqldatabase.h \
    src/sql/sqlexception.h \
    src/sql/sqlexport.h \
    src/sql/sqlquery.h \
    src/sql/sqlrecord.h \
    src/sql/sqlscript.h \
    src/sql/sqltransaction.h \
    src/sql/sqlutil.h \
    src/util/csvreader.h \
    src/util/filesystemwatcher.h \
    src/util/heap.h \
    src/util/htmlbuilder.h \
    src/util/httpdownloader.h \
    src/util/paintercontextsaver.h \
    src/util/roundedpolygon.h \
    src/util/timedcache.h \
    src/util/updatecheck.h \
    src/util/version.h \
    src/win/activationcontext.h \
    src/zip/gzip.h \
    src/zip/zipreader.h \
    src/zip/zipwriter.h

SOURCES += \
    src/atools.cpp \
    src/exception.cpp \
    src/fs/ap/airportloader.cpp \
    src/fs/bgl/ap/airport.cpp \
    src/fs/bgl/ap/approach.cpp \
    src/fs/bgl/ap/approachleg.cpp \
    src/fs/bgl/ap/approachtypes.cpp \
    src/fs/bgl/ap/apron.cpp \
    src/fs/bgl/ap/apron2.cpp \
    src/fs/bgl/ap/apronedgelight.cpp \
    src/fs/bgl/ap/com.cpp \
    src/fs/bgl/ap/del/deleteairport.cpp \
    src/fs/bgl/ap/del/deletecom.cpp \
    src/fs/bgl/ap/del/deleterunway.cpp \
    src/fs/bgl/ap/del/deletestart.cpp \
    src/fs/bgl/ap/fence.cpp \
    src/fs/bgl/ap/helipad.cpp \
    src/fs/bgl/ap/jetway.cpp \
    src/fs/bgl/ap/parking.cpp \
    src/fs/bgl/ap/rw/runway.cpp \
    src/fs/bgl/ap/rw/runwayapplights.cpp \
    src/fs/bgl/ap/rw/runwayend.cpp \
    src/fs/bgl/ap/rw/runwayvasi.cpp \
    src/fs/bgl/ap/start.cpp \
    src/fs/bgl/ap/taxipath.cpp \
    src/fs/bgl/ap/taxipoint.cpp \
    src/fs/bgl/ap/transition.cpp \
    src/fs/bgl/bglbase.cpp \
    src/fs/bgl/bglfile.cpp \
    src/fs/bgl/bglposition.cpp \
    src/fs/bgl/boundary.cpp \
    src/fs/bgl/boundarysegment.cpp \
    src/fs/bgl/converter.cpp \
    src/fs/bgl/header.cpp \
    src/fs/bgl/nav/airwaysegment.cpp \
    src/fs/bgl/nav/airwaywaypoint.cpp \
    src/fs/bgl/nav/dme.cpp \
    src/fs/bgl/nav/glideslope.cpp \
    src/fs/bgl/nav/ils.cpp \
    src/fs/bgl/nav/ilsvor.cpp \
    src/fs/bgl/nav/localizer.cpp \
    src/fs/bgl/nav/marker.cpp \
    src/fs/bgl/nav/navbase.cpp \
    src/fs/bgl/nav/ndb.cpp \
    src/fs/bgl/nav/tacan.cpp \
    src/fs/bgl/nav/vor.cpp \
    src/fs/bgl/nav/waypoint.cpp \
    src/fs/bgl/nl/namelist.cpp \
    src/fs/bgl/nl/namelistentry.cpp \
    src/fs/bgl/record.cpp \
    src/fs/bgl/recordtypes.cpp \
    src/fs/bgl/section.cpp \
    src/fs/bgl/sectiontype.cpp \
    src/fs/bgl/subsection.cpp \
    src/fs/bgl/util.cpp \
    src/fs/common/airportindex.cpp \
    src/fs/common/binarygeometry.cpp \
    src/fs/common/globereader.cpp \
    src/fs/common/magdecreader.cpp \
    src/fs/common/metadatawriter.cpp \
    src/fs/common/morareader.cpp \
    src/fs/common/procedurewriter.cpp \
    src/fs/common/xpgeometry.cpp \
    src/fs/db/airwayresolver.cpp \
    src/fs/db/ap/airportfilewriter.cpp \
    src/fs/db/ap/airportwriter.cpp \
    src/fs/db/ap/approachlegwriter.cpp \
    src/fs/db/ap/approachwriter.cpp \
    src/fs/db/ap/apronlightwriter.cpp \
    src/fs/db/ap/apronwriter.cpp \
    src/fs/db/ap/comwriter.cpp \
    src/fs/db/ap/deleteairportwriter.cpp \
    src/fs/db/ap/deleteprocessor.cpp \
    src/fs/db/ap/fencewriter.cpp \
    src/fs/db/ap/helipadwriter.cpp \
    src/fs/db/ap/legbasewriter.cpp \
    src/fs/db/ap/parkingwriter.cpp \
    src/fs/db/ap/rw/runwayendwriter.cpp \
    src/fs/db/ap/rw/runwaywriter.cpp \
    src/fs/db/ap/startwriter.cpp \
    src/fs/db/ap/taxipathwriter.cpp \
    src/fs/db/ap/transitionlegwriter.cpp \
    src/fs/db/ap/transitionwriter.cpp \
    src/fs/db/databasemeta.cpp \
    src/fs/db/datawriter.cpp \
    src/fs/db/dbairportindex.cpp \
    src/fs/db/meta/bglfilewriter.cpp \
    src/fs/db/meta/sceneryareawriter.cpp \
    src/fs/db/nav/airwaysegmentwriter.cpp \
    src/fs/db/nav/boundarywriter.cpp \
    src/fs/db/nav/ilswriter.cpp \
    src/fs/db/nav/markerwriter.cpp \
    src/fs/db/nav/ndbwriter.cpp \
    src/fs/db/nav/tacanwriter.cpp \
    src/fs/db/nav/vorwriter.cpp \
    src/fs/db/nav/waypointwriter.cpp \
    src/fs/db/routeedgewriter.cpp \
    src/fs/db/runwayindex.cpp \
    src/fs/db/writerbasebasic.cpp \
    src/fs/dfd/dfdcompiler.cpp \
    src/fs/fspaths.cpp \
    src/fs/lb/logbook.cpp \
    src/fs/lb/logbookentry.cpp \
    src/fs/lb/logbookentryfilter.cpp \
    src/fs/lb/logbookloader.cpp \
    src/fs/lb/types.cpp \
    src/fs/navdatabase.cpp \
    src/fs/navdatabaseerrors.cpp \
    src/fs/navdatabaseoptions.cpp \
    src/fs/navdatabaseprogress.cpp \
    src/fs/ns/navserver.cpp \
    src/fs/ns/navservercommon.cpp \
    src/fs/ns/navserverworker.cpp \
    src/fs/online/onlinedatamanager.cpp \
    src/fs/online/onlinetypes.cpp \
    src/fs/online/statustextparser.cpp \
    src/fs/online/whazzuptextparser.cpp \
    src/fs/perf/aircraftperf.cpp \
    src/fs/perf/aircraftperfconstants.cpp \
    src/fs/perf/aircraftperfhandler.cpp \
    src/fs/pln/flightplan.cpp \
    src/fs/pln/flightplanconstants.cpp \
    src/fs/pln/flightplanentry.cpp \
    src/fs/pln/flightplanio.cpp \
    src/fs/progresshandler.cpp \
    src/fs/sc/connecthandler.cpp \
    src/fs/sc/datareaderthread.cpp \
    src/fs/sc/simconnectaircraft.cpp \
    src/fs/sc/simconnectapi.cpp \
    src/fs/sc/simconnectdata.cpp \
    src/fs/sc/simconnectdatabase.cpp \
    src/fs/sc/simconnectdummy.cpp \
    src/fs/sc/simconnecthandler.cpp \
    src/fs/sc/simconnectreply.cpp \
    src/fs/sc/simconnectuseraircraft.cpp \
    src/fs/sc/weatherrequest.cpp \
    src/fs/sc/xpconnecthandler.cpp \
    src/fs/scenery/addoncfg.cpp \
    src/fs/scenery/addoncomponent.cpp \
    src/fs/scenery/addonpackage.cpp \
    src/fs/scenery/fileresolver.cpp \
    src/fs/scenery/sceneryarea.cpp \
    src/fs/scenery/scenerycfg.cpp \
    src/fs/userdata/userdatamanager.cpp \
    src/fs/util/coordinates.cpp \
    src/fs/util/fsutil.cpp \
    src/fs/util/morsecode.cpp \
    src/fs/util/tacanfrequencies.cpp \
    src/fs/weather/metar.cpp \
    src/fs/weather/metarparser.cpp \
    src/fs/weather/weathernetdownload.cpp \
    src/fs/weather/weathernetsingle.cpp \
    src/fs/weather/weathertypes.cpp \
    src/fs/weather/xpweatherreader.cpp \
    src/fs/xp/airwaypostprocess.cpp \
    src/fs/xp/scenerypacks.cpp \
    src/fs/xp/xpairportwriter.cpp \
    src/fs/xp/xpairspacewriter.cpp \
    src/fs/xp/xpairwaywriter.cpp \
    src/fs/xp/xpcifpwriter.cpp \
    src/fs/xp/xpconstants.cpp \
    src/fs/xp/xpdatacompiler.cpp \
    src/fs/xp/xpfixwriter.cpp \
    src/fs/xp/xpnavwriter.cpp \
    src/fs/xp/xpwriter.cpp \
    src/geo/calculations.cpp \
    src/geo/line.cpp \
    src/geo/linestring.cpp \
    src/geo/pos.cpp \
    src/geo/rect.cpp \
    src/geo/simplespatialindex.cpp \
    src/gui/actionstatesaver.cpp \
    src/gui/actiontextsaver.cpp \
    src/gui/application.cpp \
    src/gui/consoleapplication.cpp \
    src/gui/dialog.cpp \
    src/gui/errorhandler.cpp \
    src/gui/filehistoryhandler.cpp \
    src/gui/griddelegate.cpp \
    src/gui/helphandler.cpp \
    src/gui/itemviewzoomhandler.cpp \
    src/gui/mapposhistory.cpp \
    src/gui/palettesettings.cpp \
    src/gui/tools.cpp \
    src/gui/translator.cpp \
    src/gui/widgetstate.cpp \
    src/gui/widgetutil.cpp \
    src/io/binarystream.cpp \
    src/io/fileroller.cpp \
    src/io/inireader.cpp \
    src/logging/loggingconfig.cpp \
    src/logging/loggingguiabort.cpp \
    src/logging/logginghandler.cpp \
    src/logging/loggingutil.cpp \
    src/settings/settings.cpp \
    src/sql/sqldatabase.cpp \
    src/sql/sqlexception.cpp \
    src/sql/sqlexport.cpp \
    src/sql/sqlquery.cpp \
    src/sql/sqlrecord.cpp \
    src/sql/sqlscript.cpp \
    src/sql/sqltransaction.cpp \
    src/sql/sqlutil.cpp \
    src/util/csvreader.cpp \
    src/util/filesystemwatcher.cpp \
    src/util/heap.cpp \
    src/util/htmlbuilder.cpp \
    src/util/httpdownloader.cpp \
    src/util/paintercontextsaver.cpp \
    src/util/roundedpolygon.cpp \
    src/util/timedcache.cpp \
    src/util/updatecheck.cpp \
    src/util/version.cpp \
    src/win/activationcontext.cpp \
    src/zip/gzip.cpp \
    src/zip/zip.cpp


unix {
    target.path = /usr/lib
    INSTALLS += target
}

RESOURCES += \
    atools.qrc

OTHER_FILES += build/* \
    LICENSE.txt \
    README.txt \
    CHANGELOG.txt \
    BUILD.txt \
    resources/sql/fs/db/README.txt \
    uncrustify.cfg

TRANSLATIONS = atools_fr.ts \
               atools_it.ts \
               atools_nl.ts \
               atools_de.ts \
               atools_es.ts \
               atools_pt_BR.ts
