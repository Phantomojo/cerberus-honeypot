# Cerberus Honeypot Real-Time Monitoring Guide

## Overview

This guide explains the real-time monitoring and alerting system implemented for the Cerberus Honeypot project.

## Monitoring System Architecture

The monitoring system provides real-time visibility into honeypot operations, security events, and system health.

### Core Components

#### 1. Alert Management
- **Alert Types**: Security, Performance, System, Network
- **Alert Levels**: Info, Warning, Error, Critical, Emergency
- **Alert Status**: New, Acknowledged, Resolved, Suppressed
- **Alert Persistence**: In-memory storage with optional persistence

#### 2. Resource Monitoring
- **CPU Usage**: Real-time CPU percentage monitoring
- **Memory Usage**: Memory consumption tracking
- **Disk Usage**: Disk space and I/O monitoring
- **Network Connections**: Active connection tracking
- **Service Health**: Service status and availability

#### 3. Notification System
- **Email Notifications**: SMTP email alerts
- **Webhook Notifications**: HTTP POST to webhook URLs
- **Syslog Integration**: System log integration
- **SMS Notifications**: SMS gateway integration
- **Slack Integration**: Slack channel notifications

#### 4. Threshold Monitoring
- **Configurable Thresholds**: CPU, Memory, Disk, Connections
- **Adaptive Thresholds**: Dynamic threshold adjustment
- **Rate Limiting**: Alert rate limiting to prevent spam

## Alert Types

### Security Alerts
- **Authentication Failures**: Failed login attempts
- **Suspicious Activities**: Unusual behavior patterns
- **Security Violations**: Policy violations
- **Intrusion Attempts**: Attack detection
- **Malware Detection**: Malware-like behavior

### Performance Alerts
- **High CPU Usage**: CPU usage above threshold
- **High Memory Usage**: Memory usage above threshold
- **High Disk Usage**: Disk usage above threshold
- **Slow Response Times**: Performance degradation
- **Service Unavailable**: Service downtime

### System Alerts
- **Service Crashes**: Service process termination
- **Resource Exhaustion**: System resource limits
- **Configuration Errors**: Configuration issues
- **Database Issues**: Database connectivity problems

### Network Alerts
- **Connection Failures**: Network connectivity issues
- **High Connection Count**: Too many connections
- **Bandwidth Usage**: Network bandwidth monitoring
- **DDoS Attacks**: Distributed denial of service
- **Port Scanning**: Port scanning detection

## Configuration

### Basic Configuration
\`\`\`json
{
  "monitoring": {
    "enabled": true,
    "interval": 30,
    "log_file": "/var/log/cerberus-monitoring.log",
    "max_alerts": 1000,
    "alert_retention_days": 30
  },
  "alerts": {
    "email": {
      "enabled": false,
      "smtp_server": "smtp.example.com",
      "smtp_port": 587,
      "username": "alerts@example.com",
      "password": "password",
      "from_address": "cerberus@example.com"
    },
    "webhook": {
      "enabled": false,
      "url": "https://hooks.example.com/cerberus",
      "timeout": 10,
      "retry_count": 3
    },
    "thresholds": {
      "cpu_percent": 80,
      "memory_mb": 1024,
      "disk_percent": 85,
      "connections": 100,
      "error_rate_percent": 5
    }
  }
}
\`\`\`

### Advanced Configuration
\`\`\`json
{
  "monitoring": {
    "enabled": true,
    "interval": 15,
    "log_file": "/var/log/cerberus-monitoring.log",
    "max_alerts": 5000,
    "alert_retention_days": 90,
    "performance_monitoring": true
  },
  "alerts": {
    "escalation": {
      "enabled": true,
      "rules": [
        {
          "condition": "level == 'ERROR' AND count > 3 in 5m",
          "action": "escalate_to_critical"
        },
        {
          "condition": "type == 'SECURITY' AND level == 'ERROR'",
          "action": "immediate_notification"
        }
      ]
    },
    "correlation": {
      "enabled": true,
      "rules": [
        {
          "condition": "type == 'NETWORK' AND source == 'cowrie'",
          "action": "check_service_health"
        }
      ]
    },
    "suppression": {
      "enabled": true,
      "rules": [
        {
          "condition": "message CONTAINS 'test'",
          "action": "suppress_for_1h"
        }
      ]
    }
  }
}
\`\`\`

## Usage Examples

### Basic Monitoring
\`\`\`bash
# Start monitoring
./cerberus-monitoring --config monitoring.json

# Check status
./cerberus-monitoring --status

# View alerts
./cerberus-monitoring --alerts --last 100

# Export metrics
./cerberus-monitoring --export --format json
\`\`\`

### Alert Management
\`\`\`bash
# Create alert
./cerberus-monitoring --create-alert --type SECURITY --level ERROR --source "cowrie" --message "Suspicious login attempt"

# Acknowledge alert
./cerbering-monitoring --acknowledge --alert-id 12345

# Resolve alert
./cerbering-monitoring --resolve --alert-id 12345 --resolution "False positive"
\`\`\`

## Integration with Honeypot Services

### Cowrie Integration
\`\`\`python
# In cowrie configuration
[honeypot]
monitoring_url = "http://localhost:8080/monitoring"
monitoring_api_key = "your-api-key"
\`\`\`

### RTSP Integration
\`\`\`python
# In RTSP configuration
[honeypot]
monitoring_url = "http://localhost:8080/monitoring"
monitoring_api_key = "your-api-key"
\`\`\`

### Web Service Integration
\`\`\`python
# In web service configuration
[honeypot]
monitoring_url = "http://localhost:8080/monitoring"
monitoring_api_key = "your-api-key"
\`\`\`

## API Reference

### REST API Endpoints
- \`GET /api/v1/alerts\` - Get all alerts
- \`GET /api/v1/alerts/{id}\` - Get specific alert
- \`POST /api/v1/alerts\` - Create new alert
- \`PUT /api/v1/alerts/{id}/acknowledge\` - Acknowledge alert
- \`PUT /api/v1/alerts/{id}/resolve\` - Resolve alert
- \`GET /api/v1/metrics\` - Get system metrics
- \`GET /api/v1/status\` - Get monitoring status

### WebSocket API
- \`ws://localhost:8080/monitoring/ws\` - Real-time alerts
- \`ws://localhost:8080/metrics/ws\` - Real-time metrics

## Security Considerations

### Authentication
- **API Keys**: Use strong, unique API keys
- **TLS/SSL**: Encrypt all API communications
- **Rate Limiting**: Implement rate limiting on API endpoints
- **Input Validation**: Validate all API inputs

### Data Protection
- **Encryption**: Encrypt sensitive alert data
- **Access Control**: Implement proper access controls
- **Audit Logging**: Log all monitoring activities

### Privacy
- **Data Minimization**: Collect only necessary data
- **Retention Policies**: Implement data retention policies
- **Anonymization**: Anonymize sensitive information

## Performance Optimization

### Efficient Monitoring
- **Minimal Overhead**: Keep monitoring overhead below 1%
- **Batch Processing**: Batch metric collection
- **Caching**: Cache frequently accessed data
- **Asynchronous Processing**: Use async operations

### Resource Management
- **Memory Management**: Efficient memory usage
- **Connection Pooling**: Reuse network connections
- **Database Optimization**: Optimize database queries

## Troubleshooting

### Common Issues
- **High CPU Usage**: Check monitoring frequency
- **Memory Leaks**: Monitor memory usage trends
- **Network Timeouts**: Check network connectivity
- **Alert Spam**: Check alert correlation rules

### Debug Mode
- **Verbose Logging**: Enable detailed logging
- **Performance Metrics**: Monitor monitoring performance
- **Test Alerts**: Generate test alerts for validation

---

*This guide covers the real-time monitoring and alerting system implementation for the Cerberus Honeypot project.*
