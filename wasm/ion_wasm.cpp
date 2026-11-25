namespace {}  // namespace

extern "C" void ion_log(int level, const char* msg, unsigned long len);

extern "C" void ion_init() {
    const char msg[] = "ion_init: hello from wasm\n";
    ion_log(2, msg, sizeof(msg) - 1);
}
