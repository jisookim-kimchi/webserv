# webserv

## Development checks

This repository uses pre-commit to run the C++ clang-based checks locally.

- Install the hook manager: `python3 -m pip install --user pre-commit`
- Install the git hook: `pre-commit install`
- Run formatting checks manually: `pre-commit run clang-format --all-files`
- Run static analysis manually: `pre-commit run clang-tidy --all-files`

The formatting rule is defined in [.clang-format](.clang-format) and is enforced through pre-commit's clang-format hook.
The lint rule is defined in [.clang-tidy](.clang-tidy) and is enforced through pre-commit's clang-tidy hook.

