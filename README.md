# trading-bot-module

## Structure

- `src/` — library sources/headers (target: `TradingBotLib`)
- `app/` — executable entry point (target: `TradingBot`)
- `tests/` — unit tests (target: `TradingBotTests`, optional)

## Build

### Default (library + executable)

Use existing script:

`./build.sh`

### Enable unit tests (GoogleTest)

Quick tests build script:

`./build_tests.sh`

