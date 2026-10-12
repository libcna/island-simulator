"""Independent initial goat JSON contract; not a rendering or habitat oracle."""
import copy
import json
import math
import subprocess
import sys


def validate(report):
    assert report["valid"] is True
    population = report["goatPopulation"]
    assert population["snapshot"] == "initial"
    goats = population["goats"]
    assert report["wildlife"]["goats"] == len(goats) == 6
    positions = []
    for goat in goats:
        point = goat["position"]
        assert len(point) == 3 and all(map(math.isfinite, point))
        assert point[1] >= 6 and abs(point[0]) <= 220 and abs(point[2]) <= 220
        assert math.isfinite(goat["headingRadians"])
        assert goat["scale"] in (0.75, 1.0)
        assert all(math.hypot(point[0]-p[0], point[2]-p[2]) >= 7.999 for p in positions)
        positions.append(point)
    assert sum(g["scale"] == .75 for g in goats) == 1
    assert all(math.hypot(p[0]-positions[0][0], p[2]-positions[0][2]) <= 40.001 for p in positions)


if __name__ == "__main__":
    report = json.loads(subprocess.run([sys.argv[1]], capture_output=True, text=True, check=True).stdout)
    validate(report)
    for fault in ("count", "missing", "nonfinite", "overlap", "scale"):
        bad = copy.deepcopy(report)
        goats = bad["goatPopulation"]["goats"]
        if fault == "count":
            bad["wildlife"]["goats"] = 0
        elif fault == "missing":
            goats.pop()
        elif fault == "nonfinite":
            goats[0]["position"][0] = float("nan")
        elif fault == "overlap":
            goats[1]["position"] = goats[0]["position"][:]
        else:
            goats[0]["scale"] = 2
        try:
            validate(bad)
        except AssertionError:
            continue
        raise AssertionError(f"validator accepted {fault}")
    print("World goat JSON: six initial goats and five negative cases passed")
