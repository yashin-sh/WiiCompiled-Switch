"""Audited runtime repairs for files introduced by the pinned Deko3D patch.

Accept complete original or repaired files, including during dependency reuse.
The pinned reference remains untouched. Host contracts compile the actual
repaired methods; they do not substitute for execution on the Switch GPU.
"""

import argparse
import os
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

PREFIX = Path("src/dawn/native/deko3d")
TARGETS = tuple(
    PREFIX / name
    for name in ("DeviceDeko3d.cpp", "DeviceDeko3d.h", "CommandBufferDeko3d.cpp")
)
BASE_PATCH = (
    Path(__file__).resolve().parents[1]
    / "third_party/DriftDroid/aurora-main/cmake/patches/dawn-nintendo-switch-platform.patch"
)

UPLOAD_OLD = """    const uint64_t blocksPerRow = layout.bytesPerRow / block.byteSize;

    for (uint32_t layer = 0; layer < layerCount; ++layer) {
        const uint64_t layerOffset = layout.offset + bytesPerImage * layer;
        const uint64_t byteSize = is3D ? bytesPerImage * imagesPerCopy : bytesPerImage;
        uint8_t* upload = nullptr;
        DkGpuAddr uploadAddr = 0;
        DAWN_TRY(AllocateUploadMemory(byteSize, &upload, &uploadAddr));
        std::memcpy(upload, data + layerOffset, byteSize);
"""
UPLOAD_NEW = """    const uint64_t rowBytes =
        ((uint64_t(copySizePixels.width) + block.width - 1) / block.width) * block.byteSize;
    const uint64_t packedImageBytes = rowBytes * rowsPerImage;

    for (uint32_t layer = 0; layer < layerCount; ++layer) {
        const uint64_t layerOffset = layout.offset + bytesPerImage * layer;
        const uint64_t byteSize = packedImageBytes * imagesPerCopy;
        uint8_t* upload = nullptr;
        DkGpuAddr uploadAddr = 0;
        DAWN_TRY(AllocateUploadMemory(byteSize, &upload, &uploadAddr));
        // WebGPU does not require padding after the final copied row/image.
        // Repack only the texel blocks; never read the caller's trailing padding.
        for (uint32_t image = 0; image < imagesPerCopy; ++image) {
            for (uint64_t row = 0; row < rowsPerImage; ++row) {
                std::memcpy(upload + image * packedImageBytes + row * rowBytes,
                            data + layerOffset + image * bytesPerImage + row * layout.bytesPerRow,
                            rowBytes);
            }
        }
"""
COPY_OLD = """        DkCopyBuf copyBuf = {uploadAddr, static_cast<uint32_t>(blocksPerRow * block.byteSize),
                             static_cast<uint32_t>(bytesPerImage)};
"""
COPY_NEW = """        DkCopyBuf copyBuf = {uploadAddr, static_cast<uint32_t>(rowBytes),
                             static_cast<uint32_t>(packedImageBytes)};
"""
RELEASE_OLD = """    mFree{kind}Descriptors.push_back(id);
"""
RELEASE_NEW = """    // CPU descriptor writes must not replace an entry referenced by pending
    // or submitted GPU work. Retire it behind the next submission's fence.
    if (!mHasPendingCommands && mInFlight.empty()) {{
        mFree{kind}Descriptors.push_back(id);
    }} else {{
        mPendingReleased{kind}Descriptors.push_back(id);
    }}
"""
RECLAIM = """        {
            std::lock_guard<std::mutex> lock(mDescriptorMutex);
            mFreeImageDescriptors.insert(mFreeImageDescriptors.end(),
                                         done->releasedImageDescriptors.begin(),
                                         done->releasedImageDescriptors.end());
            mFreeSamplerDescriptors.insert(mFreeSamplerDescriptors.end(),
                                           done->releasedSamplerDescriptors.begin(),
                                           done->releasedSamplerDescriptors.end());
        }
"""
RETIRE = """    {
        std::lock_guard<std::mutex> lock(mDescriptorMutex);
        inFlight->releasedImageDescriptors = std::move(mPendingReleasedImageDescriptors);
        inFlight->releasedSamplerDescriptors = std::move(mPendingReleasedSamplerDescriptors);
        mPendingReleasedImageDescriptors.clear();
        mPendingReleasedSamplerDescriptors.clear();
    }
"""
COMPATIBLE = """    bool AreLayoutsCompatible() override {
        // Combined image/sampler units depend on shader reflection, even when
        // two pipelines share the same WebGPU layout. Rebind on shader changes.
        return mLastAppliedPipeline == mPipeline && BindGroupTrackerBase::AreLayoutsCompatible();
    }

"""
BUFFER_COPY_OLD = """                // The source's CPU view reflects every write recorded before this point only if it
                // is not written by GPU work in this same pass; Dawn's copy sources are mapped or
                // staging buffers, so snapshot them here.
                device->RecordBufferWrite(destination->GetGpuAddr() + copy->destinationOffset,
                                          source->GetCpuAddr() + copy->sourceOffset, copy->size);
"""
BUFFER_COPY_NEW = """                if (source->GetUsage() ==
                    (wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc)) {
                    // This frontend-restricted staging usage cannot be written by the GPU.
                    // Retain the fast snapshot path for per-frame vertex/uniform uploads.
                    device->RecordBufferWrite(destination->GetGpuAddr() + copy->destinationOffset,
                                              source->GetCpuAddr() + copy->sourceOffset, copy->size);
                } else {
                    // Depth Peek copies a compute-written storage buffer in this same list.
                    // Its CPU contents are still stale while recording: copy on the GPU,
                    // after all producers finish and flush their dirty cache lines.
                    dkCmdBufBarrier(cmdBuf, DkBarrier_Primitives, DkInvalidateFlags_L2Cache);
                    dkCmdBufCopyBuffer(cmdBuf, source->GetGpuAddr() + copy->sourceOffset,
                                       destination->GetGpuAddr() + copy->destinationOffset,
                                       static_cast<uint32_t>(copy->size));
                    dkCmdBufBarrier(cmdBuf, DkBarrier_Full,
                                    DkInvalidateFlags_Shader | DkInvalidateFlags_L2Cache);
                }
"""


def replace_once(text, before, after):
    if text.count(before) != 1:
        raise ValueError(f"Deko3D runtime source anchor changed: {before[:100]!r}")
    return text.replace(before, after, 1)


def base_source(patch, target):
    marker = f"diff --git a/{target} b/{target}\n"
    text = patch.read_text()
    if text.count(marker) != 1:
        raise ValueError(f"missing unique pinned new-file patch: {target}")
    section = text.split(marker, 1)[1].split("\ndiff --git ", 1)[0]
    match = re.search(r"^@@ -0,0 \+1,(\d+) @@\n", section, re.MULTILINE)
    if match is None:
        raise ValueError(f"expected complete new-file hunk: {target}")
    lines = section[match.end() :].splitlines()
    if len(lines) != int(match[1]) or any(not line.startswith("+") for line in lines):
        raise ValueError(f"pinned new-file hunk changed: {target}")
    return "".join(line[1:] + "\n" for line in lines)


def changes(patch=BASE_PATCH, gpu_buffer_copy=True):
    originals = {target: base_source(patch, target) for target in TARGETS}
    fixed = dict(originals)
    cpp, header, commands = TARGETS
    fixed[cpp] = replace_once(fixed[cpp], UPLOAD_OLD, UPLOAD_NEW)
    fixed[cpp] = replace_once(fixed[cpp], COPY_OLD, COPY_NEW)
    fixed[cpp] = replace_once(fixed[cpp], "        (void)rowsPerImage;\n", "")
    for kind in ("Image", "Sampler"):
        old = (
            f"""void Device::Release{kind}Descriptor(uint32_t id) {{
    std::lock_guard<std::mutex> lock(mDescriptorMutex);
"""
            + RELEASE_OLD.format(kind=kind)
            + "}\n"
        )
        new = old.replace(RELEASE_OLD.format(kind=kind), RELEASE_NEW.format(kind=kind))
        fixed[cpp] = replace_once(fixed[cpp], old, new)
    fixed[cpp] = replace_once(
        fixed[cpp],
        "    inFlight->uploadMemory = std::move(mPendingUploadMemory);\n",
        RETIRE + "    inFlight->uploadMemory = std::move(mPendingUploadMemory);\n",
    )
    fixed[cpp] = replace_once(
        fixed[cpp],
        "        mCompletedSerial = std::max(mCompletedSerial, done->serial);\n",
        RECLAIM
        + "        mCompletedSerial = std::max(mCompletedSerial, done->serial);\n",
    )
    fixed[header] = replace_once(
        fixed[header],
        "        DkFence fence;\n",
        "        std::vector<uint32_t> releasedImageDescriptors;\n        std::vector<uint32_t> releasedSamplerDescriptors;\n        DkFence fence;\n",
    )
    fixed[header] = replace_once(
        fixed[header],
        "    std::mutex mDescriptorMutex;\n",
        "    std::vector<uint32_t> mPendingReleasedImageDescriptors;\n    std::vector<uint32_t> mPendingReleasedSamplerDescriptors;\n    std::mutex mDescriptorMutex;\n",
    )
    fixed[commands] = replace_once(
        fixed[commands],
        "  private:\n    void ResetUnits() {\n",
        "  private:\n" + COMPATIBLE + "    void ResetUnits() {\n",
    )
    fixed[commands] = replace_once(
        fixed[commands],
        "        AfterApply();\n",
        "        AfterApply();\n        mLastAppliedPipeline = mPipeline;\n",
    )
    fixed[commands] = replace_once(
        fixed[commands],
        "    PipelineDeko3d* mPipeline = nullptr;\n",
        "    PipelineDeko3d* mPipeline = nullptr;\n    PipelineDeko3d* mLastAppliedPipeline = nullptr;\n",
    )
    fixed[commands] = replace_once(
        fixed[commands],
        "        const uint32_t count = mDirtyUnitsEnd - mDirtyUnitsBegin;\n",
        "        // Descriptor slots can be reused after an older fence completes.\n        dkCmdBufBarrier(mCmdBuf, DkBarrier_None, DkInvalidateFlags_Descriptors);\n        const uint32_t count = mDirtyUnitsEnd - mDirtyUnitsBegin;\n",
    )
    if gpu_buffer_copy:
        fixed[commands] = replace_once(
            fixed[commands], BUFFER_COPY_OLD, BUFFER_COPY_NEW
        )
    return {target: (originals[target], fixed[target]) for target in TARGETS}


def verify_sources(root, patch=BASE_PATCH, repaired_only=False):
    previous = changes(patch, gpu_buffer_copy=False)
    for target, (original, fixed) in changes(patch).items():
        actual = (root / target).read_text()
        if actual not in (
            (fixed,) if repaired_only else (original, previous[target][1], fixed)
        ):
            raise ValueError(f"unattributed Deko3D runtime source: {target}")


def apply(root, check_only=False):
    delta = changes()
    verify_sources(root, repaired_only=check_only)
    if not check_only:
        for target, (_, fixed) in delta.items():
            if (root / target).read_text() != fixed:
                (root / target).write_text(fixed)


def method(text, signature):
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end] + "\n"


def self_test():
    compiler = shutil.which("c++")
    if compiler is None:
        raise ValueError("runtime contracts require a host C++ compiler")
    delta = changes()
    repo = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix="deko3d-runtime-") as directory:
        root = Path(directory)
        for target, (original, _) in delta.items():
            path = root / target
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(original)
        apply(root)
        apply(root)
        apply(root, check_only=True)
        # Upgrade only the complete previously audited runtime variant.
        for target, (_, previous) in changes(gpu_buffer_copy=False).items():
            (root / target).write_text(previous)
        apply(root)
        apply(root, check_only=True)
        target = root / TARGETS[0]
        target.write_text(target.read_text() + "// unauthorized edit\n")
        try:
            apply(root)
        except ValueError:
            pass
        else:
            raise AssertionError("unexpected complete-file edit accepted")
        cpp = delta[TARGETS[0]][1]
        commands = delta[TARGETS[2]][1]
        production = (
            method(cpp, "MaybeError Device::RecordTextureUpload(")
            + method(cpp, "void Device::ReleaseImageDescriptor(")
            + method(cpp, "void Device::ReleaseSamplerDescriptor(")
            + "void Device::Retire(InFlight* inFlight) {\n"
            + RETIRE
            + "}\n"
            + "void Device::Reclaim(InFlight* done) {\n"
            + RECLAIM
            + "}\n"
            + "bool Tracker::AreLayoutsCompatible() {\n"
            + method(commands, "    bool AreLayoutsCompatible() override").split(
                "{", 1
            )[1]
        )
        executable = root / "contract"
        command = [
            compiler,
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-I",
            str(root),
            str(repo / "tests/deko3d_runtime_contract.cpp"),
            "-o",
            str(executable),
        ]
        environment = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}
        # Prove this is a regression test: the exact old upload method must
        # fail with an out-of-bounds read, before any output-layout assertion.
        old_upload = method(
            delta[TARGETS[0]][0], "MaybeError Device::RecordTextureUpload("
        )
        new_upload = method(cpp, "MaybeError Device::RecordTextureUpload(")
        (root / "production.inc").write_text(
            production.replace(new_upload, old_upload, 1)
        )
        subprocess.run(command, check=True)
        baseline = subprocess.run(
            [str(executable)],
            capture_output=True,
            text=True,
            check=False,
            env=environment,
        )
        if (
            baseline.returncode == 0
            or "AddressSanitizer: heap-buffer-overflow" not in baseline.stderr
        ):
            raise AssertionError(
                "baseline did not reproduce the texture source overread: "
                + baseline.stderr
            )
        print("Pinned baseline reproduces AddressSanitizer heap-buffer-overflow")
        (root / "production.inc").write_text(production)
        subprocess.run(command, check=True)
        subprocess.run([str(executable)], check=True, env=environment)
        copy_case = commands[
            commands.index(
                "            case Command::CopyBufferToBuffer: {"
            ) : commands.index("            case Command::CopyBufferToTexture: {")
        ]
        copy_production = (
            "MaybeError RecordCopy(Device* device, CopyBufferToBufferCmd* input) {\n"
            "    Iterator mCommands{input}; [[maybe_unused]] auto* cmdBuf = device;\n"
            "    switch (Command::CopyBufferToBuffer) {\n"
            + copy_case
            + "default: assert(false); }\n    return {};\n}\n"
        )
        copy_command = [
            compiler,
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-I",
            str(root),
            str(repo / "tests/deko3d_buffer_copy_contract.cpp"),
            "-o",
            str(executable),
        ]
        (root / "buffer-copy-production.inc").write_text(
            copy_production.replace(BUFFER_COPY_NEW, BUFFER_COPY_OLD, 1)
        )
        subprocess.run(copy_command, check=True)
        baseline = subprocess.run(
            [str(executable)], capture_output=True, text=True, check=False
        )
        if (
            baseline.returncode == 0
            or "stale GPU-produced buffer" not in baseline.stderr
        ):
            raise AssertionError(
                "baseline buffer copy did not reproduce stale data: " + baseline.stderr
            )
        print("Pinned baseline reproduces stale GPU-produced buffer copy")
        (root / "buffer-copy-production.inc").write_text(copy_production)
        subprocess.run(copy_command, check=True)
        subprocess.run([str(executable)], check=True)
    print("Deko3D runtime memory/binding contracts OK")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path, nargs="?")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    elif args.root:
        apply(args.root, check_only=args.check)
    else:
        parser.error("supply a Dawn root or --self-test")
