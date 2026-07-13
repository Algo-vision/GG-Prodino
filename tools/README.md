# GRK Tools

Desktop client, API test scripts, and OTA/firmware upload utilities for the GRK board. These talk to the board over its HTTP API (see [`firmware/README.md`](../firmware/README.md) for the API reference) - they are not part of the firmware itself.

## GUI Features (Python PyQt5)

A desktop GUI application (`gui_main.py`) is provided for easy interaction with the GRK board device.
It includes the following enhancements:

- **Responsive Layout:** The GUI window starts maximized and adapts to any screen size. All content is scrollable, ensuring accessibility on different displays.
- **LED Override Checkbox:** Allows manual control of the IO LED color directly from the GUI. When unchecked, the LED automatically returns to AUTO mode, resuming automatic firmware logic based on system status.
- **IP Configuration Fields:** Dedicated fields to modify the controller's IP address and two whitelisted IP addresses.
  - Changing the controller's IP will trigger a popup notification and redirect to the Login screen, requiring reconnection at the new IP.
- **Firmware and GUI Version Display:** Displays the firmware version from the controller on the main screen and the GUI application version on the login screen.
- **Communication Loss Handling:**
  - If communication with the controller is lost while on the main screen, the GUI automatically returns to the Login screen.
  - Attempting to log in without controller communication will display a popup notification.
- **Relay Auto-Reset Indication:** Relays 0 and 1 automatically turn OFF after 5 seconds when activated, providing visual feedback in the GUI.

### Building the GUI Executable

To create a standalone executable for the GUI application, follow these steps:

1. **Install dependencies:**
   ```sh
   pip install -r requirements.txt
   ```
2. **Build the executable (from this `tools/` directory):**
   ```sh
   pyinstaller --onefile --name "GGC-HLC-<version>" gui_main.py
   ```
   (Note: The `--distpath` argument can be used to specify an output directory, e.g., `--distpath ../dist` to place it in the project's root `dist` folder.)
3. **Find the executable:**
   The executable will be located in the `dist/` directory (e.g., `dist/GG-GRK-GUI` on Linux or `dist/GG-GRK-GUI.exe` on Windows).

## Python API Tester
See `gg_api_tester.py` for example usage of the API from Python.

## Other Scripts
- `jetson_diagnostics.py` - Network diagnostics when connecting to the GRK board from a Jetson.
- `firmware_uploader.py` / `ota_uploader.py` - OTA firmware upload helpers.
- `test_multi_client_fast_stagger.py`, `test_multi_ip_clients.py`, `test_read_speed_limit.py` - API load/stress test scripts.

## GUI Version
The GUI version is displayed on the Login screen.
