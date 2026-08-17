"""
Unit tests for api_client.py's request/response handling, focused on the
endpoints added today (get_imu, get_config, set_imu_mount_orientation).
requests.Session.post is mocked - no real network calls.
"""
import json
import sys
import os
from unittest.mock import MagicMock
import requests

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from api_client import ApiClient


def make_client():
    client = ApiClient("192.168.1.198")
    client.token = "fake-token"
    return client


def mock_response(status_code, json_data=None):
    resp = MagicMock()
    resp.status_code = status_code
    resp.json.return_value = json_data or {}
    return resp


def test_get_imu_sends_correct_payload():
    client = make_client()
    client.session.post = MagicMock(return_value=mock_response(200, {"type": "imu", "pitch": 1.0}))

    result = client.get_imu()

    sent_payload = json.loads(client.session.post.call_args.kwargs["data"])
    assert sent_payload == {"type": "get_imu", "token": "fake-token"}
    assert result == {"type": "imu", "pitch": 1.0}


def test_get_imu_401_returns_auth_error():
    client = make_client()
    client.session.post = MagicMock(return_value=mock_response(401))

    result = client.get_imu()

    assert result == {"error": "AUTH_ERROR"}


def test_get_imu_connection_error_returns_none():
    client = make_client()
    client.session.post = MagicMock(side_effect=requests.exceptions.ConnectionError())

    result = client.get_imu()

    assert result is None


def test_get_config_sends_correct_payload():
    client = make_client()
    client.session.post = MagicMock(return_value=mock_response(
        200, {"type": "config", "imuPitchAxis": "X", "imuRollAxis": "Y", "imuYawAxis": "Z"}))

    result = client.get_config()

    sent_payload = json.loads(client.session.post.call_args.kwargs["data"])
    assert sent_payload == {"type": "get_config", "token": "fake-token"}
    assert result["imuPitchAxis"] == "X"
    assert result["imuRollAxis"] == "Y"
    assert result["imuYawAxis"] == "Z"


def test_set_imu_axis_map_sends_correct_payload():
    client = make_client()
    client.session.post = MagicMock(return_value=mock_response(
        200, {"success": True, "message": "IMU axis map set to: pitch=Z roll=X yaw=Y"}))

    ok, msg = client.set_imu_axis_map("Z", "X", "Y")

    sent_payload = json.loads(client.session.post.call_args.kwargs["data"])
    assert sent_payload == {
        "type": "set_imu_axis_map", "token": "fake-token",
        "pitch_axis": "Z", "roll_axis": "X", "yaw_axis": "Y",
    }
    assert ok is True
    assert "pitch=Z" in msg


def test_set_imu_axis_map_reports_firmware_rejection():
    client = make_client()
    client.session.post = MagicMock(return_value=mock_response(
        200, {"type": "error", "message": "Pitch, roll and yaw must each use a different axis"}))

    ok, msg = client.set_imu_axis_map("X", "X", "Z")

    assert ok is False
    assert msg == "Pitch, roll and yaw must each use a different axis"


def test_get_config_timeout_returns_none():
    client = make_client()
    client.session.post = MagicMock(side_effect=requests.exceptions.Timeout())

    result = client.get_config()

    assert result is None


def test_set_imu_axis_map_401_returns_auth_error_tuple():
    client = make_client()
    client.session.post = MagicMock(return_value=mock_response(401))

    ok, msg = client.set_imu_axis_map("X", "Y", "Z")

    assert ok is False
    assert msg == "Authentication Error"


def test_set_imu_axis_map_connection_error():
    client = make_client()
    client.session.post = MagicMock(side_effect=requests.exceptions.ConnectionError())

    ok, msg = client.set_imu_axis_map("X", "Y", "Z")

    assert ok is False
    assert msg == "Connection Error"
