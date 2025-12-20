from utils import run_server_custom_args
import os
import pytest


def build_version():
    return os.environ.get("BUILD_VERSION")


@pytest.mark.parametrize("help_flag", ["--help", "-h"])
def test_displays_help(help_flag):
    result = run_server_custom_args([help_flag])

    help_text = result.stdout
    assert "Usage:" in help_text or "ion: the light-weight HTTP/2 server" in help_text
    assert "--port" in help_text or "-p" in help_text
    assert result.returncode == 0


@pytest.mark.parametrize("version_flag", ["--version", "-v"])
def test_displays_version(version_flag):
    result = run_server_custom_args([version_flag])

    assert result.returncode == 0
    assert build_version() in result.stdout
