#!/usr/bin/env python3
"""Validate compiled scene defaults against the editable V5 cfg."""

from __future__ import annotations

import re
import shlex
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / "assets/editor/default_scene.cfg"
SCENE_INC = ROOT / "src/epoch/render/world/hardcoded_scene_defaults.inc"
MATERIAL_INC = ROOT / "src/epoch/render/world/hardcoded_material_defaults.inc"

ENTRY_RE = re.compile(r"HardcodedDefaultEntry\s*\{\s*(\d+)u,\s*\"([^\"]+)\"", re.S)
MATERIAL_RE = re.compile(r"HardcodedMaterialEntry\s*\{\s*(\d+)u,\s*\"([^\"]+)\"", re.S)


def fail(message: str) -> None:
    print(f"error: {message}", file=sys.stderr)
    raise SystemExit(1)


def parse_cfg() -> list[tuple[int, str, int, float]]:
    lines = CFG.read_text(encoding="utf-8").splitlines()
    if not lines or lines[0].strip() != "EPOCH_SCENE_EDITOR_V5":
        fail("default_scene.cfg is not EPOCH_SCENE_EDITOR_V5")

    result: list[tuple[int, str, int, float]] = []
    for line_number, line in enumerate(lines[1:], start=2):
        if not line.strip():
            continue
        fields = shlex.split(line)
        if len(fields) != 31:
            fail(f"cfg line {line_number} has {len(fields)} fields; expected 31")
        try:
            result.append((int(fields[0]), fields[1], int(fields[16]), float(fields[25])))
        except ValueError as error:
            fail(f"cfg line {line_number} contains an invalid numeric field: {error}")
    return result


def parse_entries(path: Path, pattern: re.Pattern[str]) -> list[tuple[int, str]]:
    return [(int(index), name) for index, name in pattern.findall(path.read_text(encoding="utf-8"))]


def validate_sequence(label: str, entries: list[tuple[int, str]]) -> None:
    for expected, (actual, _) in enumerate(entries):
        if actual != expected:
            fail(f"{label} index {actual} is out of sequence; expected {expected}")


def main() -> None:
    cfg = parse_cfg()
    scene = parse_entries(SCENE_INC, ENTRY_RE)
    materials = parse_entries(MATERIAL_INC, MATERIAL_RE)

    validate_sequence("cfg", [(index, name) for index, name, _, _ in cfg])
    validate_sequence("scene table", scene)
    validate_sequence("material table", materials)

    if len(cfg) != len(scene) or len(cfg) != len(materials):
        fail(f"record-count mismatch: cfg={len(cfg)}, scene={len(scene)}, materials={len(materials)}")

    for index, ((_, cfg_name, deleted, uv_scale), (_, scene_name), (_, material_name)) in enumerate(
        zip(cfg, scene, materials, strict=True)
    ):
        if cfg_name != scene_name or cfg_name != material_name:
            fail(
                f"name mismatch at {index}: cfg={cfg_name!r}, scene={scene_name!r}, "
                f"material={material_name!r}"
            )
        if deleted != 0:
            fail(f"factory record {index} ({cfg_name}) is marked deleted")
        if abs(uv_scale - 1.0) > 1.0e-6:
            fail(f"factory record {index} ({cfg_name}) has UV scale {uv_scale}, expected 1.0")

    print(f"validated {len(cfg)} contiguous factory records")


if __name__ == "__main__":
    main()
