import ionWasm from "./ion.wasm";

const dec = new TextDecoder();
let memory;

const wasi = {
    fd_close() {
        return 0;
    },
    clock_time_get(clockId, precision /* BigInt */, timePtr) {
        const ns = clockId === 0
            ? BigInt(Date.now()) * 1000000n          // CLOCK_REALTIME  -> system_clock
            : BigInt(Math.round(performance.now() * 1e6));  // CLOCK_MONOTONIC -> steady_clock
        new DataView(memory.buffer).setBigUint64(timePtr, ns, true);
        return 0;
    },
};

function ionLog(level, ptr, len) {
    const message = dec.decode(new Uint8Array(memory.buffer, ptr, len));
    const text = message.endsWith("\n") ? message.slice(0, -1) : message;
    if (level >= 4) {
        console.error(text);
    } else if (level === 3) {
        console.warn(text);
    } else {
        console.log(text);
    }
}

const instance = new WebAssembly.Instance(ionWasm, {
    env: {
        ion_log: ionLog,
        emscripten_notify_memory_growth: () => {
        },
    },
    wasi_snapshot_preview1: wasi,
});
const wasm = instance.exports;
memory = wasm.memory;
wasm._initialize?.();

let initialised = false;

function ensureInit() {
    if (initialised) {
        return
    }
    wasm.ion_init();
    initialised = true;
}

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

export default {
    async fetch() {
        return new Response("ok\n");
    },
    async connect(socket) {
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
};
