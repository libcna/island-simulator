"""Check the emitted JSON contract independently of the report's internal validity bit."""
import copy
import json
import math
import subprocess
import sys


def validate(report):
    assert report["valid"] is True
    population = report["groundSnakePopulation"]
    assert population["snapshot"] == "initial"
    assert population["treeHabitatImplemented"] is False
    assert report["wildlife"]["groundSnakes"] == len(population["snakes"]) == 4
    assert population["stationsPerSnake"] == 41
    assert population["spacingMeters"] == 0.055
    homes = []
    for snake in population["snakes"]:
        home = snake["home"]
        body = snake["body"]
        assert len(home) == 2 and all(map(math.isfinite, home))
        assert math.isfinite(snake["headingRadians"])
        assert len(body) == 41
        homes.append(tuple(home))
        for point in body:
            assert len(point) == 3 and all(map(math.isfinite, point))
            assert math.hypot(point[0] - home[0], point[2] - home[1]) <= 18.001
        for a, b in zip(body, body[1:]):
            # 0.1mm JSON rounding adds at most sqrt(2)*0.1mm to horizontal distance.
            assert abs(math.hypot(a[0] - b[0], a[2] - b[2]) - .055) < .0018
    assert len(set(homes)) == 4


if __name__ == "__main__":
    result = subprocess.run([sys.argv[1]], capture_output=True, text=True, check=True)
    report = json.loads(result.stdout)
    validate(report)
    for fault in ("count", "missing-body", "nonfinite", "tree-claim", "broken-body"):
        bad = copy.deepcopy(report)
        population = bad["groundSnakePopulation"]
        if fault == "count":
            bad["wildlife"]["groundSnakes"] = 0
        elif fault == "missing-body":
            population["snakes"][0]["body"].pop()
        elif fault == "nonfinite":
            population["snakes"][0]["body"][0][1] = float("nan")
        elif fault == "tree-claim":
            population["treeHabitatImplemented"] = True
        else:
            population["snakes"][0]["body"][1][0] += 1
        try:
            validate(bad)
        except AssertionError:
            continue
        raise AssertionError(f"validator accepted {fault}")
    print("World snake JSON: four ground snakes, complete bodies and five negative cases passed")
