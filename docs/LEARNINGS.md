# Interesting Gotchas

* On macOS, the main thread has 8 MB stack size by default. However, additional threads are limited to 512 KB.
* On macOS, the client file descriptor's blocking-ness is inherited from the server socket. This is NOT the case on
  Linux.
* TCP_NODELAY should be used to disable Nagle's algorithm to give better performance. The algorithm holds back smaller
  packets until they reach a certain size, which can lead to increased latency for small packets.
* There's no standard way to generate a UUID as of C++23. Incredible.
