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

// TWiLight Menu++ / nds-bootstrap leaves hardware registers in a dirty state.
// This function forces the scroll registers back to zero so the 
// keyboard and console don't slide or artifact.
void sanitize_hardware(void) {
    // Reset sub screen scroll registers (Bottom Screen)
    REG_BG0HOFS_SUB = 0; REG_BG0VOFS_SUB = 0;
    REG_BG1HOFS_SUB = 0; REG_BG1VOFS_SUB = 0;
    REG_BG2HOFS_SUB = 0; REG_BG2VOFS_SUB = 0;
    REG_BG3HOFS_SUB = 0; REG_BG3VOFS_SUB = 0;

    // Reset main screen scroll registers (Top Screen)
    REG_BG0HOFS = 0; REG_BG0VOFS = 0;
    REG_BG1HOFS = 0; REG_BG1VOFS = 0;
    REG_BG2HOFS = 0; REG_BG2VOFS = 0;
    REG_BG3HOFS = 0; REG_BG3VOFS = 0;
}

void get_user_input(const char* prompt, char* buffer, int max_len) {
    // 1. Initialize the keyboard on the bottom screen (Sub engine)
    // Since there is NO text console on the bottom screen anymore, 
    // the keyboard gets 100% of the VRAM. No more sliding, no more artifacts!
    Keyboard* kb = keyboardGetDefault();
    keyboardInit(kb, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
    
    // NDS-BOOTSTRAP FIX: Force the keyboard's internal background scroll to 0
    bgSetScroll(kb->background, 0, 0);
    bgUpdate();

    keyboardShow();
    
    // 2. The text prompt will now print safely to the TOP screen
    printf("\n%s\n> ", prompt);
    
    int index = 0;
    while(1) {
        swiWaitForVBlank();
        int c = keyboardUpdate();
        
        if (c == '\n') { 
            buffer[index] = '\0';
            printf("\n"); // Move to next line on top screen
            break;
        } else if (c == '\b' && index > 0) {
            index--;
            printf("\b \b"); // Erase character on top screen
        } else if (c > 0 && index < max_len - 1) {
            buffer[index++] = (char)c;
            printf("%c", (char)c); // Show typed character on top screen
        }
    }

    // Hide keyboard when done
    keyboardHide();
}

int main(void) {
    // Clean up dirty hardware state left by nds-bootstrap / TWiLight Menu
    sanitize_hardware();

    // Initialize Video and VRAM Banks correctly
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    // ONLY initialize a console for the TOP screen. 
    // We leave the bottom screen completely empty for the keyboard.
    PrintConsole topScreen;
    consoleInit(&topScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    consoleSelect(&topScreen);

    printf("--- DSi Chat Setup ---\n");

    // The input function will use the bottom screen for the keyboard,
    // but it will print our typing onto the top screen!
    get_user_input("Enter Username:", chat_user, 32);
    get_user_input("Enter Password:", chat_pass, 32);

    printf("\nUser: %s\nConnecting...\n", chat_user);

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