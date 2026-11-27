import {wasm, memory, ensureInit} from "./runtime.mjs";

function feed(id, bytes) {
    const ptr = wasm.malloc(bytes.length);
    new Uint8Array(memory.buffer, ptr, bytes.length).set(bytes);
    wasm.ion_conn_feed(id, ptr, bytes.length);
    wasm.free(ptr);
}

async function flush(id, writer) {
    let n;
    while ((n = wasm.ion_conn_output_size(id)) > 0) {
        const ptr = wasm.malloc(n);
        const read = wasm.ion_conn_output_read(id, ptr, n);
        const chunk = new Uint8Array(memory.buffer, ptr, read).slice();
        wasm.free(ptr);
        await writer.write(chunk);
    }
}

export async function handleConnection(socket) {
    ensureInit();
    const reader = socket.readable.getReader();
    const writer = socket.writable.getWriter();
    const id = wasm.ion_conn_create();
    try {
        for (; ;) {
            const {value, done} = await reader.read();
            if (done) break;
            feed(id, value);
            const discard = wasm.ion_conn_process(id);
            await flush(id, writer);
            if (discard) break;
        }
    } finally {
        wasm.ion_conn_close(id);
        await writer.close().catch(() => {
        });
    }
}
