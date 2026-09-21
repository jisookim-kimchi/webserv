# Config & HTTP request parsing stress fixtures

## Layout

- `tests/fixtures/configs/edge/` — valid but awkward configs (whitespace, density, units, empty)
- `tests/fixtures/configs/invalid/` — wrong / broken files (unknown directives, unclosed braces, prose, binary, truncated)
- Large datasets are generated at test time (`generated_large.config`, gitignored)

## Run

```bash
./test.sh config
./test.sh http_request
./test.sh          # config + cgi + http_request
```

Invalid configs are exercised in a forked child because `ConfigParser` calls `exit(1)` on many errors.
