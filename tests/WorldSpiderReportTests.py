"""Independent initial spider JSON/kinematics contract, not visual acceptance."""
import copy
import json
import math
import subprocess
import sys


def point(p):
    assert len(p) == 3 and all(math.isfinite(v) for v in p)
    return p


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def distance(a, b):
    return math.sqrt(sum((x-y)**2 for x, y in zip(a, b)))


def validate(report):
    assert report["valid"] is True
    population = report["groundSpiderPopulation"]
    assert population["snapshot"] == "initial"
    assert population["websImplemented"] is True
    webs = population["webs"]
    assert report["wildlife"]["canopyWebs"] == population["webCount"] == len(webs) == 6
    assert len({web["canopyIndex"] for web in webs}) == 6
    for web in webs:
        assert isinstance(web["canopyIndex"], int) and 0 <= web["canopyIndex"] < report["placements"]["canopyTrees"]
        assert len(web["anchors"]) == len(web["rest"]) == 3
        for anchor in web["anchors"]:
            assert len(anchor) == 4 and isinstance(anchor[0], int) and 1 <= anchor[0] <= 7
            assert isinstance(anchor[1], int) and 0 <= anchor[1] < 24
            assert 0 <= anchor[2] <= 1 and 0 <= anchor[3] <= 1 and anchor[2]+anchor[3] <= 1
        rest = [point(p) for p in web["rest"]]
        assert sum(p[1] for p in rest)/3 >= 2
        assert all(distance(rest[i], rest[(i+1)%3]) >= .299 for i in range(3))
    spiders = population["spiders"]
    assert report["wildlife"]["groundSpiders"] == len(spiders) == 200
    seen = []
    for spider in spiders:
        position, home = point(spider["position"]), point(spider["home"])
        assert position == home and position[1] >= 2
        assert abs(position[0]) <= 220 and abs(position[2]) <= 220
        assert all(math.hypot(position[0]-p[0], position[2]-p[2]) >= 4.499 for p in seen)
        seen.append(position)
        assert isinstance(spider["canopyIndex"], int) and 0 <= spider["canopyIndex"] < report["placements"]["canopyTrees"]
        assert math.isfinite(spider["headingRadians"])
        assert spider["speed"] == 0 and spider["behavior"] == 0
        basis = spider["basis"]
        assert len(basis) == 3
        for axis in basis:
            point(axis)
            assert abs(dot(axis, axis)-1) < 1e-5
        for a, b in ((0, 1), (0, 2), (1, 2)):
            assert abs(dot(basis[a], basis[b])) < 1e-5
        feet = spider["feet"]
        assert len(feet) == 8
        for foot in feet:
            anchor = point(foot["position"])
            assert foot["stance"] is True and foot["reachLimited"] is False
            joints = foot["joints"]
            assert len(joints) == 4
            for joint in joints:
                point(joint)
            for index, length in enumerate((.018, .050, .065)):
                assert abs(distance(joints[index], joints[index+1])-length) < 2e-6
            local = joints[-1]
            reconstructed = [position[i]+sum(basis[j][i]*local[j] for j in range(3)) for i in range(3)]
            assert distance(reconstructed, anchor) < .00004
            assert math.hypot(anchor[0]-position[0], anchor[2]-position[2]) <= .14
        assert len({tuple(foot["position"]) for foot in feet}) == 8


if __name__ == "__main__":
    report = json.loads(subprocess.run([sys.argv[1]], capture_output=True, text=True, check=True).stdout)
    validate(report)
    for fault in ("count", "missing", "nonfinite", "overlap", "webs", "feet", "anchor", "bone", "basis", "stance", "canopy"):
        bad = copy.deepcopy(report)
        population = bad["groundSpiderPopulation"]
        spiders = population["spiders"]
        first = spiders[0]
        if fault == "count":
            bad["wildlife"]["groundSpiders"] = 0
        elif fault == "missing":
            spiders.pop()
        elif fault == "nonfinite":
            first["headingRadians"] = float("nan")
        elif fault == "overlap":
            spiders[1]["position"] = spiders[1]["home"] = first["position"][:]
        elif fault == "webs":
            population["webCount"] = 0
        elif fault == "feet":
            first["feet"].pop()
        elif fault == "anchor":
            first["feet"][0]["position"][0] += .01
        elif fault == "bone":
            first["feet"][0]["joints"][2][1] += .01
        elif fault == "basis":
            first["basis"][0][0] += .1
        elif fault == "stance":
            first["feet"][0]["stance"] = False
        else:
            first["canopyIndex"] = -1
        try:
            validate(bad)
        except AssertionError:
            continue
        raise AssertionError(f"validator accepted {fault}")
    print("World spider JSON: two hundred ground spiders, six canopy webs and eleven negative cases passed")
