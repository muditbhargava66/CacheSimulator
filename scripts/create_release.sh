#!/bin/bash
# Release creation script for Cache Simulator
# Creates a release package with binaries and documentation

# Default values
VERSION=$(grep -o 'VERSION [0-9]\+\.[0-9]\+\.[0-9]\+' CMakeLists.txt | awk '{print $2}')
PLATFORM=$(uname -s | tr '[:upper:]' '[:lower:]')
ARCH=$(uname -m)
OUTPUT_DIR="release"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --version=*)
            VERSION="${key#*=}"
            shift
            ;;
        --output=*)
            OUTPUT_DIR="${key#*=}"
            shift
            ;;
        *)
            echo "Unknown option: $key"
            echo "Usage: $0 [--version=X.Y.Z] [--output=dir]"
            exit 1
            ;;
    esac
done

RELEASE_NAME="cachesim-${VERSION}-${PLATFORM}-${ARCH}"
RELEASE_DIR="${OUTPUT_DIR}/${RELEASE_NAME}"

echo "Cache Simulator Release Creation"
echo "==============================="
echo "Version: $VERSION"
echo "Platform: $PLATFORM-$ARCH"
echo "Output directory: $OUTPUT_DIR"
echo "Release name: $RELEASE_NAME"
echo ""

# Check if build exists
if [ ! -d "build/bin" ]; then
    echo "Error: Build not found. Please build the project first."
    echo "Run: ./scripts/build_all.sh"
    exit 1
fi

# Create release directory structure
echo "Creating release directory structure..."
mkdir -p "$RELEASE_DIR/bin"
mkdir -p "$RELEASE_DIR/docs"
mkdir -p "$RELEASE_DIR/configs"
mkdir -p "$RELEASE_DIR/traces"
mkdir -p "$RELEASE_DIR/scripts"

# Copy binaries
echo "Copying binaries..."
cp -r build/bin/* "$RELEASE_DIR/bin/"

# Copy documentation
echo "Copying documentation..."
cp -r docs/* "$RELEASE_DIR/docs/"
cp README.md "$RELEASE_DIR/"
cp CHANGELOG.md "$RELEASE_DIR/"
cp LICENSE "$RELEASE_DIR/"

# Copy configuration files
echo "Copying configuration files..."
cp configs/*.json "$RELEASE_DIR/configs/"
cp configs/README.md "$RELEASE_DIR/configs/"

# Copy trace files
echo "Copying trace files..."
cp traces/*.txt "$RELEASE_DIR/traces/"
cp traces/README.md "$RELEASE_DIR/traces/"

# Copy scripts
echo "Copying scripts..."
cp scripts/run_benchmarks.sh "$RELEASE_DIR/scripts/"
chmod +x "$RELEASE_DIR/scripts/run_benchmarks.sh"

# Create version file
echo "Creating version file..."
echo "Cache Simulator v$VERSION" > "$RELEASE_DIR/VERSION"
echo "Build date: $(date)" >> "$RELEASE_DIR/VERSION"
echo "Platform: $PLATFORM-$ARCH" >> "$RELEASE_DIR/VERSION"

# Create archive
echo "Creating release archive..."
cd "$OUTPUT_DIR"
if [ "$PLATFORM" = "darwin" ] || [ "$PLATFORM" = "linux" ]; then
    tar -czf "${RELEASE_NAME}.tar.gz" "$RELEASE_NAME"
    echo "Created ${RELEASE_NAME}.tar.gz"
fi

if [ "$PLATFORM" = "darwin" ]; then
    zip -r "${RELEASE_NAME}.zip" "$RELEASE_NAME"
    echo "Created ${RELEASE_NAME}.zip"
fi

echo ""
echo "Release creation completed!"
echo "Release files available in: $OUTPUT_DIR"