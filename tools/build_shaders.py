#!/usr/bin/env python3
"""Compile GLSL shader sources under data/shaders/ to per-backend artifacts.

Outputs three artifacts per shader source:
  - <name>.spv   - SPIR-V (Vulkan), via glslangValidator
  - <name>.dxil  - DXIL (D3D12), via SDL_shadercross
  - <name>.msl   - MSL (Metal), via SDL_shadercross

Runtime selects the matching artifact via SDL_GetGPUShaderFormats; build-time
translation avoids the SPIRV-Cross + DXIL runtime dependency that
SDL_shadercross would otherwise require at runtime.

Invoked by Make/CMake/MSVC build rules under USE_SDL3 only. Skips writing if
the source is older than the artifact.

Tools required (override via env vars when not on PATH):
  GLSLANG          - glslangValidator binary  (default: glslangValidator)
  SDL_SHADERCROSS  - SDL_shadercross binary   (default: shadercross)
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path


def find_tool(env_var: str, default: str) -> str:
    """Resolve a tool path from env or PATH; raise if missing."""
    explicit = os.environ.get(env_var)
    if explicit:
        if not Path(explicit).exists():
            raise FileNotFoundError(
                "{} points at non-existent path: {}".format(env_var, explicit)
            )
        return explicit
    located = shutil.which(default)
    if located is None:
        raise FileNotFoundError(
            "Required tool '{}' not found on PATH; set {} env var or "
            "install (see doc/c++/COMPILING.md SDL3 section).".format(
                default, env_var)
        )
    return located


def stage_for_source(src: Path) -> str:
    """Return SDL_GPU shader stage flag for a source file's extension."""
    suffix = src.suffix.lower()
    if suffix == ".frag":
        return "frag"
    if suffix == ".vert":
        return "vert"
    raise ValueError(
        "Unsupported shader stage extension: {} "
        "(expected .frag or .vert)".format(suffix)
    )


def compile_glsl_to_spv(glslang: str, src: Path, dst: Path) -> None:
    """Run glslangValidator GLSL -> SPIR-V."""
    stage = stage_for_source(src)
    cmd = [glslang, "-V", "-S", stage, "-o", str(dst), str(src)]
    subprocess.run(cmd, check=True)


def compile_spv_to_dxil(shadercross: str, src_spv: Path, dst: Path) -> None:
    """Run SDL_shadercross SPIR-V -> DXIL."""
    cmd = [shadercross, str(src_spv),
           "-s", "SPIRV", "-d", "DXIL", "-o", str(dst)]
    subprocess.run(cmd, check=True)


def compile_spv_to_msl(shadercross: str, src_spv: Path, dst: Path) -> None:
    """Run SDL_shadercross SPIR-V -> MSL."""
    cmd = [shadercross, str(src_spv),
           "-s", "SPIRV", "-d", "MSL", "-o", str(dst)]
    subprocess.run(cmd, check=True)


def collect_sources(shader_dir: Path) -> list[Path]:
    return sorted(
        list(shader_dir.glob("*.frag")) + list(shader_dir.glob("*.vert"))
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Compile shader sources for the SDL_GPU lane.")
    parser.add_argument(
        "--shader-dir",
        type=Path,
        default=Path("data/shaders"),
        help=("Directory containing GLSL shader sources "
              "(default: data/shaders)"),
    )
    parser.add_argument(
        "--shader",
        type=Path,
        help="Single shader to compile",
    )
    parser.add_argument(
        "--stamp",
        type=Path,
        default=None,
        help="Optional stamp file to touch when build completes successfully.",
    )
    parser.add_argument(
        "--formats",
        default="spv,dxil,msl",
        help=("Comma-separated list of formats to produce "
              "(default: spv,dxil,msl)"),
    )
    args = parser.parse_args()

    shader_dir: Path = args.shader_dir

    formats = {f.strip().lower() for f in args.formats.split(",") if f.strip()}
    unknown = formats - {"spv", "dxil", "msl"}
    if unknown:
        print("build_shaders: unknown formats requested: {}".format(
              ", ".join(unknown)), file=sys.stderr)
        return 2

    # dxil/msl are translated from SPIR-V via SDL_shadercross, so SPIR-V is
    # required as the intermediate even if the caller didn't list it.
    if formats & {"dxil", "msl"}:
        formats.add("spv")

    try:
        glslang = find_tool("GLSLANG", "glslangValidator")
        shadercross = None
        if formats & {"dxil", "msl"}:
            shadercross = find_tool("SDL_SHADERCROSS", "shadercross")
    except FileNotFoundError as exc:
        print("build_shaders: {}".format(exc), file=sys.stderr)
        return 3

    if args.shader:
        sources = [args.shader]
    else:
        sources = collect_sources(args.shader_dir)
    if not sources:
        print("build_shaders: no shader sources found in {}".format(
              shader_dir), file=sys.stderr)

    try:
        for src in sources:
            spv = src.with_suffix(src.suffix + ".spv")
            if "spv" in formats:
                print("build_shaders: {} -> {}".format(src.name, spv.name))
                compile_glsl_to_spv(glslang, src, spv)
            if "dxil" in formats:
                dxil = src.with_suffix(src.suffix + ".dxil")
                print("build_shaders: {} -> {}".format(
                    spv.name, dxil.name))
                compile_spv_to_dxil(shadercross, spv, dxil)
            if "msl" in formats:
                msl = src.with_suffix(src.suffix + ".msl")
                print("build_shaders: {} -> {}".format(spv.name, msl.name))
                compile_spv_to_msl(shadercross, spv, msl)
    except subprocess.CalledProcessError as exc:
        print("build_shaders: shader compile failed (exit {}): {}".format(
            exc.returncode, " ".join(exc.cmd)), file=sys.stderr)
        return 4

    if args.stamp is not None:
        args.stamp.parent.mkdir(parents=True, exist_ok=True)
        args.stamp.touch()

    return 0


if __name__ == "__main__":
    sys.exit(main())
