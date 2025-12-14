# Benchmark Results

## ion 0.1.0

* Supports a single connection at one time

```
$ h2load https://localhost:8443/ -n 1000 -c 10 -t 8
starting benchmark...
spawning thread #0: 2 total client(s). 125 total requests
spawning thread #1: 2 total client(s). 125 total requests
spawning thread #2: 1 total client(s). 125 total requests
spawning thread #3: 1 total client(s). 125 total requests
spawning thread #4: 1 total client(s). 125 total requests
spawning thread #5: 1 total client(s). 125 total requests
spawning thread #6: 1 total client(s). 125 total requests
spawning thread #7: 1 total client(s). 125 total requests
TLS Protocol: TLSv1.3
Cipher: TLS_AES_128_GCM_SHA256
Server Temp Key: X25519 253 bits
Application protocol: h2

finished in 661.88ms, 1510.84 req/s, 16.88KB/s
requests: 1000 total, 1000 started, 1000 done, 1000 succeeded, 0 failed, 0 errored, 0 timeout
status codes: 1000 2xx, 0 3xx, 0 4xx, 0 5xx
traffic: 11.17KB (11440) total, 2.03KB (2080) headers (space savings 91.68%), 0B (0) data
                     min         max         mean         sd        +/- sd
time for request:       45us       299us        89us        26us    76.30%
time for connect:    10.28ms    649.25ms    213.57ms    192.17ms    80.00%
time to 1st byte:    10.69ms    649.52ms    213.82ms    192.18ms    80.00%
req/s           :     188.93     5585.94     1239.99     1771.61    80.00%
```

## ion 0.1.1

* Supports multiple connections at one time

```
h2load https://localhost:8443/ -n 1000 -c 10 -t 8
starting benchmark...
spawning thread #0: 2 total client(s). 125 total requests
spawning thread #1: 2 total client(s). 125 total requests
spawning thread #2: 1 total client(s). 125 total requests
spawning thread #3: 1 total client(s). 125 total requests
spawning thread #4: 1 total client(s). 125 total requests
spawning thread #5: 1 total client(s). 125 total requests
spawning thread #6: 1 total client(s). 125 total requests
spawning thread #7: 1 total client(s). 125 total requests
TLS Protocol: TLSv1.3
Cipher: TLS_AES_128_GCM_SHA256
Server Temp Key: X25519 253 bits
Application protocol: h2

finished in 605.29ms, 1652.10 req/s, 18.46KB/s
requests: 1000 total, 1000 started, 1000 done, 1000 succeeded, 0 failed, 0 errored, 0 timeout
status codes: 1000 2xx, 0 3xx, 0 4xx, 0 5xx
traffic: 11.17KB (11440) total, 2.03KB (2080) headers (space savings 91.68%), 0B (0) data
                     min         max         mean         sd        +/- sd
time for request:       43us      7.24ms       105us       228us    99.70%
time for connect:     8.41ms    595.65ms    168.48ms    183.04ms    90.00%
time to 1st byte:    15.48ms    596.10ms    169.47ms    182.44ms    90.00%
req/s           :     104.11     4374.92     1566.67     1662.73    80.00%

```
