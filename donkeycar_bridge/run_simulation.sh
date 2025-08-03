#!/bin/bash

# Simple launcher script for the Donkeycar simulation bridge
# This allows running the donkeycar interface without requiring
# the full CPM Lab components and RTI Connext DDS license

# Check arguments
if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <vehicle_id> [car_path] [--figure-eight]"
    echo "  vehicle_id: The ID of the vehicle to use"
    echo "  car_path: Path to a Donkeycar directory (optional, uses default config if not provided)"
    echo "  --figure-eight: Use figure-eight controller instead of circle controller"
    exit 1
fi

VEHICLE_ID=$1
CONTROLLER="circle"
CAR_PATH=""

# Parse additional arguments
shift
while [ "$#" -gt 0 ]; do
    case "$1" in
        --figure-eight)
            CONTROLLER="figure-eight"
            ;;
        *)
            if [ -z "$CAR_PATH" ]; then
                CAR_PATH=$1
            else
                echo "Unknown argument: $1"
                exit 1
            fi
            ;;
    esac
    shift
done

# Print configuration
echo "Starting Donkeycar Simulation Bridge"
echo "Vehicle ID: $VEHICLE_ID"
echo "Controller: $CONTROLLER"
if [ -n "$CAR_PATH" ]; then
    echo "Car Path: $CAR_PATH"
    CAR_PATH_ARG="--car-path $CAR_PATH"
else
    echo "No car path provided"
    CAR_PATH_ARG=""
fi

# Run the bridge
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
python "$SCRIPT_DIR/simulation/bridge.py" --vehicle-id "$VEHICLE_ID" --controller "$CONTROLLER" $CAR_PATH_ARG