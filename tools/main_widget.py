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

        # Technician Mode Section - contains Serial Number and Firmware Uploader
        self.tech_mode_box = QGroupBox("Technician Mode (Hold button during startup to enable)")
        tech_mode_layout = QVBoxLayout()
        
        # NOTE: no serial-number controls here. V1.5.1 has no
        # set_serial_number / get_serial_number command - those came later -
        # so these fields could only ever have returned "unknown request
        # type". The firmware uploader below stays: OTA does exist in V1.5.1.

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

        # IMU Axis Map. V1.5.1 replaced the single "mount orientation" enum with
        # an explicit per-axis map, so the old dropdown sent a command this
        # firmware does not implement. These three choices are what
        # set_imu_axis_map takes, and the firmware rejects a map that uses one
        # axis twice.
        axis_box = QGroupBox("IMU Axis Map")
        axis_layout = QGridLayout()
        self.axis_combos = {}
        self.axis_inverts = {}
        for col, name in enumerate(("pitch", "roll", "yaw")):
            axis_layout.addWidget(QLabel(name.capitalize() + " axis:"), 0, col * 2)
            combo = QComboBox()
            combo.addItems(["X", "Y", "Z"])
            axis_layout.addWidget(combo, 0, col * 2 + 1)
            self.axis_combos[name] = combo
            inv = QCheckBox("invert")
            axis_layout.addWidget(inv, 1, col * 2 + 1)
            self.axis_inverts[name] = inv
        self.axis_apply_btn = QPushButton("Set Axis Map")
        self.axis_apply_btn.clicked.connect(self.set_imu_axis_map)
        axis_layout.addWidget(self.axis_apply_btn, 2, 0, 1, 6)
        axis_box.setLayout(axis_layout)
        layout.addWidget(axis_box)

        # ---- Calibration ---------------------------------------------------
        # The zero calibration does not gate the readings - it only moves the
        # reference. An uncalibrated board reports perfectly plausible angles
        # measured from the ENCLOSURE rather than from the machine, and nothing
        # about the numbers reveals that. Hence a status line that is impossible
        # to miss when it is wrong.
        cal_box = QGroupBox("Calibration")
        cal_layout = QVBoxLayout()

        self.cal_status_label = QLabel("Calibration: unknown")
        self.cal_status_label.setWordWrap(True)
        cal_layout.addWidget(self.cal_status_label)

        self.rest_label = QLabel("At rest: unknown")
        self.rest_label.setStyleSheet("color: #666; font-size: 11px;")
        cal_layout.addWidget(self.rest_label)

        cal_btn_row = QHBoxLayout()
        self.burn_zero_btn = QPushButton("Burn Zero Calibration")
        self.burn_zero_btn.setToolTip(
            "Declares the machine's CURRENT attitude to be level and writes it "
            "to flash. Only press this with the machine standing on flat ground.")
        self.burn_zero_btn.clicked.connect(self.burn_zero_calibration)
        cal_btn_row.addWidget(self.burn_zero_btn)

        self.calibrate_now_btn = QPushButton("Calibrate Now (clear drift)")
        self.calibrate_now_btn.setToolTip(
            "Re-seats pitch and roll on the accelerometer and zeroes yaw. "
            "Stores nothing, and is valid at any attitude - not just level.")
        self.calibrate_now_btn.clicked.connect(self.calibrate_now)
        cal_btn_row.addWidget(self.calibrate_now_btn)
        cal_layout.addLayout(cal_btn_row)

        cal_box.setLayout(cal_layout)
        layout.addWidget(cal_box)

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
            
            for k, v in self.status_labels.items():
                val = status.get(k, "-")
                if isinstance(val, list):
                    val = ", ".join(str(x) for x in val)
                v.setText(str(val))
            # Enable technician mode section if in technician mode
            self.technician_mode = status.get("technicianMode", False)
            self.tech_mode_box.setEnabled(self.technician_mode)

            # ---- Calibration state -----------------------------------------
            # Loud when it is wrong. An uncalibrated board reports believable
            # angles measured from the enclosure, so nothing about the numbers
            # themselves would tell a technician the board was never calibrated.
            imu_group = status.get("imu") or status
            valid = imu_group.get("zeroCalValid")
            if valid is None:
                self.cal_status_label.setText(
                    "Calibration: unknown (firmware does not report it)")
                self.cal_status_label.setStyleSheet("color: #888;")
            elif valid:
                self.cal_status_label.setText(
                    "Calibrated  \u2713   mounting angle: pitch %.2f\u00b0, roll %.2f\u00b0"
                    % (imu_group.get("mountPitch", 0.0), imu_group.get("mountRoll", 0.0)))
                self.cal_status_label.setStyleSheet("color: #226b45; font-weight: bold;")
            else:
                self.cal_status_label.setText(
                    "NOT CALIBRATED - angles are measured from the enclosure, "
                    "not from the machine")
                self.cal_status_label.setStyleSheet(
                    "color: #b00020; font-weight: bold;")

            # Why a button might refuse, before it is pressed.
            still = imu_group.get("restSeconds")
            at_rest = imu_group.get("atRest")
            if still is None:
                self.rest_label.setText("At rest: unknown")
                self.burn_zero_btn.setEnabled(True)
                self.calibrate_now_btn.setEnabled(True)
            else:
                self.rest_label.setText(
                    "Standing still for %.1f s%s" % (still, "" if at_rest
                    else "  -  hold still to calibrate"))
                self.burn_zero_btn.setEnabled(bool(at_rest))
                self.calibrate_now_btn.setEnabled(bool(at_rest))

            self.fw_upload_btn.setEnabled(self.technician_mode and self.firmware_path is not None)

            # Update IP configuration fields only if not editing
            if not self.is_editing_ip:
                self.controller_ip_edit.setText(status.get("controllerIp", self.base_ip))
                whitelist = status.get("whitelistIps", [])
                self.whitelist_ip1_edit.setText(whitelist[0] if len(whitelist) > 0 else "")
                self.whitelist_ip2_edit.setText(whitelist[1] if len(whitelist) > 1 else "")

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

    def set_imu_axis_map(self):
        axes = {n: self.axis_combos[n].currentText() for n in ("pitch", "roll", "yaw")}
        if len(set(axes.values())) != 3:
            QMessageBox.warning(self, "IMU Axis Map",
                "Pitch, roll and yaw must each use a different axis.")
            return
        ok, msg = self.api_client.set_imu_axis_map(
            axes["pitch"], axes["roll"], axes["yaw"],
            self.axis_inverts["pitch"].isChecked(),
            self.axis_inverts["roll"].isChecked(),
            self.axis_inverts["yaw"].isChecked())
        if ok:
            QMessageBox.information(self, "IMU Axis Map", msg)
        else:
            QMessageBox.critical(self, "IMU Axis Map Failed", msg)

    def burn_zero_calibration(self):
        if QMessageBox.question(
                self, "Burn zero calibration",
                "Declare the machine's CURRENT attitude to be level?\n\n"
                "This writes to flash and redefines what every future reading is "
                "measured against. Only do this with the machine standing on flat "
                "ground.",
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No) != QMessageBox.Yes:
            return
        ok, data = self.api_client.burn_zero_calibration()
        if ok:
            QMessageBox.information(self, "Zero calibration burned",
                "Level recorded.\n\nMounting angle measured: pitch %.3f deg, "
                "roll %.3f deg." % (data.get("mountPitch", 0.0), data.get("mountRoll", 0.0)))
        else:
            QMessageBox.warning(self, "Calibration refused",
                "%s\n\nThe machine must be standing still for %s seconds; it has "
                "been still for %.1f." % (data.get("message", "refused"),
                                          data.get("restRequiredS", "?"),
                                          data.get("restSeconds", 0.0)))

    def calibrate_now(self):
        ok, data = self.api_client.calibrate_now()
        if ok:
            QMessageBox.information(self, "Calibrated",
                "Drift cleared.\n\npitch %.3f  roll %.3f  yaw %.3f"
                % (data.get("pitch", 0.0), data.get("roll", 0.0), data.get("yaw", 0.0)))
        else:
            QMessageBox.warning(self, "Calibration refused",
                "%s\n\nStill for %.1f s." % (data.get("message", "refused"),
                                              data.get("restSeconds", 0.0)))

    def sync_imu_mount_orientation(self):
        """One-shot sync of the axis-map controls from the device's config.

        V1.5.1 reports imuPitchAxis / imuRollAxis / imuYawAxis plus an invert
        flag for each, in place of the single orientation enum earlier firmware
        used.
        """
        config = self.api_client.get_config()
        if not config or config.get("error"):
            return
        for name in ("pitch", "roll", "yaw"):
            axis = config.get("imu%sAxis" % name.capitalize())
            if axis:
                idx = self.axis_combos[name].findText(str(axis).upper())
                if idx >= 0:
                    self.axis_combos[name].setCurrentIndex(idx)
            self.axis_inverts[name].setChecked(
                bool(config.get("imu%sInvert" % name.capitalize(), False)))

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

