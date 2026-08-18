"""
Unit tests for main_widget.py's IMU axis mapping control (angle/axis tables,
the axis diagram, and the sync/set handlers). Requires QT_QPA_PLATFORM=offscreen
in the environment (set in conftest.py) since it constructs a real QWidget.
"""
import sys
import os
from unittest.mock import patch

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from PyQt5.QtWidgets import QApplication
from main_widget import MainWidget, IMU_AXES, IMU_ANGLES, MSB_AXIS_IMAGE

# A single QApplication instance is required for any QWidget construction
_app = QApplication.instance() or QApplication([])


class DummyApiClient:
    base_url = "http://dummy/"

    def __init__(self):
        self.get_config_return = None
        self.set_imu_axis_map_return = (True, "ok")
        self.set_imu_axis_map_calls = []
        self.set_imu_axis_map_inverts = []

    def get_config(self):
        return self.get_config_return

    def get_status(self):
        return None

    def set_imu_axis_map(self, pitch_axis, roll_axis, yaw_axis,
                         pitch_invert=False, roll_invert=False, yaw_invert=False):
        self.set_imu_axis_map_calls.append((pitch_axis, roll_axis, yaw_axis))
        self.set_imu_axis_map_inverts.append((pitch_invert, roll_invert, yaw_invert))
        return self.set_imu_axis_map_return


def make_widget():
    return MainWidget(DummyApiClient(), "192.168.1.198")


def set_axes(widget, pitch, roll, yaw):
    widget.imu_axis_combos["pitch"].setCurrentText(pitch)
    widget.imu_axis_combos["roll"].setCurrentText(roll)
    widget.imu_axis_combos["yaw"].setCurrentText(yaw)


def current_axes(widget):
    return tuple(widget.imu_axis_combos[a].currentText() for a in ("pitch", "roll", "yaw"))


def test_axis_choices_are_the_three_sensor_axes():
    assert IMU_AXES == ["X", "Y", "Z"]


# The GUI defaults must match the firmware defaults in
# lib/GG/src/imu_mount_orientation.hpp (pitch=X, roll=Y, yaw=Z), otherwise the
# combos would misreport the board's state before the first sync.
def test_default_axes_match_firmware_defaults():
    defaults = {angle: default for angle, _label, _field, default, _invert in IMU_ANGLES}
    assert defaults == {"pitch": "X", "roll": "Y", "yaw": "Z"}


def test_combo_boxes_populated_with_all_axes():
    widget = make_widget()
    for angle, _label, _field, _default, _invert in IMU_ANGLES:
        combo = widget.imu_axis_combos[angle]
        items = [combo.itemText(i) for i in range(combo.count())]
        assert items == IMU_AXES


def test_combos_start_at_the_defaults():
    widget = make_widget()
    assert current_axes(widget) == ("X", "Y", "Z")


def test_axis_diagram_is_loaded():
    assert os.path.exists(MSB_AXIS_IMAGE), f"missing asset: {MSB_AXIS_IMAGE}"
    widget = make_widget()
    pixmap = widget.axis_image_label.pixmap()
    assert pixmap is not None and not pixmap.isNull()


def test_axis_diagram_sits_in_the_config_group():
    widget = make_widget()
    config_layout = widget.status_groups["config"].layout()
    widgets = [config_layout.itemAt(i).widget() for i in range(config_layout.count())]
    assert widget.axis_image_label in widgets


def test_sync_sets_combos_from_get_config_response():
    widget = make_widget()
    widget.api_client.get_config_return = {
        "type": "config", "imuPitchAxis": "Z", "imuRollAxis": "X", "imuYawAxis": "Y",
    }

    widget.sync_imu_axis_map()

    assert current_axes(widget) == ("Z", "X", "Y")


def test_sync_defaults_when_fields_missing():
    widget = make_widget()
    set_axes(widget, "Z", "X", "Y")
    widget.api_client.get_config_return = {"type": "config"}  # no imu*Axis fields

    widget.sync_imu_axis_map()

    assert current_axes(widget) == ("X", "Y", "Z")


def test_sync_does_nothing_when_offline():
    widget = make_widget()
    set_axes(widget, "Z", "X", "Y")
    widget.api_client.get_config_return = None  # simulates connection failure

    widget.sync_imu_axis_map()

    # Should be left unchanged, not reset/crash
    assert current_axes(widget) == ("Z", "X", "Y")


def test_sync_ignores_error_response():
    widget = make_widget()
    set_axes(widget, "Z", "X", "Y")
    widget.api_client.get_config_return = {"error": "AUTH_ERROR"}

    widget.sync_imu_axis_map()

    assert current_axes(widget) == ("Z", "X", "Y")


@patch("main_widget.QMessageBox.information")
def test_set_calls_api_client_with_selected_axes(mock_info_box):
    widget = make_widget()
    set_axes(widget, "Y", "Z", "X")

    widget.set_imu_axis_map()

    assert widget.api_client.set_imu_axis_map_calls == [("Y", "Z", "X")]
    mock_info_box.assert_called_once()  # confirms the success dialog path ran


@patch("main_widget.QMessageBox.information")
def test_every_permutation_is_sent_verbatim(mock_info_box):
    widget = make_widget()
    permutations = [("X", "Y", "Z"), ("X", "Z", "Y"), ("Y", "X", "Z"),
                    ("Y", "Z", "X"), ("Z", "X", "Y"), ("Z", "Y", "X")]
    for pitch, roll, yaw in permutations:
        widget.api_client.set_imu_axis_map_calls.clear()
        set_axes(widget, pitch, roll, yaw)
        widget.set_imu_axis_map()
        assert widget.api_client.set_imu_axis_map_calls == [(pitch, roll, yaw)]


# Two angles sharing an axis is physically meaningless - the GUI must block it
# locally rather than round-tripping to the board for the rejection.
@patch("main_widget.QMessageBox.warning")
def test_duplicate_axes_are_blocked_without_calling_the_api(mock_warning_box):
    widget = make_widget()
    set_axes(widget, "X", "X", "Z")

    widget.set_imu_axis_map()

    assert widget.api_client.set_imu_axis_map_calls == []
    mock_warning_box.assert_called_once()


@patch("main_widget.QMessageBox.warning")
def test_all_three_axes_identical_is_blocked(mock_warning_box):
    widget = make_widget()
    set_axes(widget, "Y", "Y", "Y")

    widget.set_imu_axis_map()

    assert widget.api_client.set_imu_axis_map_calls == []
    mock_warning_box.assert_called_once()


@patch("main_widget.QMessageBox.critical")
def test_set_shows_error_dialog_on_failure(mock_critical_box):
    widget = make_widget()
    widget.api_client.set_imu_axis_map_return = (False, "Invalid IMU axis (expected X, Y or Z)")
    set_axes(widget, "X", "Y", "Z")

    widget.set_imu_axis_map()

    mock_critical_box.assert_called_once()
