import {memory} from "./runtime.mjs";

export const wasi = {
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
