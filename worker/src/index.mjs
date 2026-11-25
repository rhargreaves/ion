import ionWasm from "./ion.wasm";

const dec = new TextDecoder();
let memory;

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
    env: {ion_log: ionLog},
});
memory = instance.exports.memory;

instance.exports._initialize?.();
instance.exports.ion_init();

export default {
    async fetch() {
        return new Response("ok\n");
    },
};
