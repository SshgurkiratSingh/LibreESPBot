#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export LD_LIBRARY_PATH="$DIR/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$DIR/lib/plugins"
export QML2_IMPORT_PATH="$DIR/lib/qml"
export FLEXIBLAS_LIBRARY_PATH="$DIR/lib/flexiblas"
exec "$DIR/LibreESPBot" "$@"
