#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
"${MKW_HOST_CXX:-clang++}" -std=c++20 -O1 -g -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -DTARGET_PC -DMKW_LOCAL_FUNCTION_EXECUTION=1 -DMKW_LOCAL_RENDERED_FAST_TRACK=0 \
    -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
    -I"$ROOT_DIR/include" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
    "$ROOT_DIR/tests/execution_cancel_raii_contract.cpp" \
    "$ROOT_DIR/source/gx_set_tev_k_alpha_sel_hle_bridge.cpp" \
    "$ROOT_DIR/source/gx_set_tev_indirect_hle_bridge.cpp" \
    -o "$TEST_DIR/cancel-raii"
"$TEST_DIR/cancel-raii"
"${MKW_HOST_CXX:-clang++}" -std=c++20 -O1 -g -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -DTARGET_PC -DMKW_LOCAL_FUNCTION_EXECUTION=1 -DMKW_LOCAL_RENDERED_FAST_TRACK=0 \
    -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
    -I"$ROOT_DIR/include" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
    "$ROOT_DIR/tests/execution_cancel_fiber_contract.cpp" \
    "$ROOT_DIR/source/switch_guest_fiber.cpp" \
    -o "$TEST_DIR/cancel-fiber"
ASAN_OPTIONS="${ASAN_OPTIONS:+$ASAN_OPTIONS:}detect_stack_use_after_return=1" "$TEST_DIR/cancel-fiber" nested
ASAN_OPTIONS="${ASAN_OPTIONS:+$ASAN_OPTIONS:}detect_stack_use_after_return=1" "$TEST_DIR/cancel-fiber" cycle
ASAN_OPTIONS="${ASAN_OPTIONS:+$ASAN_OPTIONS:}detect_stack_use_after_return=1" "$TEST_DIR/cancel-fiber" callback
"${MKW_HOST_CXX:-clang++}" -std=c++20 -O1 -g -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -D__SWITCH__ \
    -I"$ROOT_DIR/tests/execution-cancel-seams" -I"$ROOT_DIR/include" \
    -c "$ROOT_DIR/source/mkw_loader_exit.cpp" -o "$TEST_DIR/loader-exit.o"
"${MKW_HOST_CXX:-clang++}" -std=c++20 -O1 -g -Wall -Wextra -Werror -Wno-return-type-c-linkage -pthread \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -DTARGET_PC -DMKW_LOCAL_FAST_TRACK=1 -DMKW_LOCAL_RENDERED_FAST_TRACK=1 \
    -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
    -I"$ROOT_DIR/tests/execution-cancel-seams" -I"$ROOT_DIR/include" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
    "$ROOT_DIR/tests/execution_cancel_main_contract.cpp" "$ROOT_DIR/source/mkw_gpu_shutdown_guard.cpp" \
    "$TEST_DIR/loader-exit.o" -o "$TEST_DIR/cancel-main"
"$TEST_DIR/cancel-main" success
"$TEST_DIR/cancel-main" retry
"$TEST_DIR/cancel-main" join-retry
python3 - "$TEST_DIR/cancel-main" <<'PY'
import pathlib
import subprocess
import sys
import tempfile
import time

for mode in ("renderer-failure", "finalizer-failure"):
    with tempfile.TemporaryDirectory() as directory:
        log = pathlib.Path(directory) / "blocked.log"
        with log.open("w") as output:
            process = subprocess.Popen([sys.argv[1], mode], stdout=output, stderr=output)
        try:
            deadline = time.monotonic() + 30
            while "FAILURE_BLOCKED" not in log.read_text():
                assert process.poll() is None, log.read_text()
                assert time.monotonic() < deadline, log.read_text()
                time.sleep(0.02)
            # A single sleep followed by return must fail this contract too.
            time.sleep(0.15)
            assert process.poll() is None
            assert log.read_text().count("FAILURE_BLOCKED") >= 2
            print(f"PASS: actual main/SDK exit hook holds process alive on {mode}; loader return was prevented")
        finally:
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=10)
PY
"${MKW_HOST_CXX:-clang++}" -std=c++20 -O1 -g -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all \
    -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--wrap=fsync \
    -D__SWITCH__ -DTARGET_PC -DMKW_LOCAL_FAST_TRACK=1 -DMKW_DISCOVERY_SCAN_MODE=1 \
    -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
    -I"$ROOT_DIR/tests/diagnostic-seams" -I"$ROOT_DIR/include" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
    "$ROOT_DIR/source/fast_track_crash_diagnostics.cpp" \
    "$ROOT_DIR/tests/execution_cancel_reporter_contract.cpp" -o "$TEST_DIR/cancel-reporter"
mkdir "$TEST_DIR/reporter"
(cd "$TEST_DIR/reporter" && ../cancel-reporter)
bash "$ROOT_DIR/scripts/test-execution-cancel-provider.sh"
bash "$ROOT_DIR/scripts/test-gpu-shutdown-guard.sh"
