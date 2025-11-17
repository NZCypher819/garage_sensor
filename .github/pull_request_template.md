## Description

**Summary of changes:**
<!-- Briefly describe what this PR does -->

**Related Issue(s):**
<!-- Link to related issues: Fixes #123, Addresses #456 -->

## Type of Change

- [ ] 🐛 Bug fix (non-breaking change that fixes an issue)
- [ ] ✨ New feature (non-breaking change that adds functionality)
- [ ] 💥 Breaking change (fix or feature that would cause existing functionality to change)
- [ ] 🔧 Refactoring (code change that neither fixes a bug nor adds a feature)
- [ ] 📝 Documentation update
- [ ] 🔒 Security fix
- [ ] ⚡ Performance improvement
- [ ] 🧪 Test improvements

## Constitutional Principles Compliance

- [ ] **Reliability First**: Changes maintain or improve system reliability
- [ ] **User Safety**: All operations prioritize safety and fail gracefully
- [ ] **Performance Excellence**: Maintains <100ms response time requirements
- [ ] **Resource Efficiency**: Maintains <10mA idle power consumption
- [ ] **Maintainability**: Code is clean, documented, and testable
- [ ] **Security by Design**: Changes follow secure coding practices

## Testing Checklist

### Unit Tests
- [ ] All existing tests pass
- [ ] New tests added for new functionality
- [ ] Test coverage maintained/improved
- [ ] Edge cases covered

### Integration Tests
- [ ] Cross-component interactions tested
- [ ] Hardware-in-loop tests pass (if applicable)
- [ ] OTA update process tested (if relevant)
- [ ] Power consumption validated

### Hardware Testing
- [ ] Tested on actual ESP32-S3-NANO hardware
- [ ] E3JK-RR11 sensor integration verified
- [ ] LED indicators function correctly
- [ ] Response time <100ms validated

## Security Review

- [ ] No hardcoded credentials or secrets
- [ ] Input validation implemented where needed
- [ ] HTTPS/TLS requirements maintained
- [ ] OTA security not compromised
- [ ] Memory safety considered (buffer overflows, etc.)
- [ ] Error handling prevents information disclosure

## Performance Impact

- [ ] No performance regression introduced
- [ ] Memory usage impact assessed
- [ ] Power consumption impact measured
- [ ] Response time benchmarked

### Performance Metrics
<!-- Fill in actual measurements -->
| Metric | Before | After | Impact |
|--------|--------|-------|--------|
| Response Time | ? ms | ? ms | ? |
| Idle Current | ? mA | ? mA | ? |
| Memory Usage | ? KB | ? KB | ? |
| Flash Usage | ? KB | ? KB | ? |

## Code Quality

- [ ] Code follows project style guidelines
- [ ] Functions are properly documented
- [ ] Variable and function names are descriptive
- [ ] No code duplication
- [ ] Error handling is comprehensive
- [ ] Logging is appropriate and helpful

## Documentation

- [ ] README updated (if needed)
- [ ] API documentation updated (if applicable)
- [ ] Inline code comments added where necessary
- [ ] Architecture documentation updated (if applicable)
- [ ] User-facing changes documented

## Breaking Changes

<!-- If this is a breaking change, describe: -->
<!-- 1. What breaks -->
<!-- 2. Migration path for users -->
<!-- 3. Rationale for the breaking change -->

## Deployment Notes

- [ ] Database migrations (if any)
- [ ] Configuration changes required
- [ ] OTA deployment safe
- [ ] Rollback strategy defined

## Screenshots/Videos

<!-- If UI changes or hardware demonstration, include media -->

## Additional Context

<!-- Any other context about the PR -->

---

### Reviewer Guidelines

**For Reviewers:** Please ensure:
1. ✅ All checklist items are completed
2. ✅ Constitutional principles are upheld
3. ✅ Security implications considered
4. ✅ Performance impact acceptable
5. ✅ Test coverage adequate
6. ✅ Documentation complete

**Approval requires:** 2+ approvals for core changes, 1 approval for documentation/minor fixes

### Merge Requirements

- [ ] All CI checks pass
- [ ] Required approvals obtained
- [ ] No merge conflicts
- [ ] Branch up to date with target
