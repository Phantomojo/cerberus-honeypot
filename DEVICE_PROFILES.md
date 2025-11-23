# CERBERUS Device Profiles

This document describes the realistic router and CCTV device profiles used in the CERBERUS honeynet.

## Router Profiles

### TP-Link Archer C7
- **SSH Banner**: `SSH-2.0-dropbear_2017.75`
- **Telnet Banner**: `Welcome to TP-Link Router`
- **Description**: Common home router using Dropbear SSH (lightweight SSH server common in embedded devices)
- **HTML Theme**: `tplink.html` - Purple gradient theme matching TP-Link branding

### D-Link DIR-615
- **SSH Banner**: `SSH-2.0-OpenSSH_6.7p1`
- **Telnet Banner**: `D-Link System`
- **Description**: Popular budget router with older OpenSSH version
- **HTML Theme**: `dlink.html` - Blue header theme matching D-Link style

### Netgear R7000
- **SSH Banner**: `SSH-2.0-dropbear_2015.71`
- **Telnet Banner**: `NETGEAR ReadyNAS`
- **Description**: High-end router with Dropbear SSH
- **HTML Theme**: `netgear.html` - Dark theme with cyan accents

## CCTV Camera Profiles

### Hikvision DS-2CD2
- **SSH Banner**: `SSH-2.0-OpenSSH_5.8p1`
- **Telnet Banner**: `Hikvision IP Camera`
- **Description**: Popular Chinese IP camera brand, commonly targeted
- **HTML Theme**: `hikvision.html` - Dark theme with blue accents, camera interface

### Dahua IPC-HDW
- **SSH Banner**: `SSH-2.0-OpenSSH_6.0p1`
- **Telnet Banner**: `Dahua Technology`
- **Description**: Another major CCTV manufacturer, frequently exploited
- **HTML Theme**: `dahua.html` - GitHub-style dark theme

## Why These Devices?

1. **High Attack Surface**: These devices are commonly found in home networks
2. **Known Vulnerabilities**: Many have documented security issues
3. **Default Credentials**: Often ship with weak/default passwords
4. **Internet Exposure**: Frequently exposed to the internet
5. **Realistic Fingerprints**: Actual SSH/Telnet banners from real devices

## Cowrie Configuration

The morphing engine updates Cowrie's configuration files:
- `services/cowrie/etc/cowrie.cfg.local` - Local overrides
- SSH and Telnet banners are updated to match the selected profile
- Hostname is set to the profile name for logging

## Profile Rotation

The morphing engine rotates through profiles automatically:
1. Reads current profile state
2. Selects next profile in sequence
3. Updates Cowrie banners
4. Swaps HTML themes
5. Logs the morph event
6. Saves new state

This makes the honeynet appear to change device types, making it harder for attackers to fingerprint the system.

