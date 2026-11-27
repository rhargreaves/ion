export interface IonExports {
    memory: WebAssembly.Memory;
    _initialize?: () => void;

    malloc(size: number): number;

    free(ptr: number): void;

    ion_init(): void;

    ion_conn_create(): number;

    ion_conn_feed(id: number, ptr: number, len: number): void;

    ion_conn_process(id: number): number;

    ion_conn_output_size(id: number): number;

    ion_conn_output_read(id: number, ptr: number, n: number): number;

    ion_conn_close(id: number): void;
}
