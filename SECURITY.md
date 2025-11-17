# Security Policy

## Supported Versions

We actively maintain security updates for the following firmware versions:

| Version | Supported          | OTA Updates |
| ------- | ------------------ | ----------- |
| 1.x.x   | :white_check_mark: | :white_check_mark: |
| 0.x.x   | :x: (EOL)          | :x: |

## Security Features

### Firmware Security
- **Secure Boot**: ESP32-S3 secure boot validation
- **OTA Integrity**: SHA256 cryptographic validation
- **Rollback Protection**: Automatic recovery from failed updates
- **HTTPS Only**: All network communication encrypted
- **Certificate Validation**: Root CA certificate pinning

### Network Security
- **TLS 1.2+**: Modern encryption standards
- **Certificate Pinning**: DigiCert root CA validation
- **API Authentication**: Secure device authentication
- **Rate Limiting**: Protection against abuse

### Physical Security
- **Secure Storage**: Credentials encrypted in NVS
- **Debug Protection**: Production builds disable debug ports
- **Tamper Detection**: Physical access monitoring

## Reporting a Vulnerability

**Please do not report security vulnerabilities through public GitHub issues.**

Instead, please send an email to: **security@your-domain.com**

Include the following information:
- Description of the vulnerability
- Steps to reproduce the issue
- Potential impact assessment
- Suggested remediation (if any)

### What to Expect

1. **Acknowledgment**: Within 48 hours
2. **Initial Assessment**: Within 1 week
3. **Fix Development**: 2-4 weeks (depending on severity)
4. **Security Release**: Coordinated disclosure
5. **Public Disclosure**: After fix is widely deployed

### Severity Classification

| Severity | Description | Response Time |
|----------|-------------|---------------|
| **Critical** | Remote code execution, device takeover | 24-48 hours |
| **High** | Privilege escalation, data compromise | 1 week |
| **Medium** | Information disclosure, DoS | 2-4 weeks |
| **Low** | Configuration issues, minor leaks | 4-8 weeks |

## Security Update Process

### Automatic Updates
1. Security fixes are automatically built and tested
2. OTA updates are pushed to devices within 24 hours
3. Critical security updates bypass normal approval process
4. Rollback capability ensures safe deployment

### Manual Updates
For air-gapped or restricted environments:
1. Download security release from GitHub
2. Verify SHA256 checksums
3. Flash via USB or local OTA server
4. Validate deployment success

## Security Best Practices

### Deployment
- ✅ Use latest firmware version
- ✅ Enable automatic security updates
- ✅ Secure WiFi network (WPA3 recommended)
- ✅ Regular security monitoring
- ✅ Physical access controls

### Network Configuration
- ✅ Firewall protection for device subnet
- ✅ VPN access for remote management
- ✅ Network monitoring and logging
- ✅ Regular WiFi credential rotation

### Monitoring
- ✅ Enable security event logging
- ✅ Monitor for unusual network activity
- ✅ Regular health check validation
- ✅ Audit device configuration changes

## Incident Response

### In Case of Compromise
1. **Immediate**: Disconnect affected devices
2. **Assessment**: Determine scope and impact
3. **Containment**: Isolate and secure environment
4. **Recovery**: Factory reset and re-deploy
5. **Analysis**: Root cause investigation
6. **Prevention**: Update security measures

### Emergency Contacts
- **Security Team**: security@your-domain.com
- **Emergency**: +1-XXX-XXX-XXXX (24/7)
- **Support**: support@your-domain.com

## Security Disclosure Timeline

### Responsible Disclosure
- **T+0**: Vulnerability reported
- **T+48h**: Acknowledgment and initial triage
- **T+1w**: Detailed assessment and impact analysis
- **T+2-4w**: Fix development and testing
- **T+release**: Security patch released
- **T+30d**: Public disclosure (after deployment)

### Bug Bounty (Future)
We are considering a bug bounty program for security researchers. Stay tuned for updates.

## Compliance & Standards

- **NIST Cybersecurity Framework**: Core implementation
- **OWASP IoT Security**: Top 10 mitigation
- **ESP32 Security Best Practices**: Espressif guidelines
- **GitHub Security Advisories**: Automated vulnerability scanning

---

**Security is a shared responsibility. Help us keep the garage sensor ecosystem secure!**

Last updated: November 18, 2025
