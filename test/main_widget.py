from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QLabel, QPushButton, QHBoxLayout, QTableWidget, QTableWidgetItem, QComboBox, QFileDialog, QMessageBox, QGroupBox, QGridLayout, QLineEdit, QCheckBox, QScrollArea)
from PyQt5.QtCore import QTimer, pyqtSignal
from firmware_uploader import upload_firmware
from firmware_uploader import upload_firmware
import time
import random
import socket
import threading
import json

class MainWidget(QWidget):
    reconnect_requested = pyqtSignal()
    upload_finished_signal = pyqtSignal(bool, str)
    status_update_signal = pyqtSignal(dict) # New signal for UDP updates
    
    def __init__(self, api_client, base_ip, parent=None, client_offset_ms=0, polling_interval_ms=10000): # Slow poll for fallback
        super().__init__(parent)
        self.api_client = api_client
        self.base_ip = base_ip
        self.client_offset_ms = client_offset_ms
        self.polling_interval_ms = polling_interval_ms
        print(f"MainWidget.__init__: api_client.base_url is {self.api_client.base_url}")
        print(f"MainWidget.__init__: client_offset={client_offset_ms}ms, polling_interval={polling_interval_ms}ms")
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_status)
        self.is_editing_ip = False
        self.init_ui()
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
                    # Update UI on main thread (using QTimer.singleShot as a hack or signal)
                    # Since we are in a thread, we should use signals. 
                    # But for simplicity in this existing structure, let's just update the internal state
                    # and trigger a UI update safely.
                    # Actually, PyQt widgets must be updated from main thread.
                    # Let's use a signal.
                    # Wait, I can't easily add a signal to the class definition dynamically.
                    # I'll add a signal to the class in a separate edit or use QMetaObject.invokeMethod
                    # For now, let's just use the existing timer to process the LAST received UDP packet?
                    # No, that defeats the purpose.
                    
                    # I will add a signal to the class definition in the next chunk.
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
            
        # Enable firmware uploader if technician mode
        self.technician_mode = status.get("technicianMode", False)
        self.fw_box.setEnabled(self.technician_mode)
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
        # Add "controllerIp" and "whitelistIps" to the fields list
        fields = ["firmwareVersion", "controllerIp", "whitelistIps", "relays_status", "imuX", "imuY", "imuZ", "imuGx", "imuGy", "imuGz", "pitch", "roll", "yaw", "gpsLat", "gpsLng", "gpsAlt", "gpsTime", "gpsSpeedNorth", "gpsSpeedEast", "gpsSpeedDown", "gpsGroundSpeed", "gpsHeading", "ledInternal", "ledIo", "gpsValid", "button_tech", "imuValid", "GPSConnected", "optoin_status"]
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

        # Serial Number Configuration
        sn_box = QGroupBox("Serial Number Configuration")
        sn_layout = QHBoxLayout()
        
        self.sn_label = QLabel("Current SN: Unknown")
        self.sn_input = QLineEdit()
        self.sn_input.setPlaceholderText("2000-2999")
        self.set_sn_btn = QPushButton("Set SN")
        self.set_sn_btn.clicked.connect(self.set_serial_number)
        
        sn_layout.addWidget(self.sn_label)
        sn_layout.addWidget(self.sn_input)
        sn_layout.addWidget(self.set_sn_btn)
        
        sn_box.setLayout(sn_layout)
        layout.addWidget(sn_box)

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

        # Firmware uploader
        self.fw_box = QGroupBox("Firmware Uploader (Technician Mode)")
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
        self.fw_box.setLayout(fw_layout)
        layout.addWidget(self.fw_box)
        self.fw_box.setEnabled(False)

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
        status = self.api_client.get_status()
        if status and status.get("error") == "AUTH_ERROR":
            self.timer.stop()
            QMessageBox.warning(self, "Authentication Error", "Invalid session token. Please log in again.")
            self.reconnect_requested.emit()
        elif status:
            for k, v in self.status_labels.items():
                val = status.get(k, "-")
                if isinstance(val, list):
                    val = ", ".join(str(x) for x in val)
                v.setText(str(val))
            # Enable firmware uploader if technician mode
            self.technician_mode = status.get("technicianMode", False)
            self.fw_box.setEnabled(self.technician_mode)
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
            
            # Add jitter to prevent collision with other clients
            # Random jitter of ±500ms around the base interval
            # This prevents uncoordinated clients from polling at the same time
            jitter = random.randint(-500, 500)
            next_interval = self.polling_interval_ms + jitter
            # Ensure interval stays within reasonable bounds (2-4 seconds)
            next_interval = max(2000, min(4000, next_interval))
            self.timer.setInterval(next_interval)

        else:
            # Communication lost, return to login screen
            self.timer.stop()
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

    def set_serial_number(self):
        sn = self.sn_input.text()
        if not sn.isdigit() or not (2000 <= int(sn) <= 2999):
            QMessageBox.warning(self, "Invalid Serial Number", "Serial number must be a number between 2000 and 2999.")
            return

        if not self.technician_mode:
            QMessageBox.warning(self, "Technician Mode Required", "You must be in Technician Mode to set the serial number.\nHold the button on the device during startup.")
            return

        resp = self.api_client.set_serial_number(sn)
        if resp and resp.get("success"):
            QMessageBox.information(self, "Success", f"Serial number set to {resp.get('message')}. Device will reboot.")
            self.refresh_serial_number()
        elif resp and resp.get("error"):
            QMessageBox.critical(self, "Error", f"Failed to set serial number: {resp.get('message')}")
        else:
            QMessageBox.critical(self, "Error", "Failed to communicate with device.")

    def refresh_serial_number(self):
        resp = self.api_client.get_serial_number()
        if resp and resp.get("type") == "serial_number":
            sn = resp.get("serial_number", "Unknown")
            self.sn_label.setText(f"Current SN: {sn}")

