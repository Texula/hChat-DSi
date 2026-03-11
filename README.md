# hChat-DS (Hreniuc Chat for Nintendo DS)

hChat-DS is a native homebrew client for the Nintendo DS that connects directly to the Hreniuc Chat web platform. It bridges the gap between legacy 2004 hardware and modern web technologies, enabling real-time communication with browser users directly from a Nintendo DS console.

---

## Important Prerequisites

Please review the following requirements and limitations prior to downloading and executing this application:

* **Web Account Requirement:** Account registration cannot be performed from the Nintendo DS. Users must first create an account at [hreniuc.net](https://hreniuc.net).
* **Wi-Fi Limitations:** The Nintendo DS hardware exclusively supports Open or WEP security protocols; it cannot connect to modern WPA2/WPA3 networks. Users will need to utilize a compatible guest network, a legacy router, or an unsecured mobile hotspot.
* **Security Warning:** Due to the hardware limitations of the Nintendo DS regarding modern SSL/TLS encryption, login credentials are transmitted to the proxy bridge via raw TCP. 
    > **Warning:** Do not use sensitive passwords (such as those used for banking or primary personal accounts) for your Hreniuc Chat account.

---

## Features

* **Dual-Screen User Interface:** Global chat messages are displayed clearly on the Top Screen, while the typing console and keyboard are strictly isolated on the Bottom Screen to prevent visual tearing.
* **Hardware Compatibility:** Fully functional on the DS Phat, DS Lite, DSi, and 3DS. It is compatible with both physical Flashcarts (R4) and nds-bootstrap / TWiLight Menu++.
* **Real-Time Synchronization:** Communicates directly with the hreniuc.net WebSocket backend via a dedicated TCP proxy bridge.
* **60 FPS Keyboard:** Utilizes advanced asynchronous `ioctl` polling to prevent the `dswifi` library from freezing the console, ensuring consistent and responsive stylus typing input.
* **Cross-Platform Communication:** Facilitates bidirectional messaging between Nintendo DS users and web users, including display names and assigned user colors.

---

## Installation and Usage

1.  Navigate to the **[Releases](../../releases)** tab and download the latest `hChat-DS_vX.X.X.nds` compiled file.
2.  Transfer the `.nds` file to the SD card of your flashcart (or the DSi/3DS SD card if utilizing TWiLight Menu++).
3.  Connect the Nintendo DS console to a compatible Wi-Fi network via the system settings.
4.  Launch the application, input your hreniuc.net username and password, and connect to the chat.

---

## Architecture

Establishing a direct connection between a Nintendo DS and a modern Secure WebSocket (WSS) is not feasible due to hardware constraints. This project implements a three-part architecture to resolve this:

1.  **The DS Client (`main.c`):** Utilizes `libnds` and `dswifi9` to establish a Wi-Fi connection and open a raw, non-blocking TCP socket to the Proxy Bridge.
2.  **The Proxy Bridge (`ds_bridge.js`):** A lightweight Node.js server (operating on port 5050) that intercepts the raw TCP stream from the Nintendo DS. It translates custom piped commands (e.g., `LOGIN|user|pass`) into JSON payloads, decodes HTML entities, and forwards the data to the main Web App via WebSockets.
3.  **The Web App (`server.js`):** The primary Express/WebSocket server hosting hreniuc.net. It authenticates credentials against a PostgreSQL database and broadcasts messages back through the bridge to the Nintendo DS client.

---

## Compiling from Source

To fork this project or compile the binary manually, the **devkitPro** environment with the **devkitARM** toolchain is required.

**Dependencies:**
* `libnds`
* `dswifi9`

**Build Instructions:**

1. Clone the repository to your local machine:
       git clone https://github.com/texula/hChat-DS.git
       cd hChat-DS

2. Verify that a 32x32 pixel, 16-color (4-bit) `icon.bmp` file is present in the root directory. *(Note: This image must be exported without color space information).*

3. Execute the build process:
       make clean
       make

4. The compiled `.nds` executable will be generated in the root directory upon successful compilation.

---

## Credits

* Developed by **texula**
* Built utilizing [devkitPro](https://devkitpro.org/) and `libnds`
* Networking infrastructure enabled by `dswifi` (Stephen Stair)