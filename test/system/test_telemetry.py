import pytest
import httpx
import asyncio
from aiohttp import web
from helpers.utils import OK_URL

OTLP_TRACES_PATH = "/v1/traces"
MOCK_COLLECTOR_PORT = 43180


class MockOTLPCollector:
    def __init__(self):
        self.received_payloads = []
        self.app = web.Application()
        self.app.router.add_post(OTLP_TRACES_PATH, self.handle_traces)
        self.runner = None

    async def handle_traces(self, request):
        print(f"Received OTLP trace payload: {request.content}")
        body = await request.read()
        self.received_payloads.append(body)
        return web.Response(status=200)

    async def start(self):
        print(f"Starting mock OTLP collector on port {MOCK_COLLECTOR_PORT}")
        self.runner = web.AppRunner(self.app)
        await self.runner.setup()
        site = web.TCPSite(self.runner, 'localhost', MOCK_COLLECTOR_PORT)
        await site.start()

    async def stop(self):
        print(f"Stopping mock OTLP collector")
        if self.runner:
            await self.runner.cleanup()


@pytest.mark.asyncio
@pytest.mark.parametrize("ion_server", [{
    "env": {
        "OTEL_EXPORTER_OTLP_ENDPOINT": f"http://localhost:{MOCK_COLLECTOR_PORT}",
        "OTEL_BSP_SCHEDULE_DELAY": "10"
    }
}], indirect=True)
async def test_ion_exports_otlp_traces(ion_server):
    collector = MockOTLPCollector()
    await collector.start()
    try:
        client = httpx.AsyncClient(http2=True, verify=False)
        resp = await client.get(OK_URL)
        assert resp.status_code == 200

        await asyncio.sleep(0.1)

        assert len(collector.received_payloads) > 0

        all_data = b"".join(collector.received_payloads)
        assert b"http_request" in all_data
        assert b"ion-server" in all_data  # service.name

    finally:
        await collector.stop()
