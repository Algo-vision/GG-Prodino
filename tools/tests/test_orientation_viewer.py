"""
Unit tests for orientation_viewer.py's rotation transform math and IMU
source-string logic. Requires QT_QPA_PLATFORM=offscreen (set in conftest.py).
"""
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from PyQt5.QtWidgets import QApplication
import orientation_viewer as ov

_app = QApplication.instance() or QApplication([])


class DummyApiClient:
    base_url = "http://dummy/"

    def __init__(self):
        self.get_imu_return = None

    def get_imu(self):
        return self.get_imu_return


def make_widget():
    return ov.OrientationViewerWidget(DummyApiClient(), "192.168.1.198")


def assert_is_proper_rotation(matrix4x4):
    """A valid rotation's 3x3 part has determinant +1 and unit-length,
    mutually orthogonal rows (no scaling/reflection distortion)."""
    assert abs(matrix4x4.determinant() - 1.0) < 1e-3, \
        f"determinant {matrix4x4.determinant()} is not 1 (reflection or scaling present)"

    rows = [matrix4x4.row(i) for i in range(3)]
    rows_xyz = [(r.x(), r.y(), r.z()) for r in rows]
    for row in rows_xyz:
        mag_sq = sum(c * c for c in row)
        assert abs(mag_sq - 1.0) < 1e-3, f"row {row} is not unit length (mag^2={mag_sq})"
    for i in range(3):
        for j in range(i + 1, 3):
            dot = sum(rows_xyz[i][k] * rows_xyz[j][k] for k in range(3))
            assert abs(dot) < 1e-3, f"rows {i} and {j} are not orthogonal (dot={dot})"


def test_zero_rotation_is_identity_like():
    widget = make_widget()
    widget._center_and_orient(0.0, 0.0, 0.0)
    assert_is_proper_rotation(widget.box.transform())


def test_rotation_is_proper_for_various_angles():
    widget = make_widget()
    for pitch, roll, yaw in [(30, -15, 90), (0, 0, 180), (45, 45, 45), (-90, 0, 0), (0, 89, 0)]:
        widget._center_and_orient(pitch, roll, yaw)
        assert_is_proper_rotation(widget.box.transform())


def test_poll_imu_updates_labels():
    widget = make_widget()
    widget.api_client.get_imu_return = {
        "type": "imu", "pitch": 12.3, "roll": -4.5, "yaw": 67.8,
        "imuValid": True, "imu2Valid": True,
    }

    widget.poll_imu()

    assert "12.3" in widget.pitch_label.text()
    assert "-4.5" in widget.roll_label.text()
    assert "67.8" in widget.yaw_label.text()


def test_poll_imu_handles_no_data():
    widget = make_widget()
    widget.api_client.get_imu_return = None

    widget.poll_imu()  # should not raise

    assert "No data" in widget.status_label.text() or "connection lost" in widget.status_label.text()


def test_source_label_both_valid():
    widget = make_widget()
    widget.api_client.get_imu_return = {"pitch": 0, "roll": 0, "yaw": 0, "imuValid": True, "imu2Valid": True}
    widget.poll_imu()
    assert widget.status_label.text() == "Source: Fused (IMU1 + IMU2)"


def test_source_label_imu1_only():
    widget = make_widget()
    widget.api_client.get_imu_return = {"pitch": 0, "roll": 0, "yaw": 0, "imuValid": True, "imu2Valid": False}
    widget.poll_imu()
    assert widget.status_label.text() == "Source: IMU1 only"


def test_source_label_imu2_only():
    widget = make_widget()
    widget.api_client.get_imu_return = {"pitch": 0, "roll": 0, "yaw": 0, "imuValid": False, "imu2Valid": True}
    widget.poll_imu()
    assert widget.status_label.text() == "Source: IMU2 only"


def test_source_label_neither_valid():
    widget = make_widget()
    widget.api_client.get_imu_return = {"pitch": 0, "roll": 0, "yaw": 0, "imuValid": False, "imu2Valid": False}
    widget.poll_imu()
    assert widget.status_label.text() == "Source: No valid IMU data"
