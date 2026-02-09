# OpenTelemetry Stack

Demonstrates running ion with OpenTelemetry & Jaeger.

## Build

The ion docker image needs to be built:

```
make build-ion
```

## ion + Jaeger

1. `make up`
2. Make some HTTP requests to https://localhost:8443/
3. Visit http://localhost:16686/ to view traces in Jaeger

## ion + Tempo + Grafana

1. `make up-grafana`
2. Make some HTTP requests to https://localhost:8443/
3. Visit http://localhost:3000/ to view traces in Grafana (Tempo as the datasource)

## Teardown

```
make clean
```
