from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / "build" / "native"
out.mkdir(parents=True, exist_ok=True)
command = ["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2"]
for include in ["tests/fakes", "libs/systime/include", "libs/stdio-text-protocol/include", "231-firmware-template", "231-firmware-template/logic"]:
    command += ["-I", str(root / include)]
command += [str(root / f) for f in ["tests/test_logic.c", "libs/systime/src/systime.c", "libs/stdio-text-protocol/src/stdio-text-protocol.c", "231-firmware-template/logic/led-task/led-task.c", "231-firmware-template/logic/button-task/button-task.c", "231-firmware-template/logic/pi-task/pi-task.c"]]
binary = out / "test_logic.exe"
subprocess.run(command + ["-o", str(binary), "-lm"], check=True)
subprocess.run([str(binary)], check=True)
for name, project, sources in [
    ("gestures", "239-two-buttons", ["logic/button-task/button-task.c"]),
    ("stream", "321-audio-recorder", ["logic/stream-task/stream-task.c"]),
]:
    binary = out / ("test_" + name + ".exe")
    args = ["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2", "-I", str(root/project), "-I", str(root/project/"logic"), "-I", str(root/"libs/systime/include")]
    subprocess.run(args + [str(root/"tests"/("test_"+name+".c"))] + [str(root/project/s) for s in sources] + ["-o",str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

binary = out / "test_api.exe"
args = ["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2", "-DPROFILING_ENABLED=0"]
for include in ["231-firmware-template", "231-firmware-template/logic", "tests/fakes"] + ["libs/"+name+"/include" for name in ["device", "systime", "profiling", "stdio-text-protocol"]]:
    args += ["-I", str(root/include)]
sources = ["tests/test_api.c", "231-firmware-template/logic/api/api.c", "231-firmware-template/logic/api/api-callbacks.c", "231-firmware-template/logic/led-task/led-task.c", "231-firmware-template/logic/pi-task/pi-task.c", "libs/systime/src/systime.c"]
subprocess.run(args + [str(root/source) for source in sources] + ["-o", str(binary)], check=True)
reply = subprocess.run([str(binary)], check=True, capture_output=True, text=True).stdout
assert reply.count("error: usage led_period <period_ms>") == 8
for line in ["error: period_ms must be greater than 0", "led: blink, period 4294967295 ms", "pi: not started", "error: terms must be greater than 0", "pi: started, 1 terms", "error: pi is busy", "pi: running", "pi: 4.000000000 in 0 ms", "profiling is disabled in this build", "error: unknown command 'not_a_command', try help", "error: command longer than 63 characters or 4 arguments"]:
    assert line in reply, line
print("API numeric bounds, dispatch, busy state and error responses passed")
