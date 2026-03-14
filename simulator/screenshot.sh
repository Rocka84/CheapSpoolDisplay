#!/bin/bash

# Utility to capture screenshots of the CheapSpoolDisplay simulator.
# Usage: ./simulator/screenshot.sh [output_path.png]
#
# This script targets the window named "LVGL Simulator" and saves it.
# It requires 'import' from the ImageMagick package.

WINDOW_NAME="LVGL Simulator"
DEFAULT_OUTPUT="docs/screen_shot_$(date +%s).png"
OUTPUT_PATH=${1:-$DEFAULT_OUTPUT}

# Check for ImageMagick
if ! command -v import &> /dev/null; then
    echo "Error: 'import' tool not found. Please install ImageMagick (sudo apt install imagemagick)."
    exit 1
fi

# Check if simulator is running
if ! pgrep -f "./simulator/program" > /dev/null && ! pgrep -f "LVGL Simulator" > /dev/null; then
    echo "Warning: Simulator doesn't seem to be running."
    echo "Starting simulator in the background..."
    ./simulator/program &
    SIM_PID=$!
    echo "Waiting for window to appear..."
    sleep 3
fi

echo "Capturing window '$WINDOW_NAME'..."
if import -window "$WINDOW_NAME" "$OUTPUT_PATH"; then
    echo "Successfully saved screenshot to: $OUTPUT_PATH"
else
    echo "Error: Failed to capture window. Make sure 'LVGL Simulator' is visible."
    [ -n "$SIM_PID" ] && kill $SIM_PID
    exit 1
fi

# If we started it, we kill it.

if [ -n "$SIM_PID" ]; then
    echo "Stopping background simulator..."
    kill $SIM_PID
fi
