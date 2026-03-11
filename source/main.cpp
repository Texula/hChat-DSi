#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dswifi9.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define SERVER_PORT 5050
#define SERVER_IP "37.143.163.56"

#define CHAT_USER "dsi_user"
#define CHAT_PASS "mypassword"

int main(void) {

    videoSetMode(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);

    PrintConsole topScreen;
    consoleInit(&topScreen, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);

    consoleSelect(&topScreen);

    printf("DSi Chat Test\n");
    printf("Connecting WiFi...\n");

    if (!Wifi_InitDefault(WFC_CONNECT)) {
        printf("WiFi failed\n");
        while (1) swiWaitForVBlank();
    }

    while (Wifi_AssocStatus() != ASSOCSTATUS_ASSOCIATED) {
        swiWaitForVBlank();
    }

    struct in_addr ip = Wifi_GetIPInfo(NULL, NULL, NULL, NULL);
    printf("IP: %s\n", inet_ntoa(ip));

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    printf("Connecting to bridge...\n");

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Bridge offline\n");
        while (1) swiWaitForVBlank();
    }

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    char loginCmd[256];
    sprintf(loginCmd, "LOGIN|%s|%s\n", CHAT_USER, CHAT_PASS);
    send(sock, loginCmd, strlen(loginCmd), 0);

    printf("Authenticating...\n");

    bool loggedIn = false;

    char netBuffer[1024];

    int frameCounter = 0;

    while (1) {

        scanKeys();
        int pressed = keysDown();

        // PRESS A TO SEND MESSAGE
        if (loggedIn && (pressed & KEY_A)) {

            char msg[] = "MSG|Hello!\n";
            send(sock, msg, strlen(msg), 0);

            printf("Sent Hello!\n");
        }

        // NETWORK POLL
        frameCounter++;

        if (frameCounter > 20) {

            frameCounter = 0;

            int bytes = recv(sock, netBuffer, sizeof(netBuffer) - 1, 0);

            if (bytes > 0) {

                netBuffer[bytes] = '\0';

                if (!loggedIn) {

                    if (strstr(netBuffer, "OK|LOGIN")) {

                        loggedIn = true;

                        printf("Logged in!\n");
                        printf("Press A to send Hello!\n");
                    }

                    else {

                        printf("%s", netBuffer);
                    }

                } else {

                    printf("%s", netBuffer);
                }
            }
        }

        swiWaitForVBlank();
    }

    return 0;
}