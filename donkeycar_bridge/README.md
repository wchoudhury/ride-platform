# Donkeycar Bridge for CPM Lab

This project provides a bridge between the Donkeycar framework and the CPM Lab system, allowing Donkeycar-based vehicles (Python Raspberry Pi cars) to be controlled and visualized within the CPM Lab environment.

## Overview

The Donkeycar Bridge integrates the Donkeycar API with the CPM Lab's distributed control architecture, enabling:

1. **Control of Donkeycars** via CPM Lab's high-level controllers
2. **Visualization** of vehicle movement, control inputs, and camera feeds
3. **Parameter management** compatible with both CPM and Donkeycar systems
4. **Real-time data exchange** between CPM's C++ components and Donkeycar's Python API
5. **Network communication** with physical donkeycars over HTTP or MQTT protocols

## Key Components

### Core Bridge Components

- **DDS Bridge** (`src/dds_bridge.py`): Central communication module that translates between CPM's DDS messages and Donkeycar controls
- **Vehicle Adapter** (`src/vehicle_adapter.py`): Makes a Donkeycar appear as a CPM Lab vehicle
- **Parameter Config** (`src/parameter_config.py`): Sets up CPM parameters for Donkeycar vehicles
- **Visualizer** (`src/visualizer.py`): Sends visualization data to the Lab Control Center
- **Network Controller** (`src/network_controller.py`): Communicates with physical donkeycars over the network
- **Physical Car Bridge** (`src/physical_car_bridge.py`): Bridge implementation for connecting to physical cars

### Visualization Components

- **LCC Integration** (`lcc_integration/`): C++ plugin for the Lab Control Center to display Donkeycar data
  - Camera feed visualization
  - Vehicle path visualization
  - Control input visualization (steering, throttle)

### Example Controllers

- **Circle Controller** (`examples/circle_controller.py`): Makes a Donkeycar drive in a circle
- **Figure Eight Controller** (`examples/figure_eight_controller.py`): Makes a Donkeycar follow a figure-eight trajectory

## Getting Started

### Prerequisites

- CPM Lab system installed and functional
- Donkeycar framework installed
- Raspberry Pi configured with Donkeycar firmware

### Installation

1. Ensure the CPM Lab system is installed and working
2. Install the Donkeycar framework: `pip install -e /path/to/donkeycar`
3. Install the gym-donkeycar package: `pip install -e /path/to/gym-donkeycar`
4. Install the donkeycar_bridge package: `pip install -e .`

### Building the Lab Control Center Integration

```bash
cd lcc_integration
mkdir build && cd build
cmake ..
make
sudo make install
```

## Using the Bridge with Donkeycar

### Preparing a Donkeycar for Bridge Connectivity

#### Setting Up a Virtual (Simulated) Donkeycar

1. Ensure you have the donkeycar and gym-donkeycar packages installed
2. Create a car configuration that enables the simulator:

```python
# In your myconfig.py:
DONKEY_GYM = True
DONKEY_SIM_PATH = "path/to/simulator" # For local simulator
# OR
DONKEY_SIM_PATH = "remote" # For remote simulator

# Set simulator configuration
DONKEY_GYM_ENV_NAME = "donkey-generated-track-v0"
GYM_CONF = { 
    "body_style": "donkey", 
    "body_rgb": (128, 128, 128), 
    "car_name": "car", 
    "font_size": 100
}

# Set simulator host
SIM_HOST = "127.0.0.1" # For local operation
```

#### Setting Up a Physical Donkeycar

1. Ensure your car is running the standard Donkeycar web server
2. Make sure your `myconfig.py` enables the web controller:
```python
# In your physical car's myconfig.py:
WEB_CONTROL_PORT = 8887  # Port for the web controller
```

3. For MQTT-based communication (optional but recommended for lower latency):
```python
HAVE_MQTT_TELEMETRY = True
TELEMETRY_MQTT_TOPIC_TEMPLATE = 'donkey/%s/telemetry'
TELEMETRY_MQTT_BROKER_HOST = 'broker.hostname' # MQTT broker to use
TELEMETRY_MQTT_BROKER_PORT = 1883
TELEMETRY_PUBLISH_PERIOD = 1
```

### Starting a Virtual Donkeycar

To start a virtual donkeycar that can work with the bridge:

```bash
cd /path/to/your/testcar
python manage.py drive --js
```

This will start the virtual donkeycar in simulator mode.

### Starting a Physical Donkeycar 

On the Raspberry Pi running your donkeycar:

```bash
cd ~/mycar
python manage.py drive
```

This will start the webserver on port 8887 by default, which the bridge can connect to.

### Using the Bridge

#### With Virtual Donkeycars

Start a Donkeycar with the CPM Lab system:

```bash
./launch.sh <vehicle_id> [car_path] [--figure-eight]
```

Options:
- `vehicle_id`: The ID to use for the vehicle in the CPM system
- `car_path`: (Optional) Path to a Donkeycar directory to load configuration from
- `--figure-eight`: (Optional) Use the figure-eight controller instead of the circle controller

#### With Physical Cars over the Network

Connect to a physical donkeycar over the network:

```bash
./run_physical_car.py --vehicle-id <id> --ip <ip_address> --port <port>
```

Options:
- `--vehicle-id`: The ID to use for the vehicle in the CPM system (default: 1)
- `--config`: Path to configuration file (default: uses simulation/config.py)
- `--car-name`: Name of pre-configured car in config file
- `--ip`: IP address of the physical donkeycar
- `--port`: Port number for the physical donkeycar (default: 8887)
- `--protocol`: Communication protocol to use (http or mqtt, default: http)

Examples:
```bash
# Basic usage (connects to localhost:8887)
./run_physical_car.py

# Connect to a specific car by IP address
./run_physical_car.py --ip 192.168.1.100 --port 8887

# Use a pre-configured car defined in the config file
./run_physical_car.py --car-name testcar2

# Connect to a physical car using MQTT protocol
./run_physical_car.py --ip 192.168.1.100 --protocol mqtt
```

### Troubleshooting the Bridge Connection

If you're having trouble connecting:

1. **Check network connectivity**:
   ```bash
   ping <car_ip_address>
   ```

2. **Verify the webserver is running** on the physical car:
   ```bash
   curl http://<car_ip>:8887/
   ```

3. **Check the MQTT broker** (if using MQTT protocol):
   ```bash
   mosquitto_sub -h <mqtt_broker_host> -t "donkey/#" -v
   ```

4. **Examine bridge logs** for connection issues:
   ```bash
   # Look for connection messages in the terminal output
   # x indicates connection failure, . indicates successful connection
   ```

5. **Restart the bridge** if connection was lost:
   ```bash
   # Press Ctrl+C to stop the running bridge
   # Then restart with appropriate parameters
   ./run_physical_car.py --ip <car_ip>
   ```

## Implementation Status

### Completed Features

- [x] Core DDS bridge for communication between CPM and Donkeycar
- [x] Parameter configuration system for Donkeycar in CPM
- [x] Basic high-level controllers (circle, figure-eight)
- [x] Vehicle state visualization in CPM Lab Control Center
- [x] Camera feed visualization infrastructure
- [x] Position and trajectory visualization
- [x] Control input visualization (steering, throttle gauges)
- [x] Network communication with physical donkeycar vehicles
- [x] Support for both HTTP and MQTT protocols for network communication
- [x] Camera feed streaming from physical cars

### Features in Progress

- [ ] Full integration of the camera viewer with Lab Control Center's main UI
- [ ] Physical position tracking with indoor positioning system
- [ ] Error handling and recovery for communication failures
- [ ] Documentation of all API components

### Planned Future Enhancements

- [ ] 3D vehicle models for visualization
- [ ] Neural network visualization for autonomous driving models
- [ ] Multi-camera support
- [ ] Augmented reality overlay for detected objects and predicted paths
- [ ] Performance optimization for high-frequency control
- [ ] Support for multiple simultaneous Donkeycar vehicles
- [ ] Integration with CPM Lab's recording and replay functionality
- [ ] Web-based visualization and control interface

## Architecture

```
                     ┌───────────────┐
                     │   CPM Lab     │
                     │  Ecosystem    │
                     └───────┬───────┘
                             │ DDS
                             ▼
┌───────────────────────────────────────────────┐
│              Donkeycar Bridge                 │
├───────────────┬───────────────┬───────────────┤
│   DDS Bridge  │    Vehicle    │  Parameter    │
│               │    Adapter    │    Config     │
└───────┬───────┴───────┬───────┴───────┬───────┘
        │               │               │
        ▼               ▼               ▼
┌───────────────┐ ┌───────────────┐ ┌───────────────┐
│   Donkeycar   │ │ High-Level    │ │ Visualization │
│      API      │ │ Controllers   │ │    System     │
└───────┬───────┘ └───────────────┘ └───────────────┘
        │
        ▼
┌───────────────────────────────────────────────┐
│        Physical Car Network Bridge            │
├───────────────┬───────────────┬───────────────┤
│  HTTP/REST    │     MQTT      │  Telemetry    │
│    Client     │    Client     │  Collection   │
└───────┬───────┴───────┬───────┴───────────────┘
        │               │
        ▼               ▼
      ┌─────────────────────────┐
      │    Physical Donkeycar   │
      └─────────────────────────┘
```

## Network Communication

The bridge supports two protocols for communicating with physical donkeycar vehicles:

### HTTP Protocol
- RESTful API communication with the donkeycar web server
- Endpoints for sending steering/throttle commands and receiving telemetry
- Camera feed streaming via HTTP requests
- Default port: 8887 (standard donkeycar web interface port)

### MQTT Protocol
- Publish/subscribe messaging for real-time communication
- Lower latency for time-critical applications
- Topics for commands, telemetry, and camera data
- Requires MQTT broker (e.g., Mosquitto)

## Integration with Lab Control Center

The visualization components integrate with the Lab Control Center through:

1. Vehicle path and position visualization using visualization commands
2. Control input visualization (steering, throttle) as screen overlays
3. Camera feed visualization via a custom QML component

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/new-feature`
3. Commit your changes: `git commit -am 'Add new feature'`
4. Push to the branch: `git push origin feature/new-feature`
5. Submit a pull request

## License

This project is licensed under the same license as the CPM Lab system.

## Next Steps and Roadmap

### Short-term Goals

1. Complete the Lab Control Center integration for camera visualization
2. Add position tracking with indoor positioning system or visual SLAM
3. Improve error handling and system robustness
4. Add comprehensive API documentation

### Medium-term Goals

1. Implement advanced visualization features (3D models, neural network visualization)
2. Support multiple simultaneous Donkeycar vehicles
3. Add more sophisticated controllers and path planning

### Long-term Goals

1. Full integration with CPM Lab's recording and replay functionality
2. Advanced machine learning integration
3. Web-based visualization and control interface
4. Support for complex multi-vehicle scenarios

## Known Issues

- Position tracking currently relies on simulation rather than actual positioning data
- Camera visualization requires manual integration with Lab Control Center
- System startup order is important (Lab Control Center must be running first)
- Communication can be unreliable under heavy network load
- Physical car position estimation is approximate without external positioning system