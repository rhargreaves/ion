#!/usr/bin/env bash
set -euo pipefail

ION_SERVER=ion-server/cmake-build-debug/ion-server
SERVER_HOST=host.docker.internal
SERVER_PORT=8443

cleanup() {
  if [[ -n $ION_PID ]] && kill -0 $ION_PID 2>/dev/null; then
    echo "Terminating ion (PID: $ION_PID)"
    kill -TERM $ION_PID
    sleep 2
    if kill -0 $ION_PID 2>/dev/null; then
      kill -KILL $ION_PID
    fi
  fi
}
trap cleanup EXIT

$ION_SERVER &
ION_PID=$!
docker run -it summerwind/h2spec \
  -h $SERVER_HOST -p $SERVER_PORT -k -t

