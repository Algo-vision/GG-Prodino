"""
Unit tests for the two calibration controls added in V1.5.1.1 - the api_client
commands and the widget state they drive. requests.Session.post is mocked and
the widget runs offscreen (conftest.py); no board is involved.

The distinction these guard is the one that is easy to get wrong: burning a
zero REDEFINES level and is written to flash, while re-syncing to gravity only
corrects the estimate. A GUI that blurs them invites a technician to burn a
zero on a slope.
"""
import json
import sys
import os
from unittest.mock import MagicMock, patch

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from PyQt5.QtWidgets import QApplication, QMessageBox
from api_client import ApiClient
from main_widget import MainWidget

_app = QApplication.instance() or QApplication([])


def make_client():
    client = ApiClient("192.168.1.198")
    client.token = "fake-token"
    return client


def mock_response(status_code, json_data=None):
    resp = MagicMock()
    resp.status_code = status_code
    resp.json.return_value = json_data or {}
    return resp


class DummyApiClient:
    base_url = "http://dummy/"

    def __init__(self):
        self.get_config_return = None
        self.set_imu_axis_map_return = (True, "ok")
        self.set_imu_axis_map_calls = []
        self.set_imu_axis_map_inverts = []
        self.burn_return = (True, {"success": True, "mountPitch": 0.0, "mountRoll": 0.0})
        self.calibrate_return = (True, {"success": True, "pitch": 0.0, "roll": 0.0, "yaw": 0.0})
        self.burn_calls = 0
        self.calibrate_calls = 0

    def get_config(self):
        return self.get_config_return

    def get_status(self):
        return None

    def get_imu(self):
        return {"pitch": 0.0, "roll": 0.0}

    def set_imu_axis_map(self, *args, **kwargs):
        return self.set_imu_axis_map_return

    def burn_zero_calibration(self):
        self.burn_calls += 1
        return self.burn_return

    def calibrate_now(self):
        self.calibrate_calls += 1
        return self.calibrate_return


def make_widget():
    return MainWidget(DummyApiClient(), "192.168.1.198")


def status(**imu_fields):
    return {"imu": dict(imu_fields)}


# ---------------------------------------------------------------- api_client

def test_burn_sends_the_burn_command():
    client = make_client()
    client.session.post = MagicMock(return_value=mock_response(
        200, {"type": "zero_calibration_result", "success": True,
              "mountPitch": -2.31, "mountRoll": 0.88}))

    ok, data = client.burn_zero_calibration()

    sent = json.loads(client.session.post.call_args.kwargs["data"])
    assert sent == {"type": "burn_zero_calibration", "token": "fake-token"}
    assert ok is True
    assert data["mountPitch"] == -2.31


def test_calibrate_now_sends_the_other_command():
    client = make_client()
    client.session.post = MagicMock(return_value=mock_response(
        200, {"type": "calibrate_now_result", "success": True,
              "pitch": 12.4, "roll": -0.6, "yaw": 0.0}))

    ok, data = client.calibrate_now()

    sent = json.loads(client.session.post.call_args.kwargs["data"])
    assert sent == {"type": "calibrate_now", "token": "fake-token"}
    assert ok is True
    assert data["pitch"] == 12.4


# A refusal is HTTP 409, not 200. The body still carries the reason, so it must
# survive rather than being flattened into a bare failure like other endpoints.
def test_refusal_keeps_the_reason_from_a_409():
    client = make_client()
    client.session.post = MagicMock(return_value=mock_response(
        409, {"success": False, "code": "E-300",
              "message": "machine is not standing still", "restSeconds": 0.4}))

    ok, data = client.burn_zero_calibration()

    assert ok is False
    assert data["message"] == "machine is not standing still"
    assert data["restSeconds"] == 0.4


def test_expired_token_is_reported_as_auth_error():
    client = make_client()
    client.session.post = MagicMock(return_value=mock_response(401, {}))

    ok, data = client.calibrate_now()

    assert ok is False
    assert data["error"] == "AUTH_ERROR"


# --------------------------------------------------------------- widget state

# The whole reason this line exists: an uncalibrated board's angles look
# perfectly reasonable, so only an explicit statement distinguishes it.
def test_uncalibrated_board_says_so():
    widget = make_widget()
    widget.apply_calibration_state(status(zeroCalValid=False, atRest=True, restSeconds=9.0))

    assert "NO zero calibration" in widget.cal_status_label.text()
    assert "red" in widget.cal_status_label.styleSheet()


def test_calibrated_board_shows_its_mounting_angles():
    widget = make_widget()
    widget.apply_calibration_state(
        status(zeroCalValid=True, mountPitch=-2.31, mountRoll=0.88,
               atRest=True, restSeconds=9.0))

    text = widget.cal_status_label.text()
    assert "-2.31" in text and "0.88" in text
    assert "NO zero calibration" not in text


def test_moving_machine_disables_both_buttons():
    widget = make_widget()
    widget.apply_calibration_state(
        status(zeroCalValid=True, mountPitch=0.0, mountRoll=0.0,
               atRest=False, restSeconds=0.4))

    assert not widget.burn_zero_btn.isEnabled()
    assert not widget.calibrate_now_btn.isEnabled()
    assert "moving" in widget.cal_status_label.text()


def test_stationary_machine_enables_both_buttons():
    widget = make_widget()
    widget.apply_calibration_state(
        status(zeroCalValid=True, mountPitch=0.0, mountRoll=0.0,
               atRest=True, restSeconds=9.0))

    assert widget.burn_zero_btn.isEnabled()
    assert widget.calibrate_now_btn.isEnabled()


# Plain V1.5.1 answers "unknown request type" to both commands. Leaving the
# buttons live would offer a technician two controls that can only ever fail.
def test_firmware_without_the_commands_disables_both_buttons():
    widget = make_widget()
    widget.apply_calibration_state(status(pitch=1.0, roll=2.0))

    assert not widget.burn_zero_btn.isEnabled()
    assert not widget.calibrate_now_btn.isEnabled()
    assert "no calibration commands" in widget.cal_status_label.text()


# A flat get_status - a UDP packet, or older firmware - must still drive the
# line rather than being read as "no calibration support".
def test_flat_payload_is_read_too():
    widget = make_widget()
    widget.apply_calibration_state(
        {"zeroCalValid": True, "mountPitch": 1.5, "mountRoll": -0.5,
         "atRest": True, "restSeconds": 9.0})

    assert "1.50" in widget.cal_status_label.text()


# ------------------------------------------------------------ widget handlers

# Burning overwrites the stored calibration, so a mis-click on a slope is
# expensive. It must not reach the board without a confirmation.
@patch("main_widget.QMessageBox.question", return_value=QMessageBox.No)
def test_declining_the_confirmation_does_not_burn(mock_question):
    widget = make_widget()
    widget.burn_zero_calibration()

    assert widget.api_client.burn_calls == 0
    mock_question.assert_called_once()


@patch("main_widget.QMessageBox.information")
@patch("main_widget.QMessageBox.question", return_value=QMessageBox.Yes)
def test_accepting_the_confirmation_burns(mock_question, mock_info):
    widget = make_widget()
    widget.burn_zero_calibration()

    assert widget.api_client.burn_calls == 1
    mock_info.assert_called_once()


@patch("main_widget.QMessageBox.warning")
@patch("main_widget.QMessageBox.question", return_value=QMessageBox.Yes)
def test_a_refused_burn_warns_and_says_nothing_changed(mock_question, mock_warning):
    widget = make_widget()
    widget.api_client.burn_return = (False, {"message": "machine is not standing still"})

    widget.burn_zero_calibration()

    assert mock_warning.call_args.args[2].startswith("machine is not standing still")
    assert "Nothing was changed" in mock_warning.call_args.args[2]


# Re-syncing is not destructive, so it goes straight through - but at rest the
# filter has often converged already, and a dialog saying only "done" leaves
# the technician unable to tell that from a no-op.
@patch("main_widget.QMessageBox.information")
def test_resync_reports_the_correction_it_applied(mock_info):
    widget = make_widget()
    widget.api_client.calibrate_return = (
        True, {"success": True, "pitch": 3.5, "roll": -1.0, "yaw": 0.0})

    widget.calibrate_now()

    assert widget.api_client.calibrate_calls == 1
    body = mock_info.call_args.args[2]
    assert "+3.50" in body and "-1.00" in body


@patch("main_widget.QMessageBox.warning")
def test_a_refused_resync_warns(mock_warning):
    widget = make_widget()
    widget.api_client.calibrate_return = (False, {"message": "machine is not standing still"})

    widget.calibrate_now()

    mock_warning.assert_called_once()
