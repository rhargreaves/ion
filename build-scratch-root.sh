#!/usr/bin/env bash

SCRATCH_ROOT=${BUILD_DIR}/scratch-root

rm -rf "${SCRATCH_ROOT}"
mkdir -p "${SCRATCH_ROOT}/etc" \
  "${SCRATCH_ROOT}/lib" \
  "${SCRATCH_ROOT}/usr/lib" \
  "${SCRATCH_ROOT}/app"
ldd "${BUILD_DIR}/app/ion-server" | grep "=>" \
    | awk '{print $3}' \
    | xargs -r cp -L --parents -t "${SCRATCH_ROOT}/"
ldd "${BUILD_DIR}/app/ion-server" | grep -v "=>" \
    | grep -v "linux-vdso" \
    | awk '{print $1}' \
    | xargs -r cp -L --parents -t "${SCRATCH_ROOT}/"
cp /etc/ssl/certs/ca-certificates.crt "${SCRATCH_ROOT}/etc/"
echo "ion:x:1000:1000:ion:/app:/usr/sbin/nologin" > "${SCRATCH_ROOT}/etc/passwd"
echo "ion:x:1000:" > "${SCRATCH_ROOT}/etc/group"
cp "${BUILD_DIR}/app/ion-server" "${SCRATCH_ROOT}/app/ion-server"
