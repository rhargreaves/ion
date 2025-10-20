import subprocess
import time


def wait_for_port(port, timeout=5):
    start = time.time()
    while time.time() - start < timeout:
        try:
            result = subprocess.run(
                ["netstat", "-an"],
                capture_output=True,
                text=True,
                timeout=1
            )
            for line in result.stdout.splitlines():
                if f".{port}" in line and "LISTEN" in line:
                    return True
        except (subprocess.TimeoutExpired, subprocess.SubprocessError):
            pass
        time.sleep(0.05)
    return False
