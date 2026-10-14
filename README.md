### Distributed Passive Network Traffic Analyzer & IDS
A comprehensive cross-platform C++ solution for passive network traffic monitoring, anomaly detection, and network attack identification. The system is built on an agent-server architecture utilizing a multi-process model for strict fault isolation and POSIX IPC mechanisms for secure state exchange.
### Architecture
The system consists of three isolated nodes interacting via secure IPC channels:
Network Agent (client_app): Captures raw network traffic, parses link and network layer headers, and transmits structured logs to the central server.
Analysis Server (server_app): The core engine. It uses a process-per-client model. Each connected agent is handled by a separate child process, ensuring that a crash in the packet parser (e.g., due to malformed packets) only kills the specific worker, keeping the main server and other clients alive. Workers pass traffic through an analysis pipeline, send reports back to the agent, and atomically update global statistics in Shared Memory.
Admin Console (admin_app): Connects to the server via Named Pipes (FIFO) to fetch and visualize global threat statistics in real-time.
### Detection Capabilities
The server pipeline includes stateless and stateful checks for:
TCP Anomalies: SYN Floods, SYN Scans (per IP), Invalid TCP flag combinations (e.g., SYN+FIN, IPS evasion).
ICMP Threats: ICMP DDoS (Echo Flood), ICMP Tunneling / Ping of Death (oversized payloads), ICMP MITM (Redirect attacks), and legacy vulnerable ICMP protocols.
Port Vulnerabilities: Traffic targeting reserved Port 0.
Prerequisites
OS: Linux (requires POSIX IPC, fork(), prctl)
Compiler: GCC/Clang with C++17 support
Dependencies:
libtins (for packet sniffing and parsing)
nlohmann/json (for JSON serialization)
Tools: CMake, Make
### Build Instructions
```bash
mkdir build && cd build
cmake ..
make
```
### Usage Guide
The system requires 4 executables. They should be run in separate terminal windows.
### 1. Start the Server
The server initializes Shared Memory, creates FIFO pipes, and starts listening for agent connections on port 7009.
```bash
./server_app
```
### 2. Start the Admin Console (Optional)
Connects to the server's FIFO to request global statistics.
```bash
./admin_app
```
### 3. Start the Network Agent (Client)
Requires sudo privileges to open raw sockets for packet capturing. By default, it listens on the loopback interface (lo) and applies a BPF filter to capture only relevant traffic.
```bash
# Default run (interface: lo, filter: "tcp port 7009 or tcp port 0 or icmp")
sudo ./client_app
# Custom run
sudo ./client_app <interface_name> "<bpf_filter>"
```
The client will output the JSON report received from the server after analyzing the captured traffic

### 4. Generate Malicious Traffic
Use the traffic generator to simulate attacks over the loopback interface (lo) to trigger the client's filters and the server's checks.
```bash
./traffic_gen
```
This utility will send crafted packets (e.g., invalid TCP flags, ICMP redirects, Port 0 traffic) that will be caught by the client, analyzed by the server, and reflected in the admin console's global statistics.
### Inter-Process Communication (IPC) Flow
Agent to Server: TCP Sockets (JSON payload).
Server Workers to Global Stats: POSIX Shared Memory protected by Semaphores for atomic updates.
Server to Admin: FIFO Named Pipes for reading requests and writing responses.
Process Management: The server uses prctl(PR_SET_PDEATHSIG) and SIGCHLD handlers to ensure clean teardown of child processes and shared resources upon termination.
### Project Overview
A distributed system for passive network traffic analysis and anomaly detection implemented in C++. The application consists of three nodes: a network agent, an analysis server, and an administrator console. The agent captures raw traffic, parses headers, and sends structured logs to the server. The server is built on a process-per-client architecture for strict fault isolation: each client is handled by a separate process, which prevents the entire system from crashing when processing invalid packets. Server workers pass traffic through a pipeline of signature and heuristic checks, generate reports for the agent, and atomically update global statistics in shared memory. The administrator console connects to the server via named pipes to monitor global statistics in real time. The interaction of components is implemented via IPC mechanisms (Shared Memory, FIFO, TCP Sockets), ensuring high performance and fault tolerance of the system.