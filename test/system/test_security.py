import pytest
import sys
from helpers.utils import curl_http2, assert_curl_response_ok

SERVER_CLEARTEXT_PORT = 8080
SERVER_PORT = 8443
URL = f"https://localhost:{SERVER_PORT}"
SECURITY_URL = URL + "/_tests/no_new_privs"


@pytest.mark.skipif(sys.platform == "darwin", reason="PR_SET_NO_NEW_PRIVS is Linux-specific")
@pytest.mark.asyncio
async def test_server_enforces_no_new_privs_on_linux(ion_server):
    result = await curl_http2(SECURITY_URL)

    assert result.returncode == 0
    assert_curl_response_ok(result)
