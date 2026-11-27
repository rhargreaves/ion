import {memory} from './runtime.mjs'

const dec = new TextDecoder();

export function ionLog(level, ptr, len) {
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
