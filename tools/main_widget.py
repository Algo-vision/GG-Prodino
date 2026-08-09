from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QLabel, QPushButton, QHBoxLayout, QTableWidget, QTableWidgetItem, QComboBox, QFileDialog, QMessageBox, QGroupBox, QGridLayout, QLineEdit, QCheckBox, QScrollArea)
from PyQt5.QtCore import QTimer, pyqtSignal, Qt
from firmware_uploader import upload_firmware
import time
import random
import socket
import threading
import json

# Display label -> firmware wire value, for the IMU Mount Orientation control.
# See firmware/include/i2c_imu_gps.hpp (ImuMountOrientation) for what each
# orientation means; the "Forward/Backward/Left/Right" labels are a naming
# convention that should be verified against the real MSB hardware.
IMU_ORIENTATION_LABELS = {
    "Standing (Default)": "STANDING",
    "Tilted Forward": "TILT_FORWARD",
    "Tilted Backward": "TILT_BACKWARD",
    "Tilted Left": "TILT_LEFT",
    "Tilted Right": "TILT_RIGHT",
}
IMU_ORIENTATION_LABELS_REVERSE = {v: k for k, v in IMU_ORIENTATION_LABELS.items()}

class MainWidget(QWidget):
    reconnect_requested = pyqtSignal()
    upload_finished_signal = pyqtSignal(bool, str)
    status_update_signal = pyqtSignal(dict) # New signal for UDP updates
    
    def __init__(self, api_client, base_ip, parent=None, client_offset_ms=0, polling_interval_ms=50): # 1 second polling
        super().__init__(parent)
        self.api_client = api_client
        self.base_ip = base_ip
        self.client_offset_ms = client_offset_ms
        self.polling_interval_ms = polling_interval_ms
        self.technician_mode = False
        self.firmware_path = None
        self.is_editing_ip = False
        self.waiting_for_reset = False  # Flag to indicate we're waiting for board reset
        print(f"MainWidget.__init__: api_client.base_url is {self.api_client.base_url}")
        print(f"MainWidget.__init__: client_offset={client_offset_ms}ms, polling_interval={polling_interval_ms}ms")
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_status)
        self.status_ignore_deadline = 0  # Timestamp until which to ignore status updates (for reboot)
        self.init_ui()
        # One-shot sync of slow-changing config (currently just IMU mount
        # orientation) - not part of the fast get_status/UDP polling loop.
        QTimer.singleShot(0, self.sync_imu_mount_orientation)
        # Start polling with offset (if specified) for multi-client scenarios
        if self.client_offset_ms > 0:
            print(f"MainWidget: Starting polling with {self.client_offset_ms}ms offset...")
            QTimer.singleShot(self.client_offset_ms, self.start_polling)
        else:
            self.start_polling()
        self.technician_mode = False
        self.upload_finished_signal.connect(self.show_upload_result)
        self.status_update_signal.connect(self.handle_udp_status)
        
        # UDP Listener
        self.udp_socket = None
        self.udp_thread = None
        self.udp_running = False
        self.start_udp_listener()
    
    def start_polling(self):
        """Start the status polling timer (Fallback / Auth check)"""
        # We now use UDP for main data, but keep a slow poll for auth check
        print(f"MainWidget: Status polling started (interval={self.polling_interval_ms}ms)")
        self.timer.start(self.polling_interval_ms)
        # self.update_status()  # Don't force immediate HTTP update, wait for UDP

    def start_udp_listener(self):
        self.udp_running = True
        self.udp_thread = threading.Thread(target=self.udp_listen_loop)
        self.udp_thread.daemon = True
        self.udp_thread.start()

    def stop_udp_listener(self):
        self.udp_running = False
        if self.udp_socket:
            self.udp_socket.close()

    def udp_listen_loop(self):
        UDP_PORT = 5000
        try:
            self.udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.udp_socket.bind(('', UDP_PORT))
            print(f"UDP Listener started on port {UDP_PORT}")
            
            while self.udp_running:
                try:
                    data, addr = self.udp_socket.recvfrom(1024)
                    status = json.loads(data.decode())
                    self.status_update_signal.emit(status)
                    
                except socket.timeout:
                    continue
                except Exception as e:
                    print(f"UDP Error: {e}")
                    if not self.udp_running: break
                    time.sleep(1)
        except Exception as e:
            print(f"UDP Setup Error: {e}")

    def handle_udp_status(self, status):
        """Update UI from UDP status packet"""
        if not status: return
        
        for k, v in self.status_labels.items():
            val = status.get(k, "-")
            if isinstance(val, list):
                val = ", ".join(str(x) for x in val)
            v.setText(str(val))
            
        # Enable technician mode section if in technician mode
        self.technician_mode = status.get("technicianMode", False)
        self.tech_mode_box.setEnabled(self.technician_mode)
        self.fw_upload_btn.setEnabled(self.technician_mode and self.firmware_path is not None)

        # Update IP configuration fields only if not editing
        if not self.is_editing_ip:
            self.controller_ip_edit.setText(status.get("controllerIp", self.base_ip))
            whitelist = status.get("whitelistIps", [])
            self.whitelist_ip1_edit.setText(whitelist[0] if len(whitelist) > 0 else "")
            self.whitelist_ip2_edit.setText(whitelist[1] if len(whitelist) > 1 else "")
            self.whitelist_ip3_edit.setText(whitelist[2] if len(whitelist) > 2 else "192.168.1.33")
            # Update router IP field
            self.router_ip_edit.setText(status.get("routerIp", "192.168.1.1"))

        # If manual override is not active, update LED combo from status
        if not self.led_override_checkbox.isChecked():
            current_led_status = status.get("ledIo", "OFF") if status else "OFF"
            index = self.led_combo.findText(current_led_status)
            if index != -1:
                self.led_combo.setCurrentIndex(index)

    def on_ip_editing_started(self):
        self.is_editing_ip = True

    def init_ui(self):
        # Create a container widget for all content
        container = QWidget()
        layout = QVBoxLayout()
        # Status group
        self.status_group = QGroupBox("Device Status")
        status_layout = QGridLayout()
        self.status_labels = {}
        # Add IP fields to the status fields list
        fields = ["firmwareVersion", "controllerIp", "whitelistIps", "motorWorkHours", "busVoltage", "powerConnected", "relays_status", "imuX", "imuY", "imuZ", "imuGx", "imuGy", "imuGz", "pitch", "roll", "yaw", "gpsLat", "gpsLng", "gpsAlt", "gpsTime", "gpsSpeedNorth", "gpsSpeedEast", "gpsSpeedDown", "gpsGroundSpeed", "gpsHeading", "gpsSatellites", "ledInternal", "ledIo", "button_tech", "imuValid", "GPSConnected", "optoin_status", "gpsSane", "imu1Sane", "imu2Sane", "angleSane", "imuTemp", "ocuConnected", "safetyMode", "systemCurrent_A"]
        for i, field in enumerate(fields):
            label = QLabel("-")
            status_layout.addWidget(QLabel(field), i, 0)
            status_layout.addWidget(label, i, 1)
            self.status_labels[field] = label
        self.status_group.setLayout(status_layout)
        layout.addWidget(self.status_group)

        # IP Configuration Group
        ip_config_box = QGroupBox("IP Configuration")
        ip_config_layout = QGridLayout()

        self.controller_ip_edit = QLineEdit(self.base_ip)
        self.whitelist_ip1_edit = QLineEdit()
        self.whitelist_ip2_edit = QLineEdit()
        self.whitelist_ip3_edit = QLineEdit("192.168.1.33")  # Default third IP

        self.controller_ip_edit.textChanged.connect(self.on_ip_editing_started)
        self.whitelist_ip1_edit.textChanged.connect(self.on_ip_editing_started)
        self.whitelist_ip2_edit.textChanged.connect(self.on_ip_editing_started)
        self.whitelist_ip3_edit.textChanged.connect(self.on_ip_editing_started)

        ip_config_layout.addWidget(QLabel("Controller IP:"), 0, 0)
        ip_config_layout.addWidget(self.controller_ip_edit, 0, 1)
        ip_config_layout.addWidget(QLabel("Whitelist IP 1:"), 1, 0)
        ip_config_layout.addWidget(self.whitelist_ip1_edit, 1, 1)
        ip_config_layout.addWidget(QLabel("Whitelist IP 2:"), 2, 0)
        ip_config_layout.addWidget(self.whitelist_ip2_edit, 2, 1)
        ip_config_layout.addWidget(QLabel("Whitelist IP 3:"), 3, 0)
        ip_config_layout.addWidget(self.whitelist_ip3_edit, 3, 1)

        self.save_ip_btn = QPushButton("Save IP Configuration")
        self.save_ip_btn.clicked.connect(self.save_ip_configuration)
        ip_config_layout.addWidget(self.save_ip_btn, 4, 0, 1, 2)

        ip_config_box.setLayout(ip_config_layout)
        ip_config_box.setLayout(ip_config_layout)
        layout.addWidget(ip_config_box)

        # Router IP Configuration
        router_config_box = QGroupBox("Router Configuration")
        router_config_layout = QGridLayout()
        
        self.router_ip_edit = QLineEdit("192.168.1.1")
        self.router_ip_edit.textChanged.connect(self.on_ip_editing_started)
        
        router_config_layout.addWidget(QLabel("Router IP:"), 0, 0)
        router_config_layout.addWidget(self.router_ip_edit, 0, 1)
        
        self.save_router_ip_btn = QPushButton("Save Router IP")
        self.save_router_ip_btn.clicked.connect(self.save_router_ip)
        router_config_layout.addWidget(self.save_router_ip_btn, 1, 0, 1, 2)
        
        router_config_box.setLayout(router_config_layout)
        layout.addWidget(router_config_box)

        # Technician Mode Section - contains Serial Number and Firmware Uploader
        self.tech_mode_box = QGroupBox("Technician Mode (Hold button during startup to enable)")
        tech_mode_layout = QVBoxLayout()
        
        # Serial Number subsection - 4 separate digit slots
        sn_layout = QHBoxLayout()
        self.sn_label = QLabel("Current SN: Unknown")
        sn_layout.addWidget(self.sn_label)
        
        # Add "SN" prefix label
        sn_layout.addWidget(QLabel("Set SN:"))
        
        # First digit is fixed to "2" (range 2000-2999)
        self.sn_digit1 = QLineEdit("2")
        self.sn_digit1.setMaxLength(1)
        self.sn_digit1.setFixedWidth(35)
        self.sn_digit1.setAlignment(Qt.AlignCenter)
        self.sn_digit1.setEnabled(False)  # Fixed, cannot change
        self.sn_digit1.setStyleSheet("background-color: #e0e0e0;")
        sn_layout.addWidget(self.sn_digit1)
        
        # Digits 2, 3, 4 (each accepts 0-9)
        self.sn_digit2 = QLineEdit()
        self.sn_digit3 = QLineEdit()
        self.sn_digit4 = QLineEdit()
        
        self.sn_digits = [self.sn_digit2, self.sn_digit3, self.sn_digit4]
        for i, digit_input in enumerate(self.sn_digits):
            digit_input.setMaxLength(1)
            digit_input.setFixedWidth(35)
            digit_input.setAlignment(Qt.AlignCenter)
            digit_input.setPlaceholderText(str(i))
            # Only allow digits 0-9
            digit_input.textChanged.connect(self.on_sn_digit_changed)
            sn_layout.addWidget(digit_input)
        
        # Auto-advance to next digit
        self.sn_digit2.textChanged.connect(lambda t: self.sn_digit3.setFocus() if t.isdigit() else None)
        self.sn_digit3.textChanged.connect(lambda t: self.sn_digit4.setFocus() if t.isdigit() else None)
        
        self.set_sn_btn = QPushButton("Set SN")
        self.set_sn_btn.clicked.connect(self.set_serial_number)
        sn_layout.addWidget(self.set_sn_btn)
        
        sn_layout.addStretch()  # Push everything to the left
        tech_mode_layout.addLayout(sn_layout)
        
        # Firmware uploader subsection
        fw_layout = QHBoxLayout()
        self.fw_path_label = QLabel("No file selected")
        self.fw_select_btn = QPushButton("Select Firmware")
        self.fw_select_btn.clicked.connect(self.select_firmware)
        self.fw_upload_btn = QPushButton("Upload")
        self.fw_upload_btn.clicked.connect(self.upload_firmware)
        self.fw_upload_btn.setEnabled(False)
        fw_layout.addWidget(self.fw_path_label)
        fw_layout.addWidget(self.fw_select_btn)
        fw_layout.addWidget(self.fw_upload_btn)
        tech_mode_layout.addLayout(fw_layout)
        
        self.tech_mode_box.setLayout(tech_mode_layout)
        self.tech_mode_box.setEnabled(False)  # Disabled until technician mode is active
        layout.addWidget(self.tech_mode_box)

        # IMU Mount Orientation - lets a technician correct axis interpretation
        # if the HLC is mounted lying down instead of standing (the default).
        imu_orientation_box = QGroupBox("IMU Mount Orientation")
        imu_orientation_layout = QHBoxLayout()
        self.imu_orientation_combo = QComboBox()
        self.imu_orientation_combo.addItems(list(IMU_ORIENTATION_LABELS.keys()))
        self.imu_orientation_btn = QPushButton("Set Orientation")
        self.imu_orientation_btn.clicked.connect(self.set_imu_mount_orientation)
        imu_orientation_layout.addWidget(self.imu_orientation_combo)
        imu_orientation_layout.addWidget(self.imu_orientation_btn)
        imu_orientation_box.setLayout(imu_orientation_layout)
        layout.addWidget(imu_orientation_box)

        # ---- Telemetry key -------------------------------------------------
        # The board encrypts its cloud telemetry with a per-board secret. Create
        # the key on the dashboard (Admin -> Board Telemetry Keys), which also
        # registers it with the server, then paste it here to burn it into the
        # board. Write-only: nothing can read a key back out, so the box only
        # ever reports whether one is present.
        key_box = QGroupBox("Telemetry Key (encrypts the status sent to the cloud)")
        key_layout = QVBoxLayout()

        key_status_row = QHBoxLayout()
        self.key_status_label = QLabel("Key on this board: unknown")
        key_status_row.addWidget(self.key_status_label)
        key_status_row.addStretch()
        key_layout.addLayout(key_status_row)

        key_hint = QLabel(
            "Create the key on the dashboard (Admin \u2192 Board Telemetry Keys), "
            "then paste it below. Accepts the whole SET_KEY: line or just the 64 "
            "hex characters."
        )
        key_hint.setWordWrap(True)
        key_hint.setStyleSheet("color: #666; font-size: 11px;")
        key_layout.addWidget(key_hint)

        key_entry_row = QHBoxLayout()
        self.key_edit = QLineEdit()
        self.key_edit.setPlaceholderText("SET_KEY:<64 hex characters>")
        key_entry_row.addWidget(self.key_edit)
        self.key_burn_btn = QPushButton("Burn Key to Board")
        self.key_burn_btn.clicked.connect(self.burn_device_key)
        key_entry_row.addWidget(self.key_burn_btn)
        key_layout.addLayout(key_entry_row)

        key_box.setLayout(key_layout)
        layout.addWidget(key_box)

        # Relay controls
        relay_box = QGroupBox("Relays")
        relay_layout = QHBoxLayout()
        self.relay_buttons = []
        for i in range(4):
            if (i==0 or i==1):
                btn = QPushButton(f"Toggle Relay {i} (5 sec)")
            else:
                btn = QPushButton(f"Toggle Relay {i}")
            btn.clicked.connect(lambda _, idx=i: self.toggle_relay(idx))
            relay_layout.addWidget(btn)
            self.relay_buttons.append(btn)
        relay_box.setLayout(relay_layout)
        layout.addWidget(relay_box)

        # LED controls
        led_box = QGroupBox("IO LED")
        led_layout = QHBoxLayout()
        self.led_combo = QComboBox()
        self.led_combo.addItems(["AUTO", "OFF", "GREEN", "RED", "ORANGE"])
        self.led_btn = QPushButton("Set IO LED")
        self.led_btn.clicked.connect(self.set_led)
        led_layout.addWidget(self.led_combo)
        led_layout.addWidget(self.led_btn)
        self.led_override_checkbox = QCheckBox("Manual LED Override")
        self.led_override_checkbox.stateChanged.connect(self.toggle_led_controls)
        led_layout.addWidget(self.led_override_checkbox)
        led_box.setLayout(led_layout)
        layout.addWidget(led_box)

        # Internal LED
        int_led_box = QGroupBox("Internal LED")
        int_led_layout = QHBoxLayout()
        self.int_led_on_btn = QPushButton("ON")
        self.int_led_off_btn = QPushButton("OFF")
        self.int_led_on_btn.clicked.connect(lambda: self.set_internal_led(True))
        self.int_led_off_btn.clicked.connect(lambda: self.set_internal_led(False))
        int_led_layout.addWidget(self.int_led_on_btn)
        int_led_layout.addWidget(self.int_led_off_btn)
        int_led_box.setLayout(int_led_layout)
        layout.addWidget(int_led_box)

        # Set layout on container
        container.setLayout(layout)
        
        # Create scroll area and make it resizable
        scroll = QScrollArea()
        scroll.setWidget(container)
        scroll.setWidgetResizable(True)  # KEY: allows content to resize with window
        
        # Set scroll area as the main layout
        main_layout = QVBoxLayout()
        main_layout.addWidget(scroll)
        self.setLayout(main_layout)
        
        self.firmware_path = None
        
        # Initial fetch of serial number
        QTimer.singleShot(1000, self.refresh_serial_number)

    def update_status(self):
        # Check if we should ignore status updates (e.g. while waiting for reboot)
        if time.time() < self.status_ignore_deadline:
            if self.waiting_for_reset:
                print("[GUI] Ignoring status (waiting for reboot)...")
            return

        status = self.api_client.get_status()
        if status and status.get("error") == "AUTH_ERROR":
            # If waiting for reset, this is expected - don't show error
            if self.waiting_for_reset:
                print("[GUI] Board is resetting, waiting for reconnection...")
                return
            self.timer.stop()
            QMessageBox.warning(self, "Authentication Error", "Invalid session token. Please log in again.")
            self.reconnect_requested.emit()
        elif status:
            # Successfully got status - reset the waiting flag
            if self.waiting_for_reset:
                print("[GUI] Board is back online, refreshing serial number...")
                self.waiting_for_reset = False
                self.refresh_serial_number()
            
            for k, v in self.status_labels.items():
                val = status.get(k, "-")
                if isinstance(val, list):
                    val = ", ".join(str(x) for x in val)
                v.setText(str(val))
            # Enable technician mode section if in technician mode
            self.technician_mode = status.get("technicianMode", False)
            self.tech_mode_box.setEnabled(self.technician_mode)

            has_key = status.get("deviceKeySet")
            if has_key is None:
                self.key_status_label.setText(
                    "Key on this board: unknown (firmware too old to report it)")
                self.key_status_label.setStyleSheet("color: #888;")
            elif has_key:
                self.key_status_label.setText("Key on this board: SET \u2713")
                self.key_status_label.setStyleSheet("color: #226b45; font-weight: bold;")
            else:
                self.key_status_label.setText(
                    "Key on this board: NOT SET - it cannot report to the cloud")
                self.key_status_label.setStyleSheet("color: #9c5b00; font-weight: bold;")
            self.fw_upload_btn.setEnabled(self.technician_mode and self.firmware_path is not None)

            # Update IP configuration fields only if not editing
            if not self.is_editing_ip:
                self.controller_ip_edit.setText(status.get("controllerIp", self.base_ip))
                whitelist = status.get("whitelistIps", [])
                self.whitelist_ip1_edit.setText(whitelist[0] if len(whitelist) > 0 else "")
                self.whitelist_ip2_edit.setText(whitelist[1] if len(whitelist) > 1 else "")
                # Update router IP field
                self.router_ip_edit.setText(status.get("routerIp", "192.168.1.1"))

            # If manual override is not active, update LED combo from status
            if not self.led_override_checkbox.isChecked():
                current_led_status = status.get("ledIo", "OFF") if status else "OFF"
                index = self.led_combo.findText(current_led_status)
                if index != -1:
                    self.led_combo.setCurrentIndex(index)
            
            # Add small jitter to prevent collision with other clients
            # Random jitter of ±10% of polling interval
            jitter = random.randint(-self.polling_interval_ms // 10, self.polling_interval_ms // 10)
            next_interval = self.polling_interval_ms + jitter
            # Keep interval at least 50ms (board can handle ~20 RPS) and at most double the configured interval
            next_interval = max(50, min(self.polling_interval_ms * 2, next_interval))
            self.timer.setInterval(next_interval)

        else:
            # Communication lost
            if self.waiting_for_reset:
                # This is expected during reset - just wait
                print("[GUI] Waiting for board to come back online...")
                return
            
            # Unexpected communication loss - show error
            self.timer.stop()
            QMessageBox.critical(self, "Error", "Failed to communicate with device.")
            self.reconnect_requested.emit()
            for v in self.status_labels.values():
                v.setText("-")

    def toggle_led_controls(self, state):
        # Enable/disable LED combo and button based on checkbox state
        is_checked = bool(state)
        self.led_combo.setEnabled(is_checked)
        self.led_btn.setEnabled(is_checked)

        # If override is turned off, send AUTO command to resume automatic LED control
        if not is_checked:
            # Send AUTO command to device to resume automatic LED logic
            self.api_client.set_led("AUTO")
            # Set combo to AUTO and update status
            self.led_combo.setCurrentText("AUTO")
            self.update_status() # This will refresh the led_combo based on device status

    def toggle_relay(self, relay_id):
        # Get current relay state from status label
        relays = self.status_labels["relays_status"].text().split(", ")
        if len(relays) > relay_id:
            current = relays[relay_id]
            corrent_bool = current == 'True'
            new_state =  not corrent_bool
            self.api_client.set_relay(relay_id, new_state)
            self.update_status()

    def set_led(self):
        if self.led_override_checkbox.isChecked():
            color = self.led_combo.currentText()
            self.api_client.set_led(color)
            self.update_status()
        else:
            QMessageBox.warning(self, "LED Control", "Manual LED override is not active. Check the 'Manual LED Override' box to control the LED.")

    def set_internal_led(self, state):
        self.api_client.set_internal_led(state)
        self.update_status()

    def set_imu_mount_orientation(self):
        label = self.imu_orientation_combo.currentText()
        orientation = IMU_ORIENTATION_LABELS.get(label, "STANDING")

        ok, msg = self.api_client.set_imu_mount_orientation(orientation)
        if ok:
            QMessageBox.information(self, "IMU Mount Orientation",
                f"{msg}\n\nNote: Reboot the device to recalibrate for the new orientation.")
        else:
            if msg == "Authentication Error":
                self.timer.stop()
                QMessageBox.warning(self, "Authentication Error", "Invalid session token. Please log in again.")
                self.reconnect_requested.emit()
            else:
                QMessageBox.critical(self, "IMU Mount Orientation Failed", f"Failed to set IMU mount orientation: {msg}")

    def sync_imu_mount_orientation(self):
        """One-shot sync of the orientation combo from the device's current config."""
        config = self.api_client.get_config()
        if config and not config.get("error"):
            orientation = config.get("imuMountOrientation", "STANDING")
            label = IMU_ORIENTATION_LABELS_REVERSE.get(orientation, "Standing (Default)")
            index = self.imu_orientation_combo.findText(label)
            if index >= 0:
                self.imu_orientation_combo.setCurrentIndex(index)

    def select_firmware(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select Firmware", "", "Binary Files (*.bin)")
        if path:
            self.firmware_path = path
            self.fw_path_label.setText(path)
            self.fw_upload_btn.setEnabled(self.technician_mode)

    def upload_firmware(self):
        print("upload_firmware: Initiated.")
        if not self.firmware_path:
            QMessageBox.warning(self, "No file", "Please select a firmware file.")
            print("upload_firmware: No firmware file selected.")
            return
        
        self.timer.stop()
        print("upload_firmware: Status timer stopped.")

        print(f"upload_firmware: Attempting to upload firmware from {self.firmware_path} to {self.base_ip}")
        ok, msg = upload_firmware(self.base_ip, self.firmware_path)
        self.upload_finished_signal.emit(ok, msg)

    def show_upload_result(self, ok, msg):
        print(f"show_upload_result: Received signal. ok={ok}, msg={msg}")
        if ok:
            print("show_upload_result: Upload was successful. Preparing to show success message.")
            QMessageBox.information(self, "Upload", "Firmware upload successful! The device is rebooting. Please reconnect manually after a few seconds.")
            print("show_upload_result: Success message box closed. Emitting reconnect_requested after 3 seconds.")
            QTimer.singleShot(3000, self.reconnect_requested.emit)
        else:
            print(f"show_upload_result: Upload failed. Preparing to show error message: {msg}")
            QMessageBox.critical(self, "Upload Failed", f"Upload failed: {msg}")
            print("show_upload_result: Error message box closed. Restarting status timer.")
            self.timer.start(self.polling_interval_ms)


    def save_ip_configuration(self):
        print("save_ip_configuration: Initiated.")
        self.is_editing_ip = False
        controller_ip = self.controller_ip_edit.text()
        whitelist_ips = [
            self.whitelist_ip1_edit.text(),
            self.whitelist_ip2_edit.text(),
            self.whitelist_ip3_edit.text()
        ]
        # Filter out empty whitelist IPs
        whitelist_ips = [ip for ip in whitelist_ips if ip]
        print(f"save_ip_configuration: Attempting to set controller_ip={controller_ip}, whitelist_ips={whitelist_ips}")

        ok, msg = self.api_client.set_ip_config(controller_ip, whitelist_ips)
        if ok:
            print(f"save_ip_configuration: IP configuration saved successfully. New IP: {controller_ip}")
            self.base_ip = controller_ip
            self.reconnect_requested.emit() # Emit signal first to clean up MainWidget
            QMessageBox.information(self, "IP Configuration", f"IP configuration saved. Reconnection required. New IP: {controller_ip}")
        else:
            if msg == "Authentication Error":
                self.timer.stop()
                QMessageBox.warning(self, "Authentication Error", "Invalid session token. Please log in again.")
                self.reconnect_requested.emit()
            else:
                print(f"save_ip_configuration: Failed to save IP configuration: {msg}")
                QMessageBox.critical(self, "IP Configuration Failed", f"Failed to save IP configuration: {msg}")
                self.update_status() # Refresh to show the original IPs

    def save_router_ip(self):
        print("save_router_ip: Initiated.")
        self.is_editing_ip = False
        router_ip = self.router_ip_edit.text()
        
        if not router_ip:
            QMessageBox.warning(self, "Invalid IP", "Please enter a router IP address.")
            return
            
        print(f"save_router_ip: Attempting to set router_ip={router_ip}")
        
        ok, msg = self.api_client.set_router_ip(router_ip)
        if ok:
            print(f"save_router_ip: Router IP saved successfully: {router_ip}")
            QMessageBox.information(self, "Router IP Configuration", 
                f"Router IP updated to: {router_ip}\n\nNote: Reboot the device for the new IP to take effect.")
        else:
            if msg == "Authentication Error":
                self.timer.stop()
                QMessageBox.warning(self, "Authentication Error", "Invalid session token. Please log in again.")
                self.reconnect_requested.emit()
            else:
                print(f"save_router_ip: Failed to save router IP: {msg}")
                QMessageBox.critical(self, "Router IP Configuration Failed", f"Failed to save router IP: {msg}")
                self.update_status() # Refresh to show the original IP

    def burn_device_key(self):
        """Send a telemetry key to the board and burn it into flash."""
        raw = self.key_edit.text().strip()
        if raw.upper().startswith("SET_KEY:"):        # accept the pasted line as-is
            raw = raw.split(":", 1)[1].strip()
        raw = raw.replace(" ", "")

        if len(raw) != 64 or any(c not in "0123456789abcdefABCDEF" for c in raw):
            QMessageBox.warning(
                self, "Invalid key",
                "A telemetry key is exactly 64 hex characters (32 bytes).\n\n"
                f"That input has {len(raw)} character(s).\n\n"
                "Create one on the dashboard under Admin \u2192 Board Telemetry Keys "
                "and paste the SET_KEY: line here.")
            return

        if QMessageBox.question(
                self, "Burn key to board",
                "Write this key into the board's flash?\n\n"
                "It replaces any existing key, and the board will only be accepted "
                "by the server if the same key is registered there for this serial "
                "number.\n\nReboot the board afterwards to start using it.",
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No) != QMessageBox.Yes:
            return

        result = self.api_client.set_device_key(raw)
        if result is None:
            QMessageBox.critical(self, "No response",
                                 "The board did not answer. Check the connection.")
            return
        if result.get("error") == "AUTH_ERROR":
            QMessageBox.critical(self, "Not logged in",
                                 "Session expired - log in again and retry.")
            return
        if result.get("success"):
            self.key_edit.clear()
            QMessageBox.information(
                self, "Key burned",
                "The key is stored on the board.\n\n"
                "Reboot the board to start using it. The key cannot be read back "
                "out - this panel will only show whether one is present.")
        else:
            QMessageBox.critical(
                self, "Failed",
                result.get("message", "The board rejected the key."))

    def on_sn_digit_changed(self):
        """Validate that only digits 0-9 are allowed in SN input slots."""
        sender = self.sender()
        if sender and sender.text() and not sender.text().isdigit():
            sender.setText("")  # Clear if not a digit

    def set_serial_number(self):
        # Read from 4 individual digit slots
        d2 = self.sn_digit2.text()
        d3 = self.sn_digit3.text()
        d4 = self.sn_digit4.text()
        
        # Validate all digits are entered
        if not all([d2.isdigit(), d3.isdigit(), d4.isdigit()]):
            QMessageBox.warning(self, "Invalid Serial Number", 
                "Please enter all 3 remaining digits (0-9).\nFirst digit is fixed to '2'.")
            return
        
        # Construct full serial number
        sn = f"2{d2}{d3}{d4}"

        if not self.technician_mode:
            QMessageBox.warning(self, "Technician Mode Required", 
                "You must be in Technician Mode to set the serial number.\nHold the button on the device during startup.")
            return

        resp = self.api_client.set_serial_number(sn)
        if resp and resp.get("success"):
            # Clear the input fields
            self.sn_digit2.setText("")
            self.sn_digit3.setText("")
            self.sn_digit4.setText("")
            
            # Set flag to wait patiently for board to reset
            self.waiting_for_reset = True
            
            # Stop timer to prevent updates while message box is shown
            self.timer.stop()
            
            # Show info message
            QMessageBox.information(self, "Success", f"Serial number set to SN{sn}. Device will reboot.\nWaiting for device to come back online...")
            
            # Resume timer
            self.timer.start()
            
            # Set deadline to ignore status for 4 seconds (cover the 2s delay + reboot time)
            # This prevents us from seeing the "old" status before the board actually dies
            self.status_ignore_deadline = time.time() + 4.0
        elif resp and resp.get("error"):
            QMessageBox.critical(self, "Error", f"Failed to set serial number: {resp.get('message')}")
        else:
            QMessageBox.critical(self, "Error", "Failed to communicate with device.")

    def refresh_serial_number(self):
        resp = self.api_client.get_serial_number()
        if resp and resp.get("type") == "serial_number":
            # The firmware returns serialNumber (camelCase, like the rest of the
            # API); older builds used serial_number. Accept either.
            sn = resp.get("serialNumber") or resp.get("serial_number") or "Unknown"
            self.sn_label.setText(f"Current SN: {sn}")
            
            # Disable SN input if serial number is already set (starts with SN followed by 4 digits)
            is_configured = sn.startswith("SN") and len(sn) >= 6 and sn[2:6].isdigit()
            self.sn_digit2.setEnabled(not is_configured)
            self.sn_digit3.setEnabled(not is_configured)
            self.sn_digit4.setEnabled(not is_configured)
            self.set_sn_btn.setEnabled(not is_configured)
            
            if is_configured:
                self.sn_label.setStyleSheet("color: green; font-weight: bold;")
            else:
                self.sn_label.setStyleSheet("color: orange; font-weight: bold;")
