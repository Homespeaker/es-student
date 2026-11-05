"""Build each independent course project and report actual results as JSON."""
import argparse
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("projects", nargs="*")
    parser.add_argument("--jobs", type=int, default=12)
    args = parser.parse_args()
    projects = [ROOT / n for n in args.projects] if args.projects else sorted(
        p for p in ROOT.iterdir() if p.is_dir() and p.name[0].isdigit() and (p / "CMakeLists.txt").exists()
    )
    env = dict(os.environ)
    picotool = os.getenv("PICOTOOL_DIR", "C:/Repositories/pico/picotool")
    env["PATH"] = picotool + os.pathsep + env["PATH"]
    results = []
    for p in projects:
        build = p / "build"
        build.mkdir(exist_ok=True)
        config = ["cmake", "-S", str(p), "-B", str(build)]
        if not (build / "CMakeCache.txt").exists():
            config += ["-G", "Ninja"]
        if Path(picotool).is_dir():
            config += ["-Dpicotool_DIR=" + picotool]
        with (build / "configure.log").open("w", encoding="utf-8") as log:
            configured = subprocess.run(config, env=env, stdout=log, stderr=subprocess.STDOUT).returncode == 0
        success = False
        if configured:
            with (build / "build.log").open("w", encoding="utf-8") as log:
                success = subprocess.run(["cmake", "--build", str(build), "--parallel", str(args.jobs)], env=env, stdout=log, stderr=subprocess.STDOUT).returncode == 0
        log_text = (build / "build.log").read_text(encoding="utf-8", errors="replace") if (build / "build.log").exists() else ""
        binaries = list(build.glob("*.bin"))
        result = {"project": p.name, "configured": configured, "built": success,
                  "bin_bytes": binaries[0].stat().st_size if success and binaries else None,
                  "warnings": sum("warning:" in line for line in log_text.splitlines())}
        results.append(result)
        print(json.dumps(result), flush=True)
    output = ROOT / "build" / "build-results.json"
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(results, indent=2), encoding="utf-8")
    return 0 if all(r["built"] and not r["warnings"] for r in results) else 1

if __name__ == "__main__":
    raise SystemExit(main())
