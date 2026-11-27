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
         ( name = "ION_UNDER_TEST", text = "1" ),
         ( name = "ION_STATIC_PREFIX", text = "/static" ),
         ( name = "ION_STATIC_ROOT", text = "/static" ),
         ( name = "ION_CUSTOM_404_FILE_PATH", text = "/static/404.html" ),
       ],
     ) ),

    ( name = "assets",
     assets = (
       path = "../test/system/static",
       config = "{}",
       compatibilityDate = "2026-11-20",
     ) ),
   ],

   sockets = [
     ( name = "ion", address = "127.0.0.1:8443", tcp = (), service = "ion" ),
     ( name = "ion-h2c", address = "127.0.0.1:8080", tcp = (), service = "ion" ),
   ],
  );
