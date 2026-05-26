# РОЗРОБЛЕННЯ ІНТЕЛЕКТУАЛЬНОГО БОТА ДЛЯ ВИСОКОЧАСТОТНОЇ АЛГОРИТМІЧНОЇ ТОРГІВЛІ КРИПТОВАЛЮТАМИ
# Автор: Лапюк Роман Романович
# Науковий керівник: Колдовський В’ячеслав Васильович, Кандидат економічних наук, доцент Університету, керівник компетентностей SoftServe Academy, SoftServe 

## Structure

- `src/` — library sources/headers (target: `TradingBotLib`)
- `app/` — executable entry point (target: `TradingBot`)
- `tests/` — unit tests (target: `TradingBotTests`, optional)
- `insfrastructure/` — AWS deployment

## Build

### Default (library + executable)

Use existing script:

`./build.sh`

### Enable unit tests (GoogleTest)

Quick tests build script:

`./build_tests.sh`

### Deployment

`terraform init`
`terraform plan`
`terraform apply`
