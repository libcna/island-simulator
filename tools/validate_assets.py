#!/usr/bin/env python3

"""Validate island-simulator's reproducible asset manifest and attribution gate."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys
from typing import Any


REQUIRED_EXTERNAL_FIELDS = {
    "name",
    "path",
    "role",
    "sha256",
    "source",
    "author",
    "license",
    "modifications",
    "use",
    "licenseRecord",
}
REQUIRED_PROJECT_FILE_FIELDS = {"name", "path", "sha256", "licenseRecord"}
SHA256_PATTERN = re.compile(r"[0-9a-f]{64}")


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate the island-simulator asset manifest and license records."
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Optional path for a machine-readable JSON validation report.",
    )
    return parser.parse_args()


def load_json(path: Path, errors: list[str]) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        errors.append(f"cannot read {path.name}: {error}")
        return {}
    if not isinstance(value, dict):
        errors.append(f"{path.name} must contain a JSON object")
        return {}
    return value


def license_records(path: Path, errors: list[str]) -> set[str]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        errors.append(f"cannot read {path.name}: {error}")
        return set()

    records: set[str] = set()
    for line in lines:
        if not line.startswith("|"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if not cells or cells[0] in {"Asset", "---"} or set(cells[0]) <= {"-", ":"}:
            continue
        records.add(cells[0])
    return records


def resolve_project_path(root: Path, raw_path: Any, field: str,
                         errors: list[str]) -> Path | None:
    if not isinstance(raw_path, str) or not raw_path:
        errors.append(f"{field} must be a non-empty repository-relative path")
        return None
    relative = Path(raw_path)
    if relative.is_absolute() or ".." in relative.parts:
        errors.append(f"{field} must stay inside the repository: {raw_path}")
        return None
    resolved = (root / relative).resolve()
    try:
        resolved.relative_to(root)
    except ValueError:
        errors.append(f"{field} escapes the repository: {raw_path}")
        return None
    return resolved


def validate_manifest(root: Path) -> dict[str, Any]:
    errors: list[str] = []
    manifest_path = root / "assets" / "manifest.json"
    manifest = load_json(manifest_path, errors)
    records = license_records(root / "ASSET_LICENSES.md", errors)

    if manifest.get("schemaVersion") != 2:
        errors.append("assets/manifest.json schemaVersion must be 2")

    generators = manifest.get("proceduralGenerators")
    if not isinstance(generators, list):
        errors.append("proceduralGenerators must be an array")
        generators = []
    external_files = manifest.get("externalFiles")
    if not isinstance(external_files, list):
        errors.append("externalFiles must be an array")
        external_files = []
    project_files = manifest.get("projectFiles")
    if not isinstance(project_files, list):
        errors.append("projectFiles must be an array")
        project_files = []

    generator_names: set[str] = set()
    documented_records: set[str] = set()
    generator_source_count = 0
    for index, generator in enumerate(generators):
        prefix = f"proceduralGenerators[{index}]"
        if not isinstance(generator, dict):
            errors.append(f"{prefix} must be an object")
            continue
        name = generator.get("name")
        record = generator.get("licenseRecord")
        sources = generator.get("sources")
        if not isinstance(name, str) or not name:
            errors.append(f"{prefix}.name must be non-empty")
        elif name in generator_names:
            errors.append(f"duplicate procedural generator name: {name}")
        else:
            generator_names.add(name)
        if not isinstance(record, str) or record not in records:
            errors.append(f"{prefix}.licenseRecord has no ASSET_LICENSES.md row: {record}")
        else:
            documented_records.add(record)
        if not isinstance(sources, list) or not sources:
            errors.append(f"{prefix}.sources must be a non-empty array")
            continue
        for source_index, source in enumerate(sources):
            path = resolve_project_path(root, source, f"{prefix}.sources[{source_index}]", errors)
            if path is not None:
                if not path.is_file():
                    errors.append(f"missing procedural generator source: {source}")
                else:
                    generator_source_count += 1

    project_file_names: set[str] = set()
    listed_project_paths: set[str] = set()
    for index, entry in enumerate(project_files):
        prefix = f"projectFiles[{index}]"
        if not isinstance(entry, dict):
            errors.append(f"{prefix} must be an object")
            continue
        missing = sorted(REQUIRED_PROJECT_FILE_FIELDS - entry.keys())
        if missing:
            errors.append(f"{prefix} is missing fields: {', '.join(missing)}")
            continue
        name = entry["name"]
        if not isinstance(name, str) or not name.strip():
            errors.append(f"{prefix}.name must be non-empty")
        elif name in project_file_names:
            errors.append(f"duplicate project asset name: {name}")
        else:
            project_file_names.add(name)
        relative_path = entry["path"]
        path = resolve_project_path(root, relative_path, f"{prefix}.path", errors)
        if isinstance(relative_path, str):
            if relative_path in listed_project_paths:
                errors.append(f"duplicate project asset path: {relative_path}")
            listed_project_paths.add(relative_path)
        expected_hash = entry["sha256"]
        if not isinstance(expected_hash, str) or not SHA256_PATTERN.fullmatch(expected_hash):
            errors.append(f"{prefix}.sha256 must be a lowercase SHA-256 digest")
        if path is not None:
            if not path.is_file():
                errors.append(f"missing project asset file: {relative_path}")
            elif isinstance(expected_hash, str) and SHA256_PATTERN.fullmatch(expected_hash):
                actual_hash = hashlib.sha256(path.read_bytes()).hexdigest()
                if actual_hash != expected_hash:
                    errors.append(f"SHA-256 mismatch for {relative_path}")
        record = entry["licenseRecord"]
        if not isinstance(record, str) or record not in records:
            errors.append(f"{prefix}.licenseRecord has no ASSET_LICENSES.md row: {record}")
        else:
            documented_records.add(record)

    listed_external_paths: set[str] = set()
    for index, entry in enumerate(external_files):
        prefix = f"externalFiles[{index}]"
        if not isinstance(entry, dict):
            errors.append(f"{prefix} must be an object")
            continue
        missing = sorted(REQUIRED_EXTERNAL_FIELDS - entry.keys())
        if missing:
            errors.append(f"{prefix} is missing fields: {', '.join(missing)}")
            continue
        relative_path = entry["path"]
        path = resolve_project_path(root, relative_path, f"{prefix}.path", errors)
        if isinstance(relative_path, str):
            if relative_path in listed_external_paths:
                errors.append(f"duplicate external asset path: {relative_path}")
            listed_external_paths.add(relative_path)
            if relative_path in listed_project_paths:
                errors.append(f"asset path is both project-owned and external: {relative_path}")
            if not relative_path.startswith("assets/"):
                errors.append(f"external asset must live under assets/: {relative_path}")
        if entry["role"] not in {"runtime", "source"}:
            errors.append(f"{prefix}.role must be runtime or source")
        expected_hash = entry["sha256"]
        if not isinstance(expected_hash, str) or not SHA256_PATTERN.fullmatch(expected_hash):
            errors.append(f"{prefix}.sha256 must be a lowercase SHA-256 digest")
        if path is not None:
            if not path.is_file():
                errors.append(f"missing external asset file: {relative_path}")
            elif isinstance(expected_hash, str) and SHA256_PATTERN.fullmatch(expected_hash):
                actual_hash = hashlib.sha256(path.read_bytes()).hexdigest()
                if actual_hash != expected_hash:
                    errors.append(f"SHA-256 mismatch for {relative_path}")
        record = entry["licenseRecord"]
        if not isinstance(record, str) or record not in records:
            errors.append(f"{prefix}.licenseRecord has no ASSET_LICENSES.md row: {record}")
        else:
            documented_records.add(record)
        text_fields = REQUIRED_EXTERNAL_FIELDS - {"path", "role", "sha256", "licenseRecord"}
        for field in sorted(text_fields):
            if not isinstance(entry[field], str) or not entry[field].strip():
                errors.append(f"{prefix}.{field} must be non-empty")

    discovered_asset_files = {
        path.relative_to(root).as_posix()
        for path in (root / "assets").rglob("*")
        if path.is_file() and path != manifest_path
    }
    listed_asset_paths = listed_external_paths | listed_project_paths
    for unlisted in sorted(discovered_asset_files - listed_asset_paths):
        errors.append(f"unlisted file under assets/: {unlisted}")
    for missing in sorted(
        path for path in listed_asset_paths
        if path.startswith("assets/") and path not in discovered_asset_files
    ):
        errors.append(f"manifest lists a missing file under assets/: {missing}")
    for orphaned_record in sorted(records - documented_records):
        errors.append(f"ASSET_LICENSES.md row is absent from the manifest: {orphaned_record}")

    return {
        "schemaVersion": 2,
        "valid": not errors,
        "proceduralGeneratorCount": len(generators),
        "proceduralSourceCount": generator_source_count,
        "projectFileCount": len(project_files),
        "externalFileCount": len(external_files),
        "documentedLicenseRecordCount": len(documented_records),
        "errors": errors,
    }


def main() -> int:
    arguments = parse_arguments()
    root = Path(__file__).resolve().parent.parent
    report = validate_manifest(root)
    if arguments.output is not None:
        try:
            arguments.output.write_text(
                json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
            )
        except OSError as error:
            print(f"island-simulator: cannot write asset report: {error}", file=sys.stderr)
            return 1

    if report["valid"]:
        project_file_suffix = "" if report["projectFileCount"] == 1 else "s"
        print(
            "island-simulator: asset validation passed: "
            f"{report['proceduralGeneratorCount']} procedural records, "
            f"{report['proceduralSourceCount']} generator sources, "
            f"{report['projectFileCount']} project file{project_file_suffix}, "
            f"{report['externalFileCount']} external files"
        )
        return 0

    print("island-simulator: asset validation failed", file=sys.stderr)
    for error in report["errors"]:
        print(f"  - {error}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
