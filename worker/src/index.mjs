import {handleConnection} from "./connection.mjs";

//noinspection JSUnusedGlobalSymbols
export default {
    async fetch() {
        return new Response("ok\n");
    },
    async connect(socket) {
        return handleConnection(socket);
    }
};
