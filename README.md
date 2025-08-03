# RIDE-project
temporary name

### !!! IMPORTANT !!!

Please run the following commands to initialize and update the submodules

'''bash
git submodule init
git submodule update
'''


## Setup

0. (WINDOWS ONLY) Setup WSL (super useful for future projects trust me)
http://learn.microsoft.com/en-us/windows/wsl/install

1. Install dependencies and setup conda environment

```bash
./setup.sh
```

2. Download the SIM from https://github.com/tawnkramer/gym-donkeycar/releases

## (Optional) Create your own donkey car env (works for both physical and sim) *probably*

```bash
donkey createcar --path ./cars/<name-of-car>
```

For virtual donkey cars, you need to edit the myconfig.py file to connect to the donkey sim.

```bash
cd <name-of-car>
code myconfig.py
```

Please add the following to the bottom of the myconfig.py file:

```bash
DONKEY_GYM = True
DONKEY_SIM_PATH = "remote" 
DONKEY_GYM_ENV_NAME = "donkey-generated-track-v0" 
GYM_CONF = { "body_style" : "donkey", "body_rgb" : (128, 128, 128), "car_name" : "<name-of-car>", "font_size" : 100}
SIM_HOST = "<IP Address>" 
```

By default localhost on windows is not available to WSL, so you need to find your real IP address.

To find your IP address in WSL, run the following command:

```bash
ip route
```

The IP address to use is found in this line: `default via <IP Address>`

## Running the cars

1. Run the sim (downloaded earlier from https://github.com/tawnkramer/gym-donkeycar/releases)

2. Once running, select a course. It will appear empty for now.

3. Run the Donkey car

Be sure you are in the `donkey` conda environment!

| Type | Command |
|------|---------|
| Drive | `cd cars/<name-of-car> && python manage.py drive` |
| Train | `cd cars/<name-of-car> && donkey train --tub ./data --model models/<name-of-model>.h5` |
| Sim | `cd cars/<name-of-car> && python manage.py drive --model models/<name-of-model>.h5` |

After running the drive command you can control the car by going to `http://localhost:8887/`.

All information for the donkey car is in 
- [Donkey Installation Docs](https://docs.donkeycar.com/guide/host_pc/setup_ubuntu/)
- [Donkey Gym Docs](https://docs.donkeycar.com/guide/deep_learning/simulator/)

## Donkeycar Bridge for CPM Lab

The Donkeycar Bridge integrates donkeycars with the CPM Lab environment, enabling advanced control and visualization:

### Simulation Mode

Run a simulated donkeycar with the CPM Lab:

```bash
cd donkeycar_bridge
./launch.sh <vehicle_id> [car_path] [--figure-eight]
```

### Physical Car Network Bridge

Connect to physical donkeycars over the network:

```bash
cd donkeycar_bridge
./run_physical_car.py --ip <car_ip> --port 8887
```

Options:
- `--vehicle-id`: The CPM Lab vehicle ID (default: 1)
- `--car-name`: Name of pre-configured car in config
- `--ip`: IP address of the donkeycar
- `--port`: Port number (default: 8887)
- `--protocol`: Communication protocol (http or mqtt)

This allows physical donkeycars to be controlled from the CPM Lab environment and visualized in the Lab Control Center, with support for both HTTP and MQTT protocols.

See the [Donkeycar Bridge README](./donkeycar_bridge/README.md) for complete documentation.

# Next Steps

- Build and install the donkeycar onto the Raspberry Pi
- Use [ngrok](https://ngrok.com/) to expose the donkeycar API to the internet
- Check that the donkeycar can be controlled from WebUI
- Create custom Car Manager Controller (Middleware) to manage data from each car's API
- Setup car position tracking via camera and NASA image-matching algorithm
- Improve physical car networking with better position tracking
- Implement multi-car coordination through the CPM Lab environment
- Add support for custom neural network models running in CPM Lab
---
---
---

