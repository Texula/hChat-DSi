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
    // We use keyboardInit instead of keyboardDemoInit for more control
    Keyboard* kb = keyboardDemoInit();
    keyboardShow();
    
    // Clear the bottom console area so the keyboard doesn't draw over old text
    printf("\x1b[2J"); // ANSI clear screen code
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

    // 2. IMPORTANT: Hide and "kill" the keyboard visual to stop artifacts
    keyboardHide();
    // Clear the bottom screen again to remove keyboard remnants
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
    
    // Init Top Screen (Main engine)
    consoleInit(&topScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    // Init Bottom Screen (Sub engine) - This is where the keyboard lives
    consoleInit(&bottomScreen, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);

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