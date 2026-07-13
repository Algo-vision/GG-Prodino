"""
GRK Orientation Viewer - standalone 3D orientation verification tool.

A separate helper program (per the customer's requirement) that polls the
board's get_imu endpoint and renders a live-updating 3D box representing
the HLC enclosure, so a technician can physically tilt the real board and
visually confirm the calculated pitch/roll/yaw match reality.

This is a verification instrument, not a product feature - it is not part
of the main GUI (gui_main.py) on purpose.

Usage:
    python orientation_viewer.py
"""

import sys
from PyQt5.QtWidgets import QApplication, QStackedWidget, QWidget, QVBoxLayout, QGridLayout, QLabel
from PyQt5.QtCore import QTimer, Qt
import pyqtgraph.opengl as gl

from api_client import ApiClient
from login_widget import LoginWidget

BASE_IP = "192.168.1.198"

# How often to poll get_imu. get_imu is one of the "fast" endpoints, but this
# tool is for visual verification, not high-rate logging - 10Hz is smooth
# enough to watch by eye without hammering the controller's HTTP server.
POLL_INTERVAL_MS = 100

# Box dimensions (arbitrary units, roughly enclosure-shaped: wider than tall)
BOX_SIZE = (1.5, 1.0, 0.6)  # x, y, z


class OrientationViewerWidget(QWidget):
    def __init__(self, api_client, base_ip, parent=None):
        super().__init__(parent)
        self.api_client = api_client
        self.base_ip = base_ip
        self.init_ui()

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.poll_imu)
        self.timer.start(POLL_INTERVAL_MS)

    def init_ui(self):
        layout = QVBoxLayout()

        # --- 3D view ---
        self.gl_view = gl.GLViewWidget()
        self.gl_view.setCameraPosition(distance=6, elevation=25, azimuth=45)
        layout.addWidget(self.gl_view, stretch=1)

        grid = gl.GLGridItem()
        grid.setSize(x=10, y=10)
        grid.setSpacing(x=1, y=1)
        self.gl_view.addItem(grid)

        # Fixed world-frame reference axes (does not rotate)
        world_axis = gl.GLAxisItem()
        world_axis.setSize(x=3, y=3, z=3)
        self.gl_view.addItem(world_axis)

        # Box representing the HLC enclosure
        self.box = gl.GLBoxItem()
        self.box.setSize(x=BOX_SIZE[0], y=BOX_SIZE[1], z=BOX_SIZE[2])
        self.box.setColor((100, 180, 255, 200))
        self.gl_view.addItem(self.box)

        # Body-frame axis gizmo - rotates together with the box, so you can
        # see which way each sensor axis is currently pointing
        self.body_axis = gl.GLAxisItem()
        self.body_axis.setSize(x=1.5, y=1.5, z=1.5)
        self.gl_view.addItem(self.body_axis)

        self._center_and_orient(0.0, 0.0, 0.0)

        # --- Numeric readout ---
        readout_layout = QGridLayout()
        self.pitch_label = QLabel("Pitch: --")
        self.roll_label = QLabel("Roll: --")
        self.yaw_label = QLabel("Yaw: --")
        for i, lbl in enumerate([self.pitch_label, self.roll_label, self.yaw_label]):
            lbl.setStyleSheet("font-size: 16px; font-weight: bold;")
            lbl.setAlignment(Qt.AlignCenter)
            readout_layout.addWidget(lbl, 0, i)
        layout.addLayout(readout_layout)

        self.status_label = QLabel("Waiting for data...")
        self.status_label.setAlignment(Qt.AlignCenter)
        layout.addWidget(self.status_label)

        self.setLayout(layout)

    def poll_imu(self):
        data = self.api_client.get_imu()
        if not data or data.get("error"):
            self.status_label.setText("No data / connection lost")
            return

        pitch = data.get("pitch", 0.0)
        roll = data.get("roll", 0.0)
        yaw = data.get("yaw", 0.0)

        self.pitch_label.setText(f"Pitch: {pitch:.1f}°")
        self.roll_label.setText(f"Roll: {roll:.1f}°")
        self.yaw_label.setText(f"Yaw: {yaw:.1f}°")

        imu_valid = data.get("imuValid", False)
        imu2_valid = data.get("imu2Valid", False)
        if imu_valid and imu2_valid:
            source = "Fused (IMU1 + IMU2)"
        elif imu_valid:
            source = "IMU1 only"
        elif imu2_valid:
            source = "IMU2 only"
        else:
            source = "No valid IMU data"
        self.status_label.setText(f"Source: {source}")

        self._center_and_orient(pitch, roll, yaw)

    def _center_and_orient(self, pitch, roll, yaw):
        """Reset to origin-centered, then apply yaw -> pitch -> roll.

        translate() is called before rotate() so it lands innermost in the
        composed transform (pyqtgraph prepends each new op) - the box is
        shifted to rotate around its own center rather than a corner.
        """
        cx, cy, cz = BOX_SIZE[0] / 2.0, BOX_SIZE[1] / 2.0, BOX_SIZE[2] / 2.0
        for item in (self.box, self.body_axis):
            item.resetTransform()
            item.translate(-cx, -cy, -cz)
            item.rotate(yaw, 0, 0, 1)
            item.rotate(pitch, 1, 0, 0)
            item.rotate(roll, 0, 1, 0)


def main():
    app = QApplication(sys.argv)
    api_client = ApiClient(BASE_IP)
    stack = QStackedWidget()

    def on_login_success(new_ip):
        api_client.base_ip = new_ip
        api_client.base_url = f"http://{new_ip}/"
        viewer = OrientationViewerWidget(api_client, new_ip)
        stack.addWidget(viewer)
        stack.setCurrentWidget(viewer)

    login_widget = LoginWidget(api_client, on_login_success, BASE_IP)
    stack.addWidget(login_widget)
    stack.setCurrentWidget(login_widget)

    stack.setWindowTitle("GG-GRK Orientation Viewer")
    stack.setMinimumSize(800, 700)
    stack.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
