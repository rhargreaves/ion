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
memory = instance.exports.memory;

instance.exports._initialize?.();
instance.exports.ion_init();

export default {
    async fetch() {
        return new Response("ok\n");
    },
};
