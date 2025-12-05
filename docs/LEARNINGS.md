# Interesting Gotchas

* On macOS, the main thread has 8 MB stack size by default. However, additional threads are limited to 512 KB.
* On macOS, the client file descriptor's blocking-ness is inherited from the server socket. This is NOT the case on
  Linux.
