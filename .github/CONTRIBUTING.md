# Contributing to CERBERUS Honeypot

First off, thank you for considering contributing to CERBERUS! It's people like you that make this project better for everyone.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [How Can I Contribute?](#how-can-i-contribute)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Commit Guidelines](#commit-guidelines)
- [Pull Request Process](#pull-request-process)
- [Security](#security)

## Code of Conduct

This project and everyone participating in it is governed by our commitment to maintaining a welcoming, inclusive, and harassment-free environment. By participating, you are expected to uphold this standard.

### Our Standards

- Be respectful and inclusive
- Accept constructive criticism gracefully
- Focus on what's best for the community
- Show empathy towards other community members

## Getting Started

Note: We use pre-commit hooks to enforce code style (clang-format), shell lint (shellcheck), and spelling (codespell). Please install and enable them during your local setup so CI passes consistently.

### Prerequisites

- GCC or Clang compiler
- Make build system
- Git
- Docker (optional, for container testing)
- Linux environment (native or WSL)

### Quick Setup

```bash
# Clone the repository
git clone https://github.com/Phantomojo/cerberus-honeypot.git
cd cerberus-honeypot

# (Optional) Install pre-commit and enable hooks
python3 -m pip install --user pre-commit
pre-commit install --install-hooks
# Run hooks once on the whole repo
pre-commit run --all-files || true

# Build the project
make all

# Run tests
make test

# Run specific test
./build/state_engine_test
```

## How Can I Contribute?

### 🐛 Reporting Bugs

Before creating bug reports, please check existing issues to avoid duplicates.

When creating a bug report, include:
- Clear, descriptive title
- Steps to reproduce
- Expected vs actual behavior
- System information (OS, architecture, versions)
- Relevant logs or output

Use the [Bug Report template](.github/ISSUE_TEMPLATE/bug_report.md).

### ✨ Suggesting Features

We love feature suggestions! Before suggesting:
- Check if it's already planned in the roadmap
- Search existing issues for similar suggestions

Use the [Feature Request template](.github/ISSUE_TEMPLATE/feature_request.md).

### 📱 Adding Device Profiles

Device profiles are crucial for CERBERUS. To contribute a new profile:

1. Research the target device thoroughly
2. Gather realistic specifications (CPU, RAM, services, banners)
3. Use the [Device Profile template](.github/ISSUE_TEMPLATE/device_profile.md)
4. Submit as an issue first for discussion, then a PR

### 💻 Code Contributions

#### Good First Issues

Look for issues labeled `good-first-issue` for beginner-friendly tasks.

#### Areas Needing Help

- New device profiles
- Output generators for more commands
- Cowrie integration improvements
- Documentation
- Test coverage

## Development Setup

### Building

```bash
# Standard build
make all

# Debug build with sanitizers
make debug

# Clean build
make clean && make all
```

### Code Quality & Pre-commit

- Formatting: C/C++ is formatted with clang-format (configured via .clang-format). You can auto-format staged files with pre-commit or run clang-format from your IDE.
- Linting:
  - C/C++: clang-tidy and cppcheck run in CI (annotations via review tools).
  - Shell: shellcheck runs on scripts.
  - Spelling: codespell runs on docs and comments.
- Pre-commit hooks:
  - Install: pip install pre-commit; pre-commit install --install-hooks
  - Run locally: pre-commit run --all-files
- CI quality gates:
  - Code Quality workflow checks formatting and annotates lint issues.
  - Security workflows perform static analysis and additional scans.

### Project Structure

```
cerberus-honeypot/
├── src/
│   ├── state/          # State engine (the brain)
│   ├── morph/          # Morphing engine
│   ├── quorum/         # Attack detection
│   ├── network/        # Network simulation
│   ├── filesystem/     # Filesystem generation
│   ├── processes/      # Process simulation
│   ├── behavior/       # Response behavior
│   └── temporal/       # Time management
├── include/            # Header files
├── tests/              # Test files
├── services/           # External services (Cowrie, etc.)
├── docker/             # Docker configuration
└── docs/               # Documentation
```

### Key Components

1. **State Engine** (`src/state/state_engine.c`)
   - Single source of truth
   - All output derives from state
   - Maintains correlation across all fake data

2. **Device Profiles** (in `state_engine.c`)
   - Define fake device characteristics
   - Must be realistic and researched

3. **Output Generators** (in `state_engine.c`)
   - Generate `/proc/*`, `/etc/*`, command outputs
   - Must derive from state, not random values

## Coding Standards

### C Code Style

```c
/* Function comments like this */
int function_name(type_t* param1, int param2) {
    /* Local variable declarations at top */
    int result = 0;

    /* Validate inputs */
    if (!param1) return -1;

    /* Main logic with clear comments */
    for (int i = 0; i < limit; i++) {
        /* Loop body */
    }

    return result;
}
```

### Guidelines

- **Indentation**: 4 spaces (no tabs)
- **Line length**: 100 characters max
- **Braces**: Same line for functions and control structures
- **Naming**:
  - Functions: `snake_case`
  - Types: `snake_case_t`
  - Constants: `UPPER_SNAKE_CASE`
  - Macros: `UPPER_SNAKE_CASE`
- **Comments**: Use `/* */` for C compatibility
- **Error handling**: Check all return values, validate inputs

### Memory Safety

- Always check buffer sizes before string operations
- Use `strncpy` with proper size limits
- Initialize all variables
- Free allocated memory
- Run with AddressSanitizer during development

### The Rubik's Cube Principle

**All generated output MUST derive from state, not random values.**

```c
/* WRONG - Random value */
int uptime = rand() % 86400;

/* RIGHT - Derived from state */
int uptime = time(NULL) - state->boot_time;
```

## Commit Guidelines

### Commit Message Format

```
type(scope): short description

Longer description if needed.

Fixes #123
```

### Types

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `style`: Formatting, no code change
- `refactor`: Code restructuring
- `test`: Adding tests
- `chore`: Maintenance tasks
- `perf`: Performance improvement
- `security`: Security fix

### Examples

```
feat(state): add support for NAS device profiles

Adds Synology and QNAP device profiles with appropriate
process lists, filesystem layouts, and service banners.

Closes #42
```

```
fix(generator): correct memory correlation in meminfo

Memory values in /proc/meminfo now properly sum from
process memory usage instead of random values.

Fixes #37
```

## Pull Request Process

### Before Submitting

1. **Create a branch** from `main`:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes** following coding standards

3. **Test thoroughly**:
   ```bash
   make clean && make all
   make test
   ./build/state_engine_test
   ```

4. **Run with sanitizers**:
   ```bash
   make debug
   ./build/state_engine_test
   ```

5. **Update documentation** if needed

### Submitting

1. Push your branch to your fork
2. Open a Pull Request against `main`
3. Fill out the PR template completely
4. Wait for CI checks to pass
5. Address review feedback

### Review Process

- All PRs require at least one approval
- CI must pass (build, tests, security scans)
- Security-sensitive changes require extra scrutiny
- Be responsive to feedback

### After Merge

- Delete your branch
- Update any related issues
- Celebrate! 🎉

## Security

### Reporting Vulnerabilities

**Do NOT report security vulnerabilities in public issues.**

See [SECURITY.md](SECURITY.md) for reporting instructions.

### Security Considerations

When contributing:
- Never commit secrets, credentials, or API keys
- Be mindful of buffer overflows and memory safety
- Consider escape possibilities in honeypot code
- Review for common C vulnerabilities

## Questions?

- Open a [Discussion](https://github.com/Phantomojo/cerberus-honeypot/discussions) for questions
- Check existing issues and documentation first
- Be patient - maintainers are volunteers

---

Thank you for contributing to CERBERUS! Your efforts help make the internet a safer place by improving our ability to study and understand attacker behavior. 🛡️
