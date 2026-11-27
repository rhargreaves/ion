import ionWasm from "./ion.wasm";
import {ionLog} from "./log.mjs";
import {wasi} from "./wasi.mjs";

export const imports = {
    env: {
        ion_log: ionLog, emscripten_notify_memory_growth() {
        }
    },
    wasi_snapshot_preview1: wasi,
};

const instance = new WebAssembly.Instance(ionWasm, imports);
export const wasm = /** @type {import("./ion-abi").IonExports} */ (instance.exports);

export const memory = wasm.memory;

wasm._initialize?.();

let initialised = false;

export function ensureInit() {
    if (!initialised) {
        wasm.ion_init();
        initialised = true;
    }
}


