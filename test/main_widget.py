from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QLabel, QPushButton, QHBoxLayout, QTableWidget, QTableWidgetItem, QComboBox, QFileDialog, QMessageBox, QGroupBox, QGridLayout)
from PyQt5.QtCore import QTimer
from firmware_uploader import upload_firmware

class MainWidget(QWidget):
    def __init__(self, api_client, base_ip, parent=None):
        super().__init__(parent)
        self.api_client = api_client
        self.base_ip = base_ip
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_status)
        self.init_ui()
        self.timer.start(1000)
        self.technician_mode = False

    def init_ui(self):
        layout = QVBoxLayout()
        # Status group
        self.status_group = QGroupBox("Device Status")
        status_layout = QGridLayout()
        self.status_labels = {}
        fields = ["relays_status", "imuX", "imuY", "imuZ", "gpsLat", "gpsLng", "gpsAlt", "ledInternal", "ledIo", "gpsValid", "button1", "imuValid", "GPSConnected", "optoin_status"]
        for i, field in enumerate(fields):
            label = QLabel("-")
            status_layout.addWidget(QLabel(field), i, 0)
            status_layout.addWidget(label, i, 1)
            self.status_labels[field] = label
        self.status_group.setLayout(status_layout)
        layout.addWidget(self.status_group)

        # Relay controls
        relay_box = QGroupBox("Relays")
        relay_layout = QHBoxLayout()
        self.relay_buttons = []
        for i in range(4):
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
        self.led_combo.addItems(["OFF", "GREEN", "RED", "ORANGE"])
        self.led_btn = QPushButton("Set IO LED")
        self.led_btn.clicked.connect(self.set_led)
        led_layout.addWidget(self.led_combo)
        led_layout.addWidget(self.led_btn)
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

        self.setLayout(layout)
        self.firmware_path = None

    def update_status(self):
        status = self.api_client.get_status()
        if status:
            for k, v in self.status_labels.items():
                val = status.get(k, "-")
                if isinstance(val, list):
                    val = ", ".join(str(x) for x in val)
                v.setText(str(val))
            # Enable firmware uploader if technician mode
            self.technician_mode = status.get("ledIo", "") == "ORANGE"
            self.fw_box.setEnabled(self.technician_mode)
            self.fw_upload_btn.setEnabled(self.technician_mode and self.firmware_path)
        else:
            for v in self.status_labels.values():
                v.setText("-")

    def toggle_relay(self, relay_id):
        # Get current relay state from status label
        relays = self.status_labels["relays_status"].text().split(", ")
        if len(relays) > relay_id:
            current = relays[relay_id]
            new_state = not bool(int(current))
            self.api_client.set_relay(relay_id, new_state)
            self.update_status()

    def set_led(self):
        color = self.led_combo.currentText()
        self.api_client.set_led(color)
        self.update_status()

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
        if not self.firmware_path:
            QMessageBox.warning(self, "No file", "Please select a firmware file.")
            return
        ok, msg = upload_firmware(self.base_ip, self.firmware_path)
        if ok:
            QMessageBox.information(self, "Upload", "Firmware upload successful!")
        else:
            QMessageBox.critical(self, "Upload Failed", f"Upload failed: {msg}")
