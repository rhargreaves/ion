from utils import run_server_custom_args
import os


def build_version():
    return os.environ.get("BUILD_VERSION")


def test_displays_help():
    result = run_server_custom_args(["--help"])

    help_text = result.stdout
    assert "Usage:" in help_text or "ion: the light-weight HTTP/2 server" in help_text
    assert "--port" in help_text or "-p" in help_text
    assert result.returncode == 0


def test_displays_version():
    result = run_server_custom_args(["--version"])

    assert result.returncode == 0
    assert build_version() in result.stdout
