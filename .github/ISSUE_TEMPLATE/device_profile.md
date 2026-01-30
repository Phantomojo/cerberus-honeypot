---
name: Device Profile Request
about: Request a new IoT device profile for CERBERUS
title: '[PROFILE] '
labels: device-profile, enhancement
assignees: ''
---

## Device Information

### Basic Details

- **Manufacturer**: <!-- e.g., TP-Link, Hikvision, Netgear -->
- **Model**: <!-- e.g., Archer C7, DS-2CD2032 -->
- **Device Type**: <!-- Router, IP Camera, NAS, DVR, Smart Home Hub, etc. -->
- **Firmware Version**: <!-- If known -->

### Hardware Specifications

<!-- Fill in what you know, leave blank if unknown -->

- **CPU Architecture**: <!-- MIPS, ARM, ARMv7, AArch64, x86 -->
- **CPU Model**: <!-- e.g., MIPS 74Kc, ARMv7 Cortex-A9 -->
- **CPU Speed**: <!-- e.g., 720 MHz -->
- **RAM**: <!-- e.g., 128 MB -->
- **Flash Storage**: <!-- e.g., 16 MB -->

### Network Fingerprints

<!-- These help make the honeypot convincing -->

- **Default SSH Banner**:
```
<!-- e.g., SSH-2.0-dropbear_2017.75 -->
```

- **Default Telnet Banner**:
```
<!-- e.g., TP-Link Archer C7 v4\r\nLogin: -->
```

- **MAC Address Prefix (OUI)**: <!-- e.g., 14:CC:20 -->

- **Default Open Ports**: <!-- e.g., 22, 23, 80, 443 -->

### Software Details

- **OS/Firmware Base**: <!-- OpenWrt, DD-WRT, Proprietary Linux, etc. -->
- **Kernel Version**: <!-- e.g., 3.10.49 -->
- **BusyBox Version**: <!-- e.g., 1.24.1 -->
- **Default Shell**: <!-- /bin/sh, /bin/ash, etc. -->

### Default Credentials

<!-- Common/default credentials for this device -->

| Username | Password | Notes |
|----------|----------|-------|
| | | |

### Web Interface

- [ ] Has web interface
- **Default Port**: <!-- e.g., 80, 443, 8080 -->
- **Web Server**: <!-- e.g., httpd, lighttpd, nginx -->
- **Notable URLs**: <!-- e.g., /admin, /cgi-bin/login.cgi -->

### Services

<!-- Check all that apply -->

- [ ] SSH (Dropbear)
- [ ] SSH (OpenSSH)
- [ ] Telnet
- [ ] HTTP/HTTPS Web UI
- [ ] RTSP (for cameras)
- [ ] ONVIF (for cameras)
- [ ] UPnP/SSDP
- [ ] DNS/DHCP (dnsmasq)
- [ ] SNMP
- [ ] FTP
- [ ] SMB/CIFS (for NAS)
- [ ] Other:

## Source of Information

<!-- How did you get this information? -->

- [ ] I own this device
- [ ] Firmware dump/analysis
- [ ] Shodan/Censys data
- [ ] Vendor documentation
- [ ] Teardown/reverse engineering
- [ ] Security research paper
- [ ] Other:

### References

<!-- Links to documentation, firmware, research, etc. -->

-

## Sample Data

<!-- If you can provide samples, it helps a lot! -->

### /proc/cpuinfo
```
<!-- Paste actual output if you have it -->
```

### /proc/meminfo
```
<!-- Paste actual output if you have it -->
```

### /etc/passwd
```
<!-- Paste actual output if you have it -->
```

### ps output
```
<!-- Paste actual output if you have it -->
```

### Filesystem layout
```
<!-- Notable directories and files -->
```

## Known Vulnerabilities

<!-- Optional: Known CVEs or vulnerabilities for this device -->
<!-- This helps make the honeypot more attractive to attackers -->

| CVE | Description |
|-----|-------------|
| | |

## Priority

- [ ] High - Commonly targeted device
- [ ] Medium - Moderately popular device  
- [ ] Low - Niche/uncommon device

## Additional Notes

<!-- Any other information that would help create this profile -->

## Checklist

- [ ] I have searched existing profiles to ensure this device isn't already supported
- [ ] I have provided as much accurate information as possible
- [ ] I understand this profile will be used to emulate the device in a honeypot
