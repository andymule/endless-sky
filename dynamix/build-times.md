# Build Time Optimization Guide for Dynamix

This document summarizes the most effective ways to reduce build times for the Dynamix project, based on recent profiling and best practices for CMake/Ninja/C++ projects.

## Summary Table

| Optimization                | Estimated Time Savings (Full Build) | Impact Rank | Notes                                    |
|----------------------------|-------------------------------------|-------------|------------------------------------------|
| **ccache**                  | 2x-10x (rebuilds)                   | 1           | Essential for fast rebuilds               |
| **Unity Build Batch Size**  | 10-30%                              | 2           | Tune for your CPU; higher = fewer files   |
| **Precompiled Headers (PCH)**| 10-20%                             | 3           | Especially effective with many includes   |
| **Disable LTO for Dev**     | 5-15% (link step)                   | 4           | LTO is slow; disable for dev builds       |
| **Incremental Builds**      | 10x-100x (after first build)        | 5           | Only rebuilds changed files               |

---

## 1. **ccache** (Rank 1)
- **What:** Caches compiled object files so only changed files are rebuilt.
- **How:**
  - Install: `brew install ccache`
  - Ensure it's in your PATH.
  - CMake auto-detects and uses it if available.
- **Estimated Savings:** 2x-10x faster rebuilds (seconds instead of minutes).
- **Status:** Not currently enabled (`ccache not found`).

## 2. **Unity Build Batch Size** (Rank 2)
- **What:** Combines multiple source files into a single translation unit to reduce compile overhead.
- **How:**
  - Set `CMAKE_UNITY_BUILD_BATCH_SIZE` (e.g., 5, 8, 10).
  - Higher values = fewer, larger files (faster for many small files, but may hit RAM limits).
- **Estimated Savings:** 10-30% faster full builds.
- **Status:** Currently set to 5. Try 8 or 10 for more speed.

## 3. **Precompiled Headers (PCH)** (Rank 3)
- **What:** Compiles common headers once, then reuses them for all source files.
- **How:**
  - Enable with `-DENABLE_PCH=ON` in CMake.
  - Works best if you have many includes in most files.
- **Estimated Savings:** 10-20% faster builds, especially for large codebases.
- **Status:** Supported, but off by default.

## 4. **Disable LTO for Development** (Rank 4)
- **What:** Link Time Optimization (LTO) makes binaries smaller/faster, but slows down linking.
- **How:**
  - Disable for dev builds: `-DENABLE_LTO=OFF`
  - Enable for release builds only.
- **Estimated Savings:** 5-15% faster link step.
- **Status:** Currently enabled by default.

## 5. **Incremental Builds** (Rank 5)
- **What:** Only changed files are rebuilt.
- **How:**
  - Use Ninja or Make; just run `ninja` or `make` after editing.
- **Estimated Savings:** 10x-100x faster than full builds (sub-second for small changes).
- **Status:** Supported and recommended.

---

## Recommendations

1. **Install and enable ccache** for all developers.
2. **Experiment with higher unity batch sizes** (8 or 10) for your hardware.
3. **Enable PCH** for even faster builds, especially if you have many includes.
4. **Disable LTO for development** to speed up linking.
5. **Always use incremental builds** for day-to-day work.

With these optimizations, you can expect full build times to drop from ~30s to under 15s, and incremental builds to be nearly instant. 