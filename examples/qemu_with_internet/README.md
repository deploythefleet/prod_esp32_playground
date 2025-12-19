# qemu_with_internet

This example demonstrates how to use the `qemu_internet` component to enable internet access in ESP32 applications running inside QEMU. The example performs DNS resolution and makes an HTTPS request to verify that networking is working correctly.

## What It Does

1. Initializes the network stack and event loop
2. Connects to the internet using the `qemu_internet` component
3. Tests DNS resolution by looking up `www.howsmyssl.com`
4. Makes an HTTPS request to verify TLS functionality
5. Runs continuously, demonstrating stable network operation

## Using the qemu_internet Component

This example uses the shared `qemu_internet` component located in `../shared_components/qemu_internet/`. For detailed documentation on how the component works and how to integrate it into your own projects, see:

**[qemu_internet Component Documentation](../shared_components/qemu_internet/README.md)**

The component handles all Ethernet initialization and DHCP configuration, providing a simple `qemu_internet_connect()` API that blocks until an IP address is obtained.

## Building and Running

```bash
idf.py build
idf.py qemu monitor
```

## Expected Output

You should see output similar to:

```
I (1238) qemu_internet: Got IPv4 event: Interface "eth0" address: 10.0.2.15
I (1238) HTTP_CLIENT: Testing DNS resolution for www.howsmyssl.com...
I (1238) HTTP_CLIENT: DNS resolved to: 34.71.45.200
I (1238) HTTP_CLIENT: HTTPS request with url => https://www.howsmyssl.com/a/check
I (2388) esp-x509-crt-bundle: Certificate validated
I (2548) HTTP_CLIENT: HTTP Response body:
[bunch of JSON]
```

## Notes

The `_http_event_handler` function is adapted from the ESP-IDF HTTP client examples and handles streaming response data from the HTTPS request.