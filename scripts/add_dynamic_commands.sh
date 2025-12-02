#!/bin/bash
# Script to add dynamic commands to the processes module
# This is called after building the morph binary

# Create dynamic commands directory if it doesn't exist
mkdir -p services/cowrie/share/cowrie/txtcmds

# Success message
echo "Dynamic commands setup complete"
exit 0
