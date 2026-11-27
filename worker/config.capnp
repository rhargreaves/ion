using Workerd = import "/workerd/workerd.capnp";

const config :Workerd.Config = (
  services = [
    ( name = "ion",
     worker = (
       modules = [
         ( name = "index.mjs", esModule = embed "src/index.mjs" ),
         ( name = "./connection.mjs", esModule = embed "src/connection.mjs" ),
         ( name = "./runtime.mjs", esModule = embed "src/runtime.mjs" ),
         ( name = "./log.mjs", esModule = embed "src/log.mjs" ),
         ( name = "./wasi.mjs", esModule = embed "src/wasi.mjs" ),
         ( name = "./ion.wasm", wasm = embed "../build/wasm/ion.wasm" ),
       ],
       compatibilityDate = "2026-11-20",
       bindings = [
         ( name = "ASSETS",
           service = ( name = "assets", entrypoint = "AssetWorker" ) ),
         ( name = "ION_TLS_CERT", text = embed "certs/cert.pem" ),
         ( name = "ION_TLS_KEY", text = embed "certs/key.pem" ),
       ],
     ) ),

    ( name = "assets",
     assets = (
       path = "public_html",
       config = "{}",
       compatibilityDate = "2026-11-20",
     ) ),
   ],

   sockets = [
     ( name = "ion", address = "127.0.0.1:8443", tcp = (), service = "ion" ),
   ],
  );
