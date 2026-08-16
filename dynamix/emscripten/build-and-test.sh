#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DYNAMIX_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
EMSDK_DIR="${DYNAMIX_ROOT}/.emsdk"
BUILD_DIR="${SCRIPT_DIR}/build"

ensure_emsdk() {
    if command -v emcc >/dev/null 2>&1; then
        return 0
    fi

    if [[ ! -d "${EMSDK_DIR}" ]]; then
        echo "Installing Emscripten SDK to ${EMSDK_DIR}..."
        git clone --depth 1 https://github.com/emscripten-core/emsdk.git "${EMSDK_DIR}"
    fi

    pushd "${EMSDK_DIR}" >/dev/null
    ./emsdk install 3.1.64
    ./emsdk activate 3.1.64
    # shellcheck disable=SC1091
    source ./emsdk_env.sh
    popd >/dev/null
}

main() {
    ensure_emsdk

    if ! command -v emcc >/dev/null 2>&1; then
        # shellcheck disable=SC1091
        source "${EMSDK_DIR}/emsdk_env.sh"
    fi

    echo "Using Emscripten: $(emcc --version | head -n1)"

    rm -rf "${BUILD_DIR}"
    emcmake cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
    cmake --build "${BUILD_DIR}" --target spike -j"$(nproc)"

    echo "Running spike under Node..."
    node "${BUILD_DIR}/spike.js"
    echo "Emscripten spike succeeded."
}

main "$@"
