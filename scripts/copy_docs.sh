#!/bin/bash

# Configuration
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WEB_DIR="$PROJECT_DIR/web"
DOCS_DIR="$PROJECT_DIR/docs"
README_FILE="$PROJECT_DIR/README.md"
CAD_DIR="$PROJECT_DIR/cad"

# Target directories inside web/
WEB_DOCS_DIR="$WEB_DIR/docs"
WEB_README="$WEB_DIR/README.md"
WEB_CAD_DIR="$WEB_DIR/cad"

echo "Packaging documentation for web deployment..."

# 1. Copy README.md
if [ -f "$README_FILE" ]; then
    cp "$README_FILE" "$WEB_README"
    echo "Copied $README_FILE to $WEB_README"
else
    echo "Error: README.md not found at $README_FILE"
    exit 1
fi

# 2. Copy docs/ folder
if [ -d "$DOCS_DIR" ]; then
    # Use rsync if available, otherwise cp -r
    if command -v rsync >/dev/null 2>&1; then
        rsync -av --delete "$DOCS_DIR/" "$WEB_DOCS_DIR/"
    else
        rm -rf "$WEB_DOCS_DIR"
        cp -r "$DOCS_DIR" "$WEB_DOCS_DIR"
    fi
    echo "Copied $DOCS_DIR to $WEB_DOCS_DIR"
else
    echo "Error: docs/ folder not found at $DOCS_DIR"
    exit 1
fi

# 3. Copy cad/ folder
if [ -d "$CAD_DIR" ]; then
    if command -v rsync >/dev/null 2>&1; then
        rsync -av --delete "$CAD_DIR/" "$WEB_CAD_DIR/"
    else
        rm -rf "$WEB_CAD_DIR"
        cp -r "$CAD_DIR" "$WEB_CAD_DIR"
    fi
    echo "Copied $CAD_DIR to $WEB_CAD_DIR"
else
    echo "Warning: cad/ folder not found at $CAD_DIR"
fi

echo "Documentation packaging complete."
