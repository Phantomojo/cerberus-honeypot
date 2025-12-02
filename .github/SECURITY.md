# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.x.x   | :white_check_mark: |
| < 1.0   | :x:                |

## Reporting a Vulnerability

We take the security of CERBERUS Honeypot seriously. If you believe you have found a security vulnerability, please report it to us as described below.

### How to Report

**Please do NOT report security vulnerabilities through public GitHub issues.**

Instead, please report them via one of the following methods:

1. **GitHub Security Advisories**: Use the "Report a vulnerability" button in the Security tab of this repository
2. **Email**: Contact the maintainers directly (if email is provided in the repository)

### What to Include

Please include the following information in your report:

- Type of vulnerability (e.g., buffer overflow, SQL injection, XSS, etc.)
- Full paths of source file(s) related to the vulnerability
- Location of the affected source code (tag/branch/commit or direct URL)
- Step-by-step instructions to reproduce the issue
- Proof-of-concept or exploit code (if possible)
- Impact of the issue, including how an attacker might exploit it

### Response Timeline

- **Initial Response**: Within 48 hours of your report
- **Status Update**: Within 7 days with our assessment
- **Resolution Timeline**: Depends on severity
  - Critical: 24-72 hours
  - High: 7 days
  - Medium: 30 days
  - Low: 90 days

### What to Expect

1. **Acknowledgment**: We will acknowledge your report within 48 hours
2. **Verification**: We will work to verify the vulnerability
3. **Communication**: We will keep you informed of our progress
4. **Fix**: We will develop and test a fix
5. **Disclosure**: We will coordinate disclosure timing with you
6. **Credit**: We will credit you in our release notes (unless you prefer anonymity)

## Security Best Practices for Deployment

When deploying CERBERUS Honeypot, follow these security guidelines:

### Network Isolation

```bash
# Run honeypot in isolated network segment
# Never expose honeypot management interfaces to untrusted networks
# Use firewall rules to limit outbound connections from honeypot
```

### Container Security

- Run containers with minimal privileges
- Use read-only file systems where possible
- Limit container resources (CPU, memory, network)
- Regularly update base images

### Monitoring

- Monitor honeypot logs for escape attempts
- Set up alerts for unusual activity
- Regularly review captured data

### Data Handling

- Encrypt sensitive captured data
- Follow data retention policies
- Be aware of legal implications in your jurisdiction

## Known Security Considerations

### By Design

CERBERUS is a honeypot - it is intentionally vulnerable to attract attackers. This is by design, not a bug. However:

1. **Host Isolation**: The honeypot must be properly isolated from your real infrastructure
2. **Container Escapes**: We implement multiple layers to prevent container escapes
3. **Data Exfiltration**: Captured malware should be handled carefully

### Intentional "Vulnerabilities"

The following are intentional features, not security bugs:

- Default credentials in fake services
- Simulated vulnerable services
- Fake sensitive data (honey tokens)

## Security Scanning

This repository uses automated security scanning:

- **CodeQL**: Static analysis for C/C++ vulnerabilities
- **Flawfinder**: Security-focused C code analysis
- **cppcheck**: Static analysis for bugs and security issues
- **TruffleHog**: Secret detection
- **Valgrind**: Memory safety checks
- **Dependabot**: Dependency vulnerability alerts

## Acknowledgments

We thank all security researchers who have responsibly disclosed vulnerabilities:

<!-- 
Add acknowledged researchers here:
- Name/Handle - Description of finding (date)
-->

*No vulnerabilities reported yet*

---

Thank you for helping keep CERBERUS Honeypot and its users safe!