"""
Unit tests for the per-section data toggle: each status QGroupBox doubles as
a checkbox that drops its section from the get_status poll ("groups" field).
Uses the same offscreen-Qt setup as test_main_widget.py.
"""
import json
import sys
import os
from unittest.mock import MagicMock

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from PyQt5.QtWidgets import QApplication
from main_widget import MainWidget, STATUS_SECTIONS
from api_client import ApiClient
import requests

_app = QApplication.instance() or QApplication([])

from test_main_widget import DummyApiClient

ALL_SECTIONS = [s for s, _t, _f in STATUS_SECTIONS]

# Minimal successful status document - enough for update_status's happy path.
FULL_STATUS = {
    "type": "status", "ledIo": "OFF",
    "config": {"technicianMode": False, "controllerIp": "192.168.1.198",
               "whitelistIps": []},
    "overview": {}, "imu": {"zeroCalValid": True, "atRest": True,
                            "mountPitch": 0.0, "mountRoll": 0.0,
                            "restSeconds": 5.0},
    "gps": {},
}


def make_widget():
    client = DummyApiClient()
    client.get_status_return = dict(FULL_STATUS)
    return MainWidget(client, "192.168.1.198"), client


def test_all_section_boxes_start_checked():
    widget, _client = make_widget()
    for section in ALL_SECTIONS:
        assert widget.status_groups[section].isCheckable()
        assert widget.status_groups[section].isChecked()


def test_all_checked_omits_groups_field():
    widget, client = make_widget()
    widget.update_status()
    assert client.get_status_calls == [None]


def test_unchecking_gps_requests_the_other_three():
    widget, client = make_widget()
    widget.status_groups["gps"].setChecked(False)
    widget.update_status()
    assert client.get_status_calls == [["config", "overview", "imu"]]


def test_only_imu_checked_requests_only_imu():
    widget, client = make_widget()
    for section in ALL_SECTIONS:
        widget.status_groups[section].setChecked(section == "imu")
    widget.update_status()
    assert client.get_status_calls == [["imu"]]


def test_unchecked_imu_does_not_touch_calibration_state():
    widget, client = make_widget()
    widget.status_groups["imu"].setChecked(False)
    client.get_status_return = {k: v for k, v in FULL_STATUS.items() if k != "imu"}
    widget.cal_status_label.setText("SENTINEL")
    widget.update_status()
    # An absent (untoggled) imu section must not be read as "firmware has no
    # calibration commands".
    assert widget.cal_status_label.text() == "SENTINEL"


def test_unchecked_config_does_not_drop_technician_mode():
    widget, client = make_widget()
    widget.technician_mode = True
    widget.status_groups["config"].setChecked(False)
    client.get_status_return = {k: v for k, v in FULL_STATUS.items() if k != "config"}
    widget.update_status()
    assert widget.technician_mode is True


# --- ApiClient payload wire format ---

def _mock_post(status_code=200, json_data=None):
    resp = MagicMock()
    resp.status_code = status_code
    resp.json.return_value = json_data if json_data is not None else {"type": "status"}
    return MagicMock(return_value=resp)


def test_api_client_omits_groups_by_default():
    client = ApiClient("192.168.1.198")
    client.token = "fake-token"
    client.session.post = _mock_post()
    client.get_status()
    sent = json.loads(client.session.post.call_args.kwargs["data"])
    assert sent == {"type": "get_status", "token": "fake-token"}


def test_api_client_sends_groups_when_given():
    client = ApiClient("192.168.1.198")
    client.token = "fake-token"
    client.session.post = _mock_post()
    client.get_status(groups=["imu", "gps"])
    sent = json.loads(client.session.post.call_args.kwargs["data"])
    assert sent == {"type": "get_status", "token": "fake-token",
                    "groups": ["imu", "gps"]}


# --- HTTP rate label ---

def test_rate_label_starts_blank():
    widget, _client = make_widget()
    assert widget.http_rate_label.text() == "HTTP: -"


def test_rate_label_updates_after_polls():
    widget, _client = make_widget()
    # Feed synthetic round trips directly - update_status in a tight test
    # loop would measure the mock, not the mechanism.
    t = [1000.0]
    import main_widget as mw
    real_time = mw.time.time
    mw.time.time = lambda: t[0]
    try:
        for _ in range(5):
            widget.record_poll(0.030)
            t[0] += 0.05
    finally:
        mw.time.time = real_time
    text = widget.http_rate_label.text()
    assert "Hz" in text and "ms/poll" in text
    # 5 polls, 50 ms apart -> 20 Hz; 30 ms mean round trip
    assert "20.0 Hz" in text and "30 ms" in text


def test_rate_label_resets_on_comm_loss(monkeypatch):
    widget, client = make_widget()
    widget.update_status()
    assert widget.http_rate_label.text() != "HTTP: -"
    client.get_status_return = None
    # Comm-loss path pops a modal QMessageBox - neutralize it for the test.
    from PyQt5.QtWidgets import QMessageBox
    monkeypatch.setattr(QMessageBox, "critical", staticmethod(lambda *a, **k: None))
    widget.update_status()
    assert widget.http_rate_label.text() == "HTTP: -"
