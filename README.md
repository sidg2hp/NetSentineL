# Design and Implementation of a Concurrent, TCP-Based HTTP/HTTPS Proxy Server with Traffic Control


## 📖 Overview
This project is a high-performance **HTTP/HTTPS Forward Proxy Server** implemented in C. It acts as an intermediary system that accepts client network requests and forwards them to destination servers, relaying the responses back.

Adhering to the project requirements, this implementation demonstrates fundamental systems programming concepts:
* **Socket Programming:** Reliable TCP-based client-server communication.
* **Concurrency:** Handling multiple clients simultaneously using POSIX threads.
* **Protocol Parsing:** Custom parsing of HTTP Request Lines and Headers.
* **Traffic Control:** Configurable domain filtering and activity logging.
* **Extensions:** Support for HTTPS Tunneling via the `CONNECT` method.

## 🚀 Objectives & Features
* **Reliable TCP Communication:** Built on standard POSIX sockets (`socket`, `bind`, `listen`, `accept`).
* **Concurrency:** Uses a **Thread-per-Connection** model to handle multiple clients without blocking.
* **HTTPS Tunneling:** Supports the `CONNECT` method to transparently forward encrypted SSL/TLS traffic.
* **Access Control:** Blocks blacklisted domains defined in `config/blocked_domains.txt` with `403 Forbidden` responses.
* **Logging:** Automatically records timestamp, client IP, requested URL, and status codes to `proxy.log`.

## 📂 Project Layout
The repository structure separates networking, parsing, and filtering logic to ensure modularity and testability.

```text
proxy-project/
├── Makefile                    # Build instructions
├── README.md                   # Project documentation (Design & Usage)
├── config/
│   ├── blocked_domains.txt     # Filtering rules (Blocked domains)
│   └── server.conf             # Server configuration defaults
├── include/                    # Header files (Modular interfaces)
│   ├── proxy_filter.h
│   ├── proxy_log.h
│   └── proxy_parse.h
├── src/                        # Source code (Implementation)
│   ├── proxy.c                 # Main server & threading logic
│   ├── proxy_filter.c          # Domain filtering logic
│   └── proxy_parse.c           # HTTP parsing logic
└── tests/
    └── test_proxy.sh           # Automated test scripts

```

## 🛠️ Build & Runtime Instructions

### Prerequisites

* **OS:** Linux (Ubuntu, Debian, or WSL)
* **Compiler:** GCC
* **Build Tool:** Make

### Compilation

To compile the project and link the required `pthread` libraries, run the following command in the project root:

```bash
make

```

* **Output:** An executable file named `proxy`.

### Running the Server

Start the proxy server using the default configuration (Port 8888):

```bash
./proxy

```

## 🧪 Testing Strategies

You can verify the proxy functionality using `curl` commands or the automated test suite.

### 1. Basic Forwarding (HTTP)

Verify that the proxy correctly forwards a standard GET request.

```bash
curl -x http://localhost:8888 [http://example.com](http://example.com)

```

* **Expected:** Returns the HTML content of Example Domain (200 OK).

### 2. Domain Filtering (Blocking)

Verify that the proxy blocks domains listed in `config/blocked_domains.txt`.

```bash
curl -x http://localhost:8888 [http://badsite.com](http://badsite.com)

```

* **Expected:** Returns `403 Forbidden` error page.

### 3. HTTPS Tunneling (CONNECT)

Verify that the proxy can tunnel encrypted traffic using the `CONNECT` method.

```bash
curl -x http://localhost:8888 [https://google.com](https://google.com)

```

* **Expected:** Returns a response from Google (e.g., 301 Moved or page content), confirming the tunnel is established.

### 4. Automated Test Suite

A script is provided to run concurrent and sequential tests automatically.

```bash
# Ensure ./proxy is running in a separate terminal window first
./tests/test_proxy.sh

```

## ⚙️ Configuration

The server behavior is controlled by files in the `config/` directory:

1. **Filtering Rules (`config/blocked_domains.txt`):**
* Add one domain per line to block access.
* Example:
```text
badsite.com
facebook.com

```




2. **Server Config (`config/server.conf`):**
* Documents the default listening address (`0.0.0.0`), port (`8888`), and log location (`proxy.log`).



## 📋 Logging

All traffic events are logged to `proxy.log` in the root directory.

**Sample Log Output:**

```text
[2025-12-21 14:00:01] Client: 127.0.0.1 | Request: example.com | Status: 200
[2025-12-21 14:00:05] Client: 127.0.0.1 | Request: badsite.com | Status: 403

```

---

## 📐 Design & Architecture Document

### 1. High-Level Architecture

The system is designed as a **Multi-threaded Forward Proxy**. It operates at the Application Layer (HTTP) and Transport Layer (TCP) to intercept, inspect, and forward client traffic.

**Core Components:**

1. **Listener Module:** Creates a TCP socket, binds to Port 8888, and listens for incoming client connections.
2. **Dispatcher:** Upon `accept()`, a new POSIX thread (`pthread`) is spawned to handle the specific client connection, ensuring the main loop remains non-blocking.
3. **HTTP Parser:** Analyzes the raw request buffer to extract the HTTP Method (GET, CONNECT), Hostname, and Port.
4. **Filter Engine:** Compares the extracted Hostname against the `blocked_domains.txt` blacklist.
5. **Relay Engine:** Manages the upstream connection to the destination server and bridges data between client and server.

### 2. Concurrency Model

**Selected Model:** Thread-per-Connection.

**Rationale:**

* **Reliability:** The problem statement requires reliable client-server communication. Isolating each client in its own thread ensures that if one transaction stalls (e.g., a slow upstream server), it does not block the main acceptor loop or other clients.
* **Simplicity:** Using POSIX threads (`pthread`) allows for straightforward, linear logic flow (Read -> Parse -> Connect -> Forward) compared to the complexity of asynchronous event loops.
* **Resource Management:** Threads are detached immediately after creation, allowing the OS to reclaim resources automatically upon completion.

### 3. Data Flow Description

#### Scenario A: Standard HTTP Request

1. **Ingress:** Client sends a standard HTTP request (e.g., `GET http://example.com/ HTTP/1.1`).
2. **Parsing:** The proxy parses the headers to find `Host: example.com`.
3. **Filtering:** The Filter Engine checks if `example.com` is blacklisted.
* If Blocked: Respond with `403 Forbidden` and close connection.
* If Allowed: Proceed.


4. **Upstream Connection:** Proxy performs DNS resolution (`gethostbyname`) and opens a TCP socket to the destination.
5. **Forwarding:** The client's original request buffer is sent to the destination.
6. **Response:** The proxy reads the destination's response and writes it back to the client socket until the stream ends.

#### Scenario B: HTTPS (CONNECT) Tunneling

1. **Ingress:** Client sends a `CONNECT` request (e.g., `CONNECT google.com:443`).
2. **Handshake:** Proxy connects to the destination port (443) and sends `HTTP/1.1 200 Connection Established` to the client.
3. **Tunneling:** The proxy enters a "blind forwarding" mode using `select()`.
* Encrypted bytes received from the Client are immediately sent to the Server.
* Encrypted bytes received from the Server are immediately sent to the Client.
* The proxy does not interpret the payload, maintaining end-to-end encryption security.



### 4. Error Handling & Limitations

* **Upstream Failures:** If the destination server is unreachable, the proxy returns `502 Bad Gateway`.
* **Security:** The proxy does not inspect HTTPS payloads (SSL/TLS Interception), preserving user privacy.
* **Resource Limits:** The server uses `SO_REUSEADDR` to prevent port locking during restarts. Connections are closed after a single request-response cycle (no persistent Keep-Alive support).

```

```
