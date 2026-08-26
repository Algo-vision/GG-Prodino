from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QLabel, QPushButton, QHBoxLayout, QTableWidget, QTableWidgetItem, QComboBox, QFileDialog, QMessageBox, QGroupBox, QGridLayout, QLineEdit, QCheckBox, QScrollArea)
from PyQt5.QtCore import QTimer, pyqtSignal, Qt
from PyQt5.QtGui import QPixmap
from firmware_uploader import upload_firmware
import os
import time

import socket
import threading
import json
from collections import deque

# Which sensor axis each angle rotates about. Firmware default is
# pitch=X, roll=Y, yaw=Z - see lib/GG/src/imu_mount_orientation.hpp.
# The three angles must each use a different axis; the firmware rejects
# anything else.
IMU_AXES = ["X", "Y", "Z"]
IMU_ANGLES = [
    # (angle key, GUI label, axis field from get_config, default axis, invert field)
    ("pitch", "Pitch", "imuPitchAxis", "X", "imuPitchInvert"),
    ("roll",  "Roll",  "imuRollAxis",  "Y", "imuRollInvert"),
    ("yaw",   "Yaw",   "imuYawAxis",   "Z", "imuYawInvert"),
]

# Axis diagram shown under the axis fields in the Config box, alongside this
# file so the GUI's assets travel with it.
MSB_AXIS_IMAGE = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "MSB_Axis.png")

# The four status sections, mirroring the get_overview / get_imu / get_gps /
# get_config firmware commands (src/http_server.cpp). get_status returns all
# four nested under these keys - see statusGenerateJson() in
# src/status_manager.cpp.
STATUS_SECTIONS = [
    ("config", "Config", [
        "firmwareVersion",
        "controllerIp",
        "whitelistIps",
        "technicianMode",
        "burnedHours",
        "sessionHours",
        "techLedColor",
        "imuPitchAxis",
        "imuRollAxis",
        "imuYawAxis",
        "imuPitchInvert",
        "imuRollInvert",
        "imuYawInvert",
    ]),
    ("overview", "Overview", [
        "powerConnected",
        "powerSane",
        "systemVoltage",
        "systemCurrent_A",
        "relays_status",
        "optoin_status",
        "safetyMode",
        "safetyModeDurationMs",
        "ocuConnected",
        "ocuDisconnectedDurationMs",
    ]),
    ("imu", "IMU", [
        "angleSane",
        "pitch",
        "roll",
        "yaw",
        "imuValid",
        "imu1Sane",
        "imuX",
        "imuY",
        "imuZ",
        "imuGx",
        "imuGy",
        "imuGz",
        "imu2Valid",
        "imu2Sane",
        "imu2X",
        "imu2Y",
        "imu2Z",
        "imu2Gx",
        "imu2Gy",
        "imu2Gz",
        "imuTemp",
        "zeroCalValid",
        "mountPitch",
        "mountRoll",
        "restSeconds",
        "atRest",
    ]),
    ("gps", "GPS", [
        "gpsConnected",
        "gpsSane",
        "gpsSatellites",
        "gpsLat",
        "gpsLng",
        "gpsAlt",
        "gpsHeading",
        "gpsGroundSpeed",
        "gpsSpeedNorth",
        "gpsSpeedEast",
        "gpsSpeedDown",
        "gpsTime",
        "lastGpsLat",
        "lastGpsLng",
        "lastGpsAlt",
    ]),
]

class MainWidget(QWidget):
    reconnect_requested = pyqtSignal()
    upload_finished_signal = pyqtSignal(bool, str)
    status_update_signal = pyqtSignal(dict) # New signal for UDP updates
    
    def __init__(self, api_client, base_ip, parent=None, client_offset_ms=0, polling_interval_ms=0):
        # polling_interval_ms is the MINIMUM gap between the end of one poll
        # and the start of the next. 0 (the default) polls as fast as the link
        # allows - the next request goes out as soon as the previous answer is
        # in and pending UI events have run. The old fixed 50 ms cadence dated
        # from when the board could only take ~20 req/s; it now serves 30-65
        # depending on which sections are requested.
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
        # Single-shot, re-armed at the END of each update_status. A repeating
        # timer ran the interval and the HTTP round trip in SERIES (the jitter
        # code reset the countdown after every poll), so 50 ms + ~29 ms wire
        # time made a 12.5 Hz GUI out of a 34 Hz link. Chaining single-shots
        # makes the gap start when the poll ends - with a 0 gap the loop runs
        # at whatever the link gives, and the event loop still services user
        # clicks between polls.
        self.timer = QTimer(self)
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(self.update_status)
        self.status_ignore_deadline = 0  # Timestamp until which to ignore status updates (for reboot)
        self.init_ui()
        # One-shot sync of slow-changing config (currently just the IMU axis
        # map) - not part of the fast get_status polling loop.
        QTimer.singleShot(0, self.sync_imu_axis_map)
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
        """Kick off the poll chain - each completed poll schedules the next."""
        print(f"MainWidget: Status polling started (min gap={self.polling_interval_ms}ms)")
        self.timer.start(self.polling_interval_ms)

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

    def record_poll(self, duration_s):
        """Fold one completed get_status round trip into the HTTP rate label.

        Rate over the polls of the last 3 seconds; duration as their mean.
        Client-side on purpose: this is the rate the technician's machine is
        actually achieving, network included, not what the board could do.
        """
        now = time.time()
        self._poll_history.append((now, duration_s))
        recent = [(t, d) for t, d in self._poll_history if now - t <= 3.0]
        if len(recent) < 2:
            self.http_rate_label.setText("HTTP: measuring...")
            return
        span = recent[-1][0] - recent[0][0]
        mean_ms = sum(d for _t, d in recent) / len(recent) * 1000.0
        if span > 1e-6:
            self.http_rate_label.setText(
                "HTTP: %.1f Hz, %.0f ms/poll" % ((len(recent) - 1) / span, mean_ms))
        else:
            self.http_rate_label.setText("HTTP: %.0f ms/poll" % mean_ms)

    def config_field(self, status, key, default=None):
        """Read a field that now lives only under the "config" category.

        firmwareVersion / controllerIp / whitelistIps / technicianMode used to
        be duplicated at the top level of get_status; they are not any more.
        The top-level lookup is kept as a fallback for older firmware.
        """
        config = status.get("config")
        if isinstance(config, dict) and key in config:
            return config[key]
        return status.get(key, default)

    def apply_status_labels(self, status):
        """Fill the Overview/IMU/GPS boxes from a get_status response.

        Fields live under the "overview"/"imu"/"gps" objects, but we fall back
        to the top level so a flat payload (older firmware, UDP packet) still
        renders instead of showing all dashes.
        """
        for section, labels in self.status_labels.items():
            section_data = status.get(section)
            if not isinstance(section_data, dict):
                section_data = {}
            for field, label in labels.items():
                val = section_data.get(field, status.get(field, "-"))
                if isinstance(val, list):
                    val = ", ".join(str(x) for x in val)
                label.setText(str(val))

    def clear_status_labels(self):
        for labels in self.status_labels.values():
            for label in labels.values():
                label.setText("-")

    def handle_udp_status(self, status):
        """Update UI from UDP status packet"""
        if not status: return

        self.apply_status_labels(status)
        self.apply_calibration_state(status)

        # Enable technician mode section if in technician mode
        self.technician_mode = self.config_field(status, "technicianMode", False)
        self.tech_mode_box.setEnabled(self.technician_mode)
        self.fw_upload_btn.setEnabled(self.technician_mode and self.firmware_path is not None)

        # Update IP configuration fields only if not editing
        if not self.is_editing_ip:
            self.controller_ip_edit.setText(self.config_field(status, "controllerIp", self.base_ip))
            whitelist = self.config_field(status, "whitelistIps", []) or []
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

    def add_axis_diagram(self, group_layout, row):
        """Add the MSB axis diagram spanning both columns. Returns the next row.

        Missing or unreadable image is not fatal - the GUI still has to work
        from a checkout without the asset, so we fall back to a note.
        """
        pixmap = QPixmap(MSB_AXIS_IMAGE)
        image_label = QLabel()
        if pixmap.isNull():
            image_label.setText(f"(axis diagram not found: {MSB_AXIS_IMAGE})")
            image_label.setWordWrap(True)
            image_label.setStyleSheet("color: gray; font-style: italic;")
        else:
            # Cap the width so a large asset can't stretch the Config column.
            if pixmap.width() > 320:
                pixmap = pixmap.scaledToWidth(320, Qt.SmoothTransformation)
            image_label.setPixmap(pixmap)
        image_label.setAlignment(Qt.AlignCenter)

        group_layout.addWidget(QLabel("MSB axes:"), row, 0, 1, 2)
        group_layout.addWidget(image_label, row + 1, 0, 1, 2)
        self.axis_image_label = image_label
        return row + 2

    def init_ui(self):
        # Create a container widget for all content
        container = QWidget()
        layout = QVBoxLayout()

        # Live HTTP poll rate, measured client-side over the last few seconds.
        # The Hz is capped by the poll timer (polling_interval_ms), so the
        # ms/request number is where a section toggle shows its effect even
        # when the timer is the bottleneck.
        self.http_rate_label = QLabel("HTTP: -")
        self.http_rate_label.setToolTip(
            "Completed get_status polls per second, and one poll's round-trip "
            "time. Fewer checked sections = cheaper polls.")
        self._poll_history = deque(maxlen=60)   # (end_time, duration_s)
        rate_row = QHBoxLayout()
        rate_row.addStretch()
        rate_row.addWidget(self.http_rate_label)
        layout.addLayout(rate_row)

        # Status groups - one box per firmware command (get_overview / get_imu /
        # get_gps / get_config), laid out side by side. self.status_labels is
        # keyed by section, then by field name.
        status_row = QHBoxLayout()
        self.status_groups = {}
        self.status_labels = {}
        for section, title, fields in STATUS_SECTIONS:
            group = QGroupBox(title)
            # Each box doubles as that section's data toggle. Unchecking it
            # drops the section from the get_status poll ("groups" field), so
            # the board never builds or serialises those fields - polling gets
            # measurably faster and the box greys out. All four checked (the
            # default) sends the exact pre-groups request, so older firmware
            # is unaffected.
            group.setCheckable(True)
            group.setChecked(True)
            group.setToolTip("Uncheck to stop requesting this section from the"
                             " board. Polling gets faster; the section greys"
                             " out until re-checked.")
            group_layout = QGridLayout()
            self.status_labels[section] = {}
            for i, field in enumerate(fields):
                label = QLabel("-")
                group_layout.addWidget(QLabel(field), i, 0)
                group_layout.addWidget(label, i, 1)
                self.status_labels[section][field] = label
            next_row = len(fields)
            # The axis diagram belongs with the imu*Axis fields it explains.
            if section == "config":
                next_row = self.add_axis_diagram(group_layout, next_row)
            group_layout.setRowStretch(next_row, 1)  # keep rows top-aligned
            group.setLayout(group_layout)
            self.status_groups[section] = group
            status_row.addWidget(group)
        layout.addLayout(status_row)

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

        # IMU Axis Mapping - which sensor axis each angle rotates about, so the
        # angles still read correctly when the sensor is mounted in a different
        # orientation. See the MSB axis diagram in the Config box above.
        imu_axis_box = QGroupBox("IMU Axis Mapping (which axis each angle rotates about)")
        imu_axis_layout = QHBoxLayout()
        self.imu_axis_combos = {}
        self.imu_invert_checkboxes = {}
        for angle, label, _axis_field, default_axis, _invert_field in IMU_ANGLES:
            combo = QComboBox()
            combo.addItems(IMU_AXES)
            combo.setCurrentText(default_axis)
            # Negates the axis - for a sensor mounted flipped end-for-end,
            # where the angle would otherwise run backwards.
            invert = QCheckBox("Invert")
            invert.setToolTip(f"Negate the {label.lower()} axis (reverses its direction)")
            imu_axis_layout.addWidget(QLabel(f"{label}:"))
            imu_axis_layout.addWidget(combo)
            imu_axis_layout.addWidget(invert)
            self.imu_axis_combos[angle] = combo
            self.imu_invert_checkboxes[angle] = invert
        self.imu_axis_btn = QPushButton("Set Axis Mapping")
        self.imu_axis_btn.clicked.connect(self.set_imu_axis_map)
        imu_axis_layout.addWidget(self.imu_axis_btn)
        imu_axis_layout.addStretch()
        imu_axis_box.setLayout(imu_axis_layout)
        layout.addWidget(imu_axis_box)

        # Calibration. Two buttons that are easy to confuse, so the box says
        # what each one does rather than relying on the names.
        cal_box = QGroupBox("Calibration")
        cal_layout = QVBoxLayout()

        self.cal_status_label = QLabel("-")
        self.cal_status_label.setWordWrap(True)
        cal_layout.addWidget(self.cal_status_label)

        cal_buttons = QHBoxLayout()
        self.burn_zero_btn = QPushButton("Burn Zero Calibration")
        self.burn_zero_btn.setToolTip(
            "Declare the machine's CURRENT attitude to be level, and store it "
            "in flash.\nPress only with the machine standing on flat ground.")
        self.burn_zero_btn.clicked.connect(self.burn_zero_calibration)
        cal_buttons.addWidget(self.burn_zero_btn)

        self.calibrate_now_btn = QPushButton("Re-sync to Gravity")
        self.calibrate_now_btn.setToolTip(
            "Take pitch and roll straight from the accelerometer and zero "
            "yaw.\nDoes NOT level anything - on a slope it reports the slope.\n"
            "Stores nothing; use it to clear drift accumulated over a long run.")
        self.calibrate_now_btn.clicked.connect(self.calibrate_now)
        cal_buttons.addWidget(self.calibrate_now_btn)
        cal_buttons.addStretch()
        cal_layout.addLayout(cal_buttons)

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

        # Technician Mode Section - contains Firmware Uploader
        self.tech_mode_box = QGroupBox("Technician Mode (Hold button during startup to enable)")
        tech_mode_layout = QVBoxLayout()
        
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

    def update_status(self):
        # The timer is single-shot; every path re-arms it here except the
        # fatal ones (auth failure, communication loss), which return None
        # from _poll_once and end the chain.
        delay_ms = self._poll_once()
        if delay_ms is not None:
            self.timer.start(delay_ms)

    def _poll_once(self):
        """One status poll. Returns the delay in ms before the next poll,
        or None to stop polling."""
        # While a reboot is expected, retry gently instead of hammering a
        # dead IP with connection attempts.
        if time.time() < self.status_ignore_deadline:
            if self.waiting_for_reset:
                print("[GUI] Ignoring status (waiting for reboot)...")
            return 250

        # Ask only for the sections whose boxes are checked. With all four
        # checked (the default) the "groups" field is omitted entirely -
        # byte-identical to the old request.
        enabled = [s for s, _title, _fields in STATUS_SECTIONS
                   if self.status_groups[s].isChecked()]
        groups = None if len(enabled) == len(STATUS_SECTIONS) else enabled
        poll_start = time.time()
        status = self.api_client.get_status(groups)
        if status and status.get("error") == "AUTH_ERROR":
            # If waiting for reset, this is expected - don't show error
            if self.waiting_for_reset:
                print("[GUI] Board is resetting, waiting for reconnection...")
                return 250
            self.timer.stop()
            self._poll_history.clear()
            self.http_rate_label.setText("HTTP: -")
            QMessageBox.warning(self, "Authentication Error", "Invalid session token. Please log in again.")
            self.reconnect_requested.emit()
            return None
        elif status:
            # Successfully got status - reset the waiting flag
            if self.waiting_for_reset:
                print("[GUI] Board is back online.")
                self.waiting_for_reset = False

            self.record_poll(time.time() - poll_start)
            self.apply_status_labels(status)
            # Sections we did not ask for are absent from the response, and
            # absence must not be interpreted - an unchecked IMU box would
            # otherwise read as "firmware without calibration commands", and
            # an unchecked Config box would drop technician mode.
            if self.status_groups["imu"].isChecked():
                self.apply_calibration_state(status)
            if self.status_groups["config"].isChecked():
                # Enable technician mode section if in technician mode
                self.technician_mode = self.config_field(status, "technicianMode", False)
                self.tech_mode_box.setEnabled(self.technician_mode)
                self.fw_upload_btn.setEnabled(self.technician_mode and self.firmware_path is not None)

                # Update IP configuration fields only if not editing
                if not self.is_editing_ip:
                    self.controller_ip_edit.setText(self.config_field(status, "controllerIp", self.base_ip))
                    whitelist = self.config_field(status, "whitelistIps", []) or []
                    self.whitelist_ip1_edit.setText(whitelist[0] if len(whitelist) > 0 else "")
                    self.whitelist_ip2_edit.setText(whitelist[1] if len(whitelist) > 1 else "")

            # If manual override is not active, update LED combo from status
            if not self.led_override_checkbox.isChecked():
                current_led_status = status.get("ledIo", "OFF") if status else "OFF"
                index = self.led_combo.findText(current_led_status)
                if index != -1:
                    self.led_combo.setCurrentIndex(index)

            # The old fixed-cadence jitter ("board can handle ~20 RPS") is
            # gone: the gap starts when the poll ends, so clients cannot lock
            # into phase with each other, and the board now serves 30-65
            # req/s depending on the sections requested.
            return self.polling_interval_ms

        else:
            # Communication lost
            if self.waiting_for_reset:
                # This is expected during reset - just wait
                print("[GUI] Waiting for board to come back online...")
                return 250

            # Unexpected communication loss - show error
            self.timer.stop()
            self._poll_history.clear()
            self.http_rate_label.setText("HTTP: -")
            QMessageBox.critical(self, "Error", "Failed to communicate with device.")
            self.reconnect_requested.emit()
            self.clear_status_labels()
            return None

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
        relays = self.status_labels["overview"]["relays_status"].text().split(", ")
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
        axes = {angle: combo.currentText() for angle, combo in self.imu_axis_combos.items()}
        inverts = {angle: box.isChecked() for angle, box in self.imu_invert_checkboxes.items()}

        # The firmware rejects a non-permutation too, but catching it here gives
        # an immediate, clearer message than a round trip.
        if len(set(axes.values())) != len(axes):
            QMessageBox.warning(self, "IMU Axis Mapping",
                "Pitch, Roll and Yaw must each use a different axis.")
            return

        ok, msg = self.api_client.set_imu_axis_map(
            axes["pitch"], axes["roll"], axes["yaw"],
            inverts["pitch"], inverts["roll"], inverts["yaw"])
        if ok:
            QMessageBox.information(self, "IMU Axis Mapping",
                f"{msg}\n\nReboot the device, then re-burn the zero calibration - "
                "the stored one was measured through the old axis map.")
        else:
            if msg == "Authentication Error":
                self.timer.stop()
                QMessageBox.warning(self, "Authentication Error", "Invalid session token. Please log in again.")
                self.reconnect_requested.emit()
            else:
                QMessageBox.critical(self, "IMU Axis Mapping Failed", f"Failed to set IMU axis map: {msg}")

    def sync_imu_axis_map(self):
        """One-shot sync of the axis combos and invert boxes from the device."""
        config = self.api_client.get_config()
        if not config or config.get("error"):
            return
        for angle, _label, axis_field, default_axis, invert_field in IMU_ANGLES:
            axis = config.get(axis_field, default_axis)
            index = self.imu_axis_combos[angle].findText(axis)
            if index >= 0:
                self.imu_axis_combos[angle].setCurrentIndex(index)
            self.imu_invert_checkboxes[angle].setChecked(bool(config.get(invert_field, False)))

    # ------------------------------------------------------------------
    # Calibration
    # ------------------------------------------------------------------

    def apply_calibration_state(self, status):
        """Drive the calibration line and buttons from a status document.

        Without a burned zero the angles above are still perfectly plausible -
        they are just measured from the enclosure instead of from the machine -
        so nothing in the numbers reveals an uncalibrated board. This line is
        the only place that says so.
        """
        imu = status.get("imu")
        if not isinstance(imu, dict):
            imu = status
        if "zeroCalValid" not in imu:
            # Firmware without the calibration commands (plain V1.5.1). Say so
            # rather than leaving two buttons that can only fail.
            self.cal_status_label.setText(
                "This firmware has no calibration commands (V1.5.1.1 or later needed).")
            self.cal_status_label.setStyleSheet("color: gray; font-style: italic;")
            self.burn_zero_btn.setEnabled(False)
            self.calibrate_now_btn.setEnabled(False)
            return

        zero_valid = bool(imu.get("zeroCalValid"))
        at_rest = bool(imu.get("atRest"))
        rest_s = imu.get("restSeconds", 0.0)

        if zero_valid:
            text = ("Zero calibration burned - mounting angles pitch %.2f deg, roll %.2f deg."
                    % (imu.get("mountPitch", 0.0), imu.get("mountRoll", 0.0)))
            style = "color: green;"
        else:
            text = ("NO zero calibration burned - angles are reported in the sensor "
                    "frame, uncorrected for how the unit is mounted.")
            style = "color: red; font-weight: bold;"

        if at_rest:
            text += "  At rest."
        else:
            try:
                text += "  Machine is moving (still for %.1f s) - both buttons refused." % float(rest_s)
            except (TypeError, ValueError):
                text += "  Machine is moving - both buttons refused."

        self.cal_status_label.setText(text)
        self.cal_status_label.setStyleSheet(style)
        # Grey out rather than let the request come back 409. The line above
        # already says why.
        self.burn_zero_btn.setEnabled(at_rest)
        self.calibrate_now_btn.setEnabled(at_rest)

    def burn_zero_calibration(self):
        """Redefine level. Destructive to the previous calibration, so confirm."""
        confirm = QMessageBox.question(
            self, "Burn Zero Calibration",
            "This declares the machine's CURRENT attitude to be level, and writes "
            "it to flash.\n\nOnly press this with the machine standing on flat "
            "ground. Any tilt it has now becomes the new zero.\n\nContinue?",
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
        if confirm != QMessageBox.Yes:
            return

        ok, data = self.api_client.burn_zero_calibration()
        if data.get("error") == "AUTH_ERROR":
            self.timer.stop()
            self._poll_history.clear()
            self.http_rate_label.setText("HTTP: -")
            QMessageBox.warning(self, "Authentication Error", "Invalid session token. Please log in again.")
            self.reconnect_requested.emit()
            return
        if ok:
            QMessageBox.information(
                self, "Zero Calibration Burned",
                "This attitude is now level.\n\nMounting angles measured: "
                "pitch %.2f deg, roll %.2f deg.\n\nThese are how the bracket holds "
                "the sensor - if they look nothing like the installation, the burn "
                "is suspect."
                % (data.get("mountPitch", 0.0), data.get("mountRoll", 0.0)))
        else:
            QMessageBox.warning(
                self, "Zero Calibration Refused",
                "%s\n\nNothing was changed; any previous calibration is intact."
                % data.get("message", "Unknown error"))

    def calibrate_now(self):
        """Clear accumulated drift. Reports the correction it applied, so that
        'nothing happened' is visible rather than ambiguous - at rest the filter
        has often converged on gravity already."""
        before = self.api_client.get_imu() or {}

        ok, data = self.api_client.calibrate_now()
        if data.get("error") == "AUTH_ERROR":
            self.timer.stop()
            self._poll_history.clear()
            self.http_rate_label.setText("HTTP: -")
            QMessageBox.warning(self, "Authentication Error", "Invalid session token. Please log in again.")
            self.reconnect_requested.emit()
            return
        if not ok:
            QMessageBox.warning(
                self, "Re-sync Refused",
                "%s\n\nNothing was changed." % data.get("message", "Unknown error"))
            return

        try:
            d_pitch = float(data.get("pitch", 0.0)) - float(before.get("pitch", 0.0))
            d_roll = float(data.get("roll", 0.0)) - float(before.get("roll", 0.0))
            correction = ("\n\nCorrection applied: pitch %+.2f deg, roll %+.2f deg."
                          % (d_pitch, d_roll))
        except (TypeError, ValueError):
            correction = ""

        QMessageBox.information(
            self, "Re-synced to Gravity",
            "Pitch and roll taken from the accelerometer, yaw zeroed.\n\n"
            "Now reading: pitch %.2f deg, roll %.2f deg, yaw %.2f deg.%s\n\n"
            "Note this does not redefine level - on a slope it reports the slope."
            % (data.get("pitch", 0.0), data.get("roll", 0.0), data.get("yaw", 0.0),
               correction))

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

