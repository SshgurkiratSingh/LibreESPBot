#!/bin/bash
# package_release.sh
# Collects all dependencies for LibreESPBot and packages them into a portable release

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
APP_NAME="LibreESPBot"
BUILD_DIR="$DIR/../software/controller_app/build"
RELEASE_DIR="$DIR/../release/LibreESPBot-Linux-x86_64"

echo "Creating release directory at $RELEASE_DIR"
rm -rf "$RELEASE_DIR"
mkdir -p "$RELEASE_DIR/lib"

# 1. Copy the executable
echo "Copying executable..."
cp "$BUILD_DIR/$APP_NAME" "$RELEASE_DIR/"

# 2. Collect shared libraries
echo "Collecting shared libraries via ldd..."
LIBS=$(ldd "$BUILD_DIR/$APP_NAME" | awk 'NF == 4 {print $3}; NF == 2 {print $1}')
for lib in $LIBS; do
    # Skip core system libraries that shouldn't be bundled (libc, libm, libpthread, libdl, etc.)
    if [[ "$lib" == *"libc.so"* || "$lib" == *"libm.so"* || "$lib" == *"libdl.so"* || "$lib" == *"libpthread.so"* || "$lib" == *"libresolv.so"* || "$lib" == *"ld-linux"* ]]; then
        continue
    fi
    
    if [ -f "$lib" ]; then
        cp "$lib" "$RELEASE_DIR/lib/"
    fi
done

# Copy any loaded OpenCV xml resources
if [ -f "$BUILD_DIR/haarcascade_frontalface_default.xml" ]; then
    cp "$BUILD_DIR/haarcascade_frontalface_default.xml" "$RELEASE_DIR/"
fi

# Copy FlexiBLAS plugins if they exist
if [ -d "/usr/lib64/flexiblas" ]; then
    cp -r "/usr/lib64/flexiblas" "$RELEASE_DIR/lib/"
fi

# 3. Create run.sh wrapper
echo "Creating launch script..."
cat > "$RELEASE_DIR/run.sh" << 'EOF'
#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export LD_LIBRARY_PATH="$DIR/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$DIR/lib/plugins"
export QML2_IMPORT_PATH="$DIR/lib/qml"
export FLEXIBLAS_LIBRARY_PATH="$DIR/lib/flexiblas"
exec "$DIR/LibreESPBot" "$@"
EOF

chmod +x "$RELEASE_DIR/run.sh"

# 4. Zip release
echo "Creating zip archive..."
cd "$DIR/../release"
zip -r "LibreESPBot-Linux-x86_64.zip" "LibreESPBot-Linux-x86_64"

echo "Release package created successfully: release/LibreESPBot-Linux-x86_64.zip"
