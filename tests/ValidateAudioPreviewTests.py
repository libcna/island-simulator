#!/usr/bin/env python3

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import wave


EXPECTED_FRAMES = {
    "surf.wav": 220500,
    "breeze.wav": 242550,
    "jungle.wav": 639450,
    "stream.wav": 286650,
    "campfire.wav": 418950,
    "bunker-hum.wav": 374850,
    "water-movement.wav": 154350,
    "stronger-wave-a.wav": 88200,
    "stronger-wave-b.wav": 88200,
    "shipwreck-creak-a.wav": 39690,
    "shipwreck-creak-b.wav": 39690,
    "water-transition-a.wav": 33075,
    "water-transition-b.wav": 33075,
    "sand-footstep-a.wav": 6615,
    "sand-footstep-b.wav": 6615,
    "soil-footstep-a.wav": 6615,
    "soil-footstep-b.wav": 6615,
    "bird-call-a.wav": 26460,
    "bird-call-b.wav": 29767,
    "monkey-call-a.wav": 39690,
    "monkey-call-b.wav": 46305,
    "boar-breath-a.wav": 26460,
    "boar-breath-b.wav": 26460,
    "boar-snuffle-a.wav": 26460,
    "boar-snuffle-b.wav": 26460,
    "boar-grunt-a.wav": 26460,
    "boar-grunt-b.wav": 26460,
    "goat-breath-a.wav": 26460,
    "goat-breath-b.wav": 26460,
    "goat-bleat-a.wav": 26460,
    "goat-bleat-b.wav": 26460,
    "goat-hoof-a.wav": 26460,
    "goat-hoof-b.wav": 26460,
    "snake-hiss-a.wav": 22050,
    "snake-hiss-b.wav": 22050,
    "bear-breath-a.wav": 35280,
    "bear-breath-b.wav": 35280,
    "bear-snuffle-a.wav": 35280,
    "bear-snuffle-b.wav": 35280,
    "bear-huff-a.wav": 35280,
    "bear-huff-b.wav": 35280,
    "fly-buzz-a.wav": 19845,
    "fly-buzz-b.wav": 19845,
}


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: ValidateAudioPreviewTests.py <island-audio-report>")

    with tempfile.TemporaryDirectory(prefix="island-audio-preview-test-") as temporary:
        root = Path(temporary)
        report_path = root / "audio-report.json"
        wave_directory = root / "wav"
        result = subprocess.run(
            [sys.argv[1], "--output", str(report_path),
             "--wav-directory", str(wave_directory)],
            check=True,
            capture_output=True,
            text=True,
        )
        if "audio report valid" not in result.stdout:
            raise AssertionError("audio exporter did not report successful validation")

        report = json.loads(report_path.read_text(encoding="utf-8"))
        if not report.get("valid") or report.get("clipCount") != len(EXPECTED_FRAMES):
            raise AssertionError("audio report summary does not describe all valid clips")
        if report.get("totalPcmBytes") != 6_881_804:
            raise AssertionError("audio report PCM footprint changed unexpectedly")

        written_files = {path.name for path in wave_directory.iterdir() if path.is_file()}
        if written_files != set(EXPECTED_FRAMES):
            raise AssertionError("WAV export set differs from the authoritative clip inventory")

        for file_name, expected_frames in EXPECTED_FRAMES.items():
            with wave.open(str(wave_directory / file_name), "rb") as preview:
                if preview.getnchannels() != 1:
                    raise AssertionError(f"{file_name} is not mono")
                if preview.getsampwidth() != 2:
                    raise AssertionError(f"{file_name} is not PCM16")
                if preview.getframerate() != 22050:
                    raise AssertionError(f"{file_name} has the wrong sample rate")
                if preview.getnframes() != expected_frames:
                    raise AssertionError(f"{file_name} has the wrong frame count")
                if len(preview.readframes(expected_frames + 1)) != expected_frames * 2:
                    raise AssertionError(f"{file_name} has an incomplete PCM payload")

    print("audio preview export validation passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
