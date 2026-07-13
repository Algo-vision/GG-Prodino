"""
Configuration for MQTT Subscriber Server
"""

# MQTT Broker Configuration
MQTT_BROKER = "localhost"  # Change to "192.168.1.1" for RUTX12
MQTT_PORT = 1883
MQTT_USERNAME = None  # Set to "python_server" when authentication enabled
MQTT_PASSWORD = None  # Set password when authentication enabled
MQTT_CLIENT_ID = "python_server_001"
MQTT_KEEPALIVE = 60
SUBSCRIBE_TOPICS = ["prodino/#"]

# Web Server Configuration
WEB_SERVER_HOST = "0.0.0.0"
WEB_SERVER_PORT = 5000
DEBUG = False  # Set to False to avoid Flask reloader creating duplicate MQTT connections

# Data retention
MAX_HISTORY_SIZE = 1000  # Maximum number of historical data points to keep
