"""
Simplified simulation configuration for Donkeycar Bridge
This configuration allows running the donkeycar interface without
requiring the full CPM Lab components and RTI Connext DDS license
"""

# Simulation parameters
SIMULATION_MODE = True

# Vehicle configuration
VEHICLE_ID = 1
MAX_SPEED = 1.0  # m/s
MAX_STEERING_ANGLE = 30.0  # degrees

# Track configuration 
TRACK_WIDTH = 2.0  # meters
TRACK_LENGTH = 10.0  # meters

# Controller options
CONTROLLER_TYPE = "circle"  # or "figure-eight"
CONTROLLER_FREQUENCY = 10  # Hz

# Basic donkeycar configuration
DONKEY_CONFIG = {
    'DRIVE_LOOP_HZ': 20,
    'CAMERA_RESOLUTION': (120, 160),
    'CAMERA_FRAMERATE': 20,
    'STEERING_CHANNEL': 1,
    'THROTTLE_CHANNEL': 0,
    'STEERING_LEFT_PWM': 460,
    'STEERING_RIGHT_PWM': 290,
    'THROTTLE_FORWARD_PWM': 500,
    'THROTTLE_STOPPED_PWM': 370,
    'THROTTLE_REVERSE_PWM': 220,
}

# Network communication settings
NETWORK_ENABLED = False  # Set to True to enable network communication with physical cars
DEFAULT_PORT = 8887  # Default port for donkeycar web controller
PROTOCOL = "http"  # Protocol for communication with cars (http or mqtt)

# Physical car network settings
PHYSICAL_CARS = {
    # Example: "car_name": {"ip": "192.168.1.100", "port": 8887}
    "testcar": {"ip": "localhost", "port": 8887},
    "testcar2": {"ip": "localhost", "port": 8887},
}

# MQTT settings (if using MQTT protocol)
MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_TOPIC_PREFIX = "donkey/"  # Topic format will be donkey/<car_name>/<command>