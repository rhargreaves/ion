import pytest
import sys
from helpers.utils import curl_http2, assert_curl_response_ok, SECURITY_URL


@pytest.mark.skipif(sys.platform == "darwin", reason="PR_SET_NO_NEW_PRIVS is Linux-specific")
@pytest.mark.asyncio
async def test_server_enforces_no_new_privs_on_linux(ion_server):
    result = await curl_http2(SECURITY_URL)

    assert result.returncode == 0
    assert_curl_response_ok(result)
