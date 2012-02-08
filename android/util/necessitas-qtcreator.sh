#!/bin/bash

#   ***************************************************************************
#     necessitas-qtcreator.sh - Run necessitas qtcreator with the config.conf
#     environment variables set
#      --------------------------------------
#
#      based on Marco Bernasocchi's android QGIS scripts
#
#   ***************************************************************************
#   *                                                                         *
#   *   This program is free software; you can redistribute it and/or modify  *
#   *   it under the terms of the GNU General Public License as published by  *
#   *   the Free Software Foundation; either version 2 of the License, or     *
#   *   (at your option) any later version.                                   *
#   *                                                                         *
#   ***************************************************************************/


source `dirname $0`/config.conf

export BIN_PATH=$NECESSITAS_DIR/QtCreator/bin/
export LD_LIBRARY_PATH=$BIN_PATH/../Qt/lib:$LD_LIBRARY_PATH
export QT_PLUGIN_PATH=$BIN_PATH/../Qt/plugins:$QT_PLUGIN_PATH
export QT_IMPORT_PATH=$BIN_PATH/../Qt/imports:$QT_IMPORT_PATH

if [ "$WINDOWS" == "true" ]; then
 QTCREATOR=$BIN_PATH/qtcreator.exe
else
 QTCREATOR=$BIN_PATH/qtcreator
fi

$QTCREATOR &
echo "to import android branch into necessitas do:
file -> open file or project open CMakeLists.txt
choose a build dirng
pass the cmake flags you obtain from echo $MY_CMAKE_FLAGS in build-stel.sh"
 

