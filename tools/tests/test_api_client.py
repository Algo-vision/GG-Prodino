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
    client.session.post = MagicMock(return_value=mock_response(200, {"type": "config", "imuMountOrientation": "STANDING"}))

    result = client.get_config()

    sent_payload = json.loads(client.session.post.call_args.kwargs["data"])
    assert sent_payload == {"type": "get_config", "token": "fake-token"}
    assert result["imuMountOrientation"] == "STANDING"


def test_get_config_timeout_returns_none():
    client = make_client()
    client.session.post = MagicMock(side_effect=requests.exceptions.Timeout())

    result = client.get_config()

    assert result is None


def test_set_imu_mount_orientation_sends_correct_payload():
    client = make_client()
    client.session.post = MagicMock(return_value=mock_response(200, {"success": True, "message": "IMU mount orientation set to: TILT_LEFT"}))

    ok, msg = client.set_imu_mount_orientation("TILT_LEFT")

    sent_payload = json.loads(client.session.post.call_args.kwargs["data"])
    assert sent_payload == {"type": "set_imu_mount_orientation", "token": "fake-token", "orientation": "TILT_LEFT"}
    assert ok is True
    assert "TILT_LEFT" in msg


def test_set_imu_mount_orientation_failure_response():
    client = make_client()
    client.session.post = MagicMock(return_value=mock_response(200, {"type": "error", "message": "Invalid IMU mount orientation"}))

    ok, msg = client.set_imu_mount_orientation("GARBAGE")

    assert ok is False
    assert msg == "Invalid IMU mount orientation"


def test_set_imu_mount_orientation_401_returns_auth_error_tuple():
    client = make_client()
    client.session.post = MagicMock(return_value=mock_response(401))

    ok, msg = client.set_imu_mount_orientation("STANDING")

    assert ok is False
    assert msg == "Authentication Error"


def test_set_imu_mount_orientation_connection_error():
    client = make_client()
    client.session.post = MagicMock(side_effect=requests.exceptions.ConnectionError())

    ok, msg = client.set_imu_mount_orientation("STANDING")

    assert ok is False
    assert msg == "Connection Error"
