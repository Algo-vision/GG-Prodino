"""
Unit tests for main_widget.py's IMU mount orientation control (label
mapping + sync/set handlers) added today. Requires QT_QPA_PLATFORM=offscreen
in the environment (set in conftest.py) since it constructs a real QWidget.
"""
import sys
import os
from unittest.mock import patch

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from PyQt5.QtWidgets import QApplication
from main_widget import MainWidget, IMU_ORIENTATION_LABELS, IMU_ORIENTATION_LABELS_REVERSE

# A single QApplication instance is required for any QWidget construction
_app = QApplication.instance() or QApplication([])


class DummyApiClient:
    base_url = "http://dummy/"

    def __init__(self):
        self.get_config_return = None
        self.set_imu_mount_orientation_return = (True, "ok")
        self.set_imu_mount_orientation_calls = []

    def get_config(self):
        return self.get_config_return

    def get_status(self):
        return None

    def set_imu_mount_orientation(self, orientation):
        self.set_imu_mount_orientation_calls.append(orientation)
        return self.set_imu_mount_orientation_return


def make_widget():
    return MainWidget(DummyApiClient(), "192.168.1.198")


def test_label_mapping_covers_all_five_orientations():
    assert set(IMU_ORIENTATION_LABELS.values()) == {
        "STANDING", "TILT_FORWARD", "TILT_BACKWARD", "TILT_LEFT", "TILT_RIGHT"
    }


def test_label_mapping_round_trips():
    for label, wire_value in IMU_ORIENTATION_LABELS.items():
        assert IMU_ORIENTATION_LABELS_REVERSE[wire_value] == label


def test_combo_box_populated_with_all_labels():
    widget = make_widget()
    items = [widget.imu_orientation_combo.itemText(i) for i in range(widget.imu_orientation_combo.count())]
    assert items == list(IMU_ORIENTATION_LABELS.keys())


def test_sync_sets_combo_from_get_config_response():
    widget = make_widget()
    widget.api_client.get_config_return = {"type": "config", "imuMountOrientation": "TILT_RIGHT"}

    widget.sync_imu_mount_orientation()

    assert widget.imu_orientation_combo.currentText() == "Tilted Right"


def test_sync_defaults_to_standing_when_field_missing():
    widget = make_widget()
    widget.api_client.get_config_return = {"type": "config"}  # no imuMountOrientation field

    widget.sync_imu_mount_orientation()

    assert widget.imu_orientation_combo.currentText() == "Standing (Default)"


def test_sync_does_nothing_when_offline():
    widget = make_widget()
    widget.imu_orientation_combo.setCurrentText("Tilted Left")
    widget.api_client.get_config_return = None  # simulates connection failure

    widget.sync_imu_mount_orientation()

    # Should be left unchanged, not reset/crash
    assert widget.imu_orientation_combo.currentText() == "Tilted Left"


def test_sync_ignores_error_response():
    widget = make_widget()
    widget.imu_orientation_combo.setCurrentText("Tilted Right")
    widget.api_client.get_config_return = {"error": "AUTH_ERROR"}

    widget.sync_imu_mount_orientation()

    assert widget.imu_orientation_combo.currentText() == "Tilted Right"


@patch("main_widget.QMessageBox.information")
def test_set_calls_api_client_with_correct_wire_value(mock_info_box):
    widget = make_widget()
    widget.imu_orientation_combo.setCurrentText("Tilted Backward")

    widget.set_imu_mount_orientation()

    assert widget.api_client.set_imu_mount_orientation_calls == ["TILT_BACKWARD"]
    mock_info_box.assert_called_once()  # confirms the success dialog path ran


@patch("main_widget.QMessageBox.information")
def test_set_for_each_combo_option_sends_matching_wire_value(mock_info_box):
    widget = make_widget()
    for label, wire_value in IMU_ORIENTATION_LABELS.items():
        widget.api_client.set_imu_mount_orientation_calls.clear()
        widget.imu_orientation_combo.setCurrentText(label)
        widget.set_imu_mount_orientation()
        assert widget.api_client.set_imu_mount_orientation_calls == [wire_value]


@patch("main_widget.QMessageBox.critical")
def test_set_shows_error_dialog_on_failure(mock_critical_box):
    widget = make_widget()
    widget.api_client.set_imu_mount_orientation_return = (False, "Invalid IMU mount orientation")
    widget.imu_orientation_combo.setCurrentText("Standing (Default)")

    widget.set_imu_mount_orientation()

    mock_critical_box.assert_called_once()
