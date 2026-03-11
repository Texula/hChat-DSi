#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define SERVER_PORT 5050
#define SERVER_IP "37.143.163.56"

char chat_user[32] = {0};
char chat_pass[32] = {0};

void get_user_input(const char* prompt, char* buffer, int max_len) {
    // 1. Setup the keyboard specifically for this prompt
    // Grab the default layout but initialize it manually on Background Layer 3.
    // We use Map Base 20 to keep it far away from the console's Map Base 31.
    Keyboard* kb = keyboardGetDefault();
    keyboardInit(kb, 3, BgType_Text4bpp, BgSize_T_256x256, 20, 0, false, true);
    keyboardShow();
    
    // Clear the restricted window area we set up in main()
    printf("\x1b[2J"); 
    printf("\n%s\n> ", prompt);
    
    int index = 0;
    while(1) {
        swiWaitForVBlank();
        int c = keyboardUpdate();
        
        if (c == '\n') { 
            buffer[index] = '\0';
            break;
        } else if (c == '\b' && index > 0) {
            index--;
            printf("\b \b"); 
        } else if (c > 0 && index < max_len - 1) {
            buffer[index++] = (char)c;
            printf("%c", (char)c);
        }
    }

    // 2. IMPORTANT: Hide the keyboard to free up the screen
    keyboardHide();
    // Clear the restricted screen window again
    printf("\x1b[2J"); 
}

int main(void) {
    // Initialize Video and VRAM Banks correctly
    // Bank A for Main (Top), Bank C for Sub (Bottom)
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    PrintConsole topScreen, bottomScreen;
    
    // Init Top Screen (Main engine) - Maps to Background 3, Map Base 31
    consoleInit(&topScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    
    // Init Bottom Screen (Sub engine) - Maps to Background 0, Map Base 31
    consoleInit(&bottomScreen, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);

    // Restrict the text area on the bottom screen to the top 10 rows!
    // This stops the text from ever reaching the keyboard area (which sits below), 
    // preventing the hardware scrolling from tearing the keyboard graphics.
    consoleSetWindow(&bottomScreen, 0, 0, 32, 10);

    // Start with Bottom Screen for Input
    consoleSelect(&bottomScreen);
    get_user_input("Enter Username:", chat_user, 32);
    get_user_input("Enter Password:", chat_pass, 32);

    // Switch focus to Top Screen for Chat Logs
    consoleSelect(&topScreen);
    printf("--- DSi Chat ---\nUser: %s\nConnecting...\n", chat_user);

    // --- WIFI & SOCKET LOGIC ---
    if (!Wifi_InitDefault(WFC_CONNECT)) {
        printf("WiFi failed!\n");
        while (1) swiWaitForVBlank();
    }
    while (Wifi_AssocStatus() != ASSOCSTATUS_ASSOCIATED) swiWaitForVBlank();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Bridge offline\n");
        while (1) swiWaitForVBlank();
    }

    // Set socket to non-blocking mode so the UI doesn't freeze while waiting for messages
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);

    char loginCmd[256];
    snprintf(loginCmd, sizeof(loginCmd), "LOGIN|%s|%s\n", chat_user, chat_pass);
    send(sock, loginCmd, strlen(loginCmd), 0);

    bool loggedIn = false;
    char netBuffer[1024];

    while (1) {
        scanKeys();
        int pressed = keysDown();

        if (loggedIn && (pressed & KEY_A)) {
            char msg[] = "MSG|Hello!\n";
            send(sock, msg, strlen(msg), 0);
            // Print local feedback on the top screen
            printf("[%s]: Hello!\n", chat_user);
        }

        int bytes = recv(sock, netBuffer, sizeof(netBuffer) - 1, 0);
        if (bytes > 0) {
            netBuffer[bytes] = '\0';
            if (!loggedIn && strstr(netBuffer, "OK|LOGIN")) {
                loggedIn = true;
                printf("Auth Success!\nTap A to say Hi.\n");
            } else if (loggedIn) {
                printf("%s", netBuffer);
            }
        }

        swiWaitForVBlank();
    }

    return 0;
}