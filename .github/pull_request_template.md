## Description

<!-- Provide a brief description of the changes in this PR -->

## Type of Change

<!-- Mark the relevant option with an [x] -->

- [ ] 🐛 Bug fix (non-breaking change that fixes an issue)
- [ ] ✨ New feature (non-breaking change that adds functionality)
- [ ] 💥 Breaking change (fix or feature that would cause existing functionality to change)
- [ ] 📝 Documentation update
- [ ] 🔧 Configuration change
- [ ] 🧪 Test update
- [ ] 🔒 Security fix
- [ ] ♻️ Refactoring (no functional changes)

## Related Issues

<!-- Link any related issues here using #issue_number -->

Closes #

## Changes Made

<!-- List the specific changes made in this PR -->

-
-
-

## Testing

<!-- Describe how you tested these changes -->

### Tests Run

- [ ] `make test` passes
- [ ] `./build/state_engine_test` passes
- [ ] Manual testing performed

### Test Details

<!-- Describe any specific testing performed -->

```
# Add test output or commands run here
```

## Checklist

<!-- Ensure all items are checked before requesting review -->

- [ ] My code follows the project's coding style
- [ ] I have commented my code, particularly in hard-to-understand areas
- [ ] I have updated the documentation accordingly
- [ ] My changes generate no new warnings
- [ ] I have added tests that prove my fix/feature works
- [ ] New and existing tests pass locally
- [ ] Any dependent changes have been merged and published

## Security Considerations

<!-- For security-sensitive changes, describe the security implications -->

- [ ] This change does not introduce new attack vectors
- [ ] This change does not expose sensitive information
- [ ] This change has been reviewed for common vulnerabilities (buffer overflow, injection, etc.)

## Screenshots/Output

<!-- If applicable, add screenshots or command output to demonstrate the changes -->

## Additional Notes

<!-- Any additional information that reviewers should know -->

---

**Reviewer Guidelines:**
- Check for memory safety issues (buffer overflows, use-after-free)
- Verify correlation logic in state engine changes
- Ensure new device profiles are realistic
- Test morphing behavior if state engine is modified
