"""Keep native build/test presets inclusive of newly registered CPU regressions."""
import copy
import json
from pathlib import Path


def validate(document):
    builds = {p["name"]: p for p in document["buildPresets"]}
    for name in ("linux-debug", "linux-sanitize"):
        assert builds[name]["targets"] == ["all"], "curated target lists can leave stale test binaries"
    tests = {p["name"]: p for p in document["testPresets"]}
    assert tests["linux-core"]["filter"] == {"exclude": {"label": "display"}}, "run every CPU test, never display tests"


if __name__ == "__main__":
    document = json.loads((Path(__file__).resolve().parents[1] / "CMakePresets.json").read_text())
    validate(document)
    for fault in ("debug-targets", "sanitize-targets", "narrow-tests", "unsafe-tests"):
        bad = copy.deepcopy(document)
        builds = {p["name"]: p for p in bad["buildPresets"]}
        tests = {p["name"]: p for p in bad["testPresets"]}
        if fault == "debug-targets":
            builds["linux-debug"]["targets"] = ["island-simulator"]
        elif fault == "sanitize-targets":
            builds["linux-sanitize"]["targets"] = ["island_core_tests"]
        elif fault == "narrow-tests":
            tests["linux-core"]["filter"] = {"include": {"name": "^IslandCoreTests$"}}
        else:
            tests["linux-core"]["filter"] = {}
        try:
            validate(bad)
        except AssertionError:
            continue
        raise AssertionError(f"preset validator accepted {fault}")
    print("Native preset coverage and four rejected regressions passed")
