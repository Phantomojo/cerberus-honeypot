#!/bin/bash
# CERBERUS Network Masking Application Script
# Usage: ./apply_net_masking.sh <container_name> <ttl> <tcp_window>

CONTAINER=$1
TTL=$2
WINDOW=$3

if [ -z "$CONTAINER" ] || [ -z "$TTL" ]; then
    echo "Usage: $0 <container_name> <ttl> [tcp_window]"
    exit 1
fi

echo "Applying network masking to $CONTAINER: TTL=$TTL"

# Set TTL via sysctl inside the container
# This requires --privileged or --cap-add=NET_ADMIN
docker exec -u root "$CONTAINER" sysctl -w net.ipv4.ip_default_ttl="$TTL" || \
    echo "Warning: Could not set TTL. Container may lack NET_ADMIN privileges."

# TCP Window size is harder to set globally via sysctl for established connections,
# but we can set the default receive window
if [ ! -z "$WINDOW" ]; then
    docker exec -u root "$CONTAINER" sysctl -w net.ipv4.tcp_rmem="4096 $WINDOW 6291456"
    docker exec -u root "$CONTAINER" sysctl -w net.ipv4.tcp_wmem="4096 $WINDOW 4194304"
fi
