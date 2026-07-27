#!/usr/bin/env python3
"""Reconstruct and validate the release's exact runtime OBJ model archive."""

from __future__ import annotations

import argparse
import base64
import hashlib
import io
import shutil
import tarfile
from pathlib import Path, PurePosixPath

ARCHIVE_SHA256 = "5026d93ee4514264da5948c7c5b663e0f2890de2f7e0cdc2bbfaf2da8ac5c4fd"
PART_DIRECTORY = Path("assets/default_pack/model_bundle_4_6_7")
PART_PATTERN = "models.tar.xz.part*.b64"
EXPECTED_PART_COUNT = 9
MODEL_PREFIX = PurePosixPath("assets/default_pack/models")

REQUIRED_MODELS = (
    "assets/default_pack/models/architecture/bridge.obj",
    "assets/default_pack/models/architecture/cabin.obj",
    "assets/default_pack/models/characters/humanoid_static.obj",
    "assets/default_pack/models/diagnostics/hard_edges.obj",
    "assets/default_pack/models/diagnostics/smooth_edges.obj",
    "assets/default_pack/models/diagnostics/textured_atlas_cube.obj",
    "assets/default_pack/models/diagnostics/vertex_colors.obj",
    "assets/default_pack/models/nature/rock_cluster.obj",
    "assets/default_pack/models/nature/tree_oak.obj",
    "assets/default_pack/models/nature/tree_pine.obj",
    "assets/default_pack/models/primitives/box.obj",
    "assets/default_pack/models/primitives/capsule.obj",
    "assets/default_pack/models/primitives/cone.obj",
    "assets/default_pack/models/primitives/cylinder.obj",
    "assets/default_pack/models/primitives/plane.obj",
    "assets/default_pack/models/primitives/pyramid.obj",
    "assets/default_pack/models/primitives/sphere_ico.obj",
    "assets/default_pack/models/primitives/sphere_uv.obj",
    "assets/default_pack/models/primitives/torus.obj",
    "assets/default_pack/models/primitives/wedge.obj",
    "assets/default_pack/models/props/barrel.obj",
    "assets/default_pack/models/props/crate.obj",
    "assets/default_pack/models/props/lamp_post.obj",
    "assets/default_pack/models/terrain/terrain_32m.obj",
)


def _parts(source_root: Path) -> list[Path]:
    parts = sorted((source_root / PART_DIRECTORY).glob(PART_PATTERN))
    if len(parts) != EXPECTED_PART_COUNT:
        raise RuntimeError(
            f"expected {EXPECTED_PART_COUNT} model archive parts in "
            f"{source_root / PART_DIRECTORY}, found {len(parts)}"
        )
    expected_names = [f"models.tar.xz.part{index:02d}.b64" for index in range(EXPECTED_PART_COUNT)]
    actual_names = [part.name for part in parts]
    if actual_names != expected_names:
        raise RuntimeError(f"model archive parts are not contiguous: {actual_names}")
    return parts


def decode_archive(source_root: Path) -> bytes:
    encoded = "".join(part.read_text(encoding="ascii").strip() for part in _parts(source_root))
    try:
        archive = base64.b64decode(encoded, validate=True)
    except ValueError as error:
        raise RuntimeError(f"model archive base64 is invalid: {error}") from error
    digest = hashlib.sha256(archive).hexdigest()
    if digest != ARCHIVE_SHA256:
        raise RuntimeError(
            f"model archive checksum mismatch: expected {ARCHIVE_SHA256}, got {digest}"
        )
    return archive


def inspect_archive(archive: bytes) -> set[str]:
    names: set[str] = set()
    with tarfile.open(fileobj=io.BytesIO(archive), mode="r:xz") as package:
        for member in package.getmembers():
            path = PurePosixPath(member.name)
            if path.is_absolute() or ".." in path.parts:
                raise RuntimeError(f"unsafe model archive path: {member.name}")
            if not (member.isdir() or member.isfile()):
                raise RuntimeError(f"unsupported model archive member: {member.name}")
            if member.isfile():
                if path.suffix.lower() != ".obj" or not path.is_relative_to(MODEL_PREFIX):
                    raise RuntimeError(f"unexpected model archive file: {member.name}")
                names.add(path.as_posix())
    missing = sorted(set(REQUIRED_MODELS) - names)
    if missing:
        raise RuntimeError("model archive is missing required assets: " + ", ".join(missing))
    return names


def extract_archive(archive: bytes, destination: Path) -> set[str]:
    names = inspect_archive(archive)
    if destination.exists():
        shutil.rmtree(destination)
    destination.mkdir(parents=True, exist_ok=True)
    with tarfile.open(fileobj=io.BytesIO(archive), mode="r:xz") as package:
        for member in package.getmembers():
            if not member.isfile():
                continue
            relative = PurePosixPath(member.name)
            output = destination.joinpath(*relative.parts)
            output.parent.mkdir(parents=True, exist_ok=True)
            source = package.extractfile(member)
            if source is None:
                raise RuntimeError(f"could not read model archive member: {member.name}")
            with source, output.open("wb") as target:
                shutil.copyfileobj(source, target)
    return names


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--destination", type=Path)
    parser.add_argument("--check-only", action="store_true")
    args = parser.parse_args()

    source_root = args.source_root.resolve()
    archive = decode_archive(source_root)
    names = inspect_archive(archive)
    if args.destination is not None and not args.check_only:
        extract_archive(archive, args.destination.resolve())
    print(f"validated exact model archive: {len(names)} OBJ files, sha256={ARCHIVE_SHA256}")


if __name__ == "__main__":
    main()
