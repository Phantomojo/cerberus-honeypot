#!/bin/bash
# Script to add dynamic commands to the processes module
# This is called after building the morph binary

set -e  # Exit on error

# Create dynamic commands directory if it doesn't exist
if ! mkdir -p services/cowrie/share/cowrie/txtcmds; then
    echo "Error: Failed to create dynamic commands directory" >&2
    exit 1
fi

# Success message
echo "Dynamic commands setup complete"
exit 0
