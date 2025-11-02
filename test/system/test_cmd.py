from utils import run_server_help


def test_displays_help():
    result = run_server_help()

    help_text = result.stdout
    assert "Usage:" in help_text or "ion: the light-weight HTTP/2 server" in help_text
    assert "--port" in help_text or "-p" in help_text
    assert result.returncode == 0
