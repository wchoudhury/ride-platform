#!/usr/bin/env python3
"""
Simplified Donkeycar Bridge
This script connects the simulation controller with Donkeycar
without requiring the full CPM Lab components and RTI Connext DDS license
"""

import os
import sys
import time
import math
import argparse
import logging
import threading
import signal
import subprocess
from pathlib import Path

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("Donkeycar Bridge")

# Import local configuration
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import config

class DonkeycarBridge:
    """Bridge between simulation controller and Donkeycar"""
    
    def __init__(self, vehicle_id=1, controller_type="circle", car_path=None):
        self.vehicle_id = vehicle_id
        self.controller_type = controller_type
        self.car_path = car_path
        self.controller_process = None
        self.donkey_process = None
        self.running = False
        logger.info(f"Initialized Donkeycar Bridge for vehicle {vehicle_id}")
        logger.info(f"Controller type: {controller_type}")
        if car_path:
            logger.info(f"Car path: {car_path}")
        
    def start_controller(self):
        """Start the simulation controller in a separate process"""
        controller_script = os.path.join(os.path.dirname(os.path.abspath(__file__)), "controller.py")
        cmd = [
            "python",
            controller_script,
            "--vehicle-id", str(self.vehicle_id),
            "--controller", self.controller_type
        ]
        
        logger.info(f"Starting controller: {' '.join(cmd)}")
        self.controller_process = subprocess.Popen(
            cmd, 
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True
        )
        
        # Start a thread to read and log controller output
        def log_controller_output():
            for line in self.controller_process.stdout:
                logger.debug(f"Controller: {line.strip()}")
        
        threading.Thread(target=log_controller_output, daemon=True).start()
        logger.info("Controller started")
    
    def start_donkeycar(self):
        """Start Donkeycar in drive mode if a car path is provided"""
        if not self.car_path:
            logger.info("No car path provided, skipping Donkeycar startup")
            return
        
        # Verify car path exists
        car_path = Path(self.car_path)
        if not car_path.exists() or not car_path.is_dir():
            logger.error(f"Car path does not exist or is not a directory: {self.car_path}")
            return
        
        # Check for manage.py
        manage_py = car_path / "manage.py"
        if not manage_py.exists():
            logger.error(f"manage.py not found in car path: {self.car_path}")
            return
        
        # Start Donkeycar in drive mode
        cmd = [
            "python",
            str(manage_py),
            "drive",
            "--js"  # Use joystick for input
        ]
        
        logger.info(f"Starting Donkeycar: {' '.join(cmd)}")
        self.donkey_process = subprocess.Popen(
            cmd,
            cwd=str(car_path),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True
        )
        
        # Start a thread to read and log Donkeycar output
        def log_donkeycar_output():
            for line in self.donkey_process.stdout:
                logger.debug(f"Donkeycar: {line.strip()}")
        
        threading.Thread(target=log_donkeycar_output, daemon=True).start()
        logger.info("Donkeycar started")
    
    def run(self):
        """Run the bridge"""
        self.running = True
        
        # Set up signal handler for clean shutdown
        def signal_handler(sig, frame):
            logger.info("Received signal to shut down")
            self.shutdown()
            sys.exit(0)
        
        signal.signal(signal.SIGINT, signal_handler)
        signal.signal(signal.SIGTERM, signal_handler)
        
        # Start components
        self.start_controller()
        self.start_donkeycar()
        
        logger.info("Bridge is running. Press Ctrl+C to stop.")
        
        # Main loop - keep the bridge running
        try:
            while self.running:
                # Check if processes are still running
                if self.controller_process and self.controller_process.poll() is not None:
                    logger.error("Controller process has exited")
                    self.running = False
                    break
                
                if self.donkey_process and self.donkey_process.poll() is not None:
                    logger.error("Donkeycar process has exited")
                    self.running = False
                    break
                
                time.sleep(1)
                
        except KeyboardInterrupt:
            logger.info("Bridge stopped by user")
        except Exception as e:
            logger.error(f"Error in bridge: {e}")
        finally:
            self.shutdown()
    
    def shutdown(self):
        """Shut down all components"""
        logger.info("Shutting down Donkeycar Bridge")
        
        # Terminate controller process
        if self.controller_process:
            logger.info("Terminating controller process")
            self.controller_process.terminate()
            try:
                self.controller_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                logger.warning("Controller process did not terminate, killing it")
                self.controller_process.kill()
        
        # Terminate Donkeycar process
        if self.donkey_process:
            logger.info("Terminating Donkeycar process")
            self.donkey_process.terminate()
            try:
                self.donkey_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                logger.warning("Donkeycar process did not terminate, killing it")
                self.donkey_process.kill()
        
        self.running = False
        logger.info("Shutdown complete")

def main():
    """Parse arguments and run bridge"""
    parser = argparse.ArgumentParser(description='Simplified Donkeycar Bridge')
    parser.add_argument('--vehicle-id', type=int, default=1, help='Vehicle ID')
    parser.add_argument('--controller', type=str, default='circle', 
                        choices=['circle', 'figure-eight'], 
                        help='Controller type (circle or figure-eight)')
    parser.add_argument('--car-path', type=str, help='Path to Donkeycar directory')
    
    args = parser.parse_args()
    
    bridge = DonkeycarBridge(args.vehicle_id, args.controller, args.car_path)
    bridge.run()

if __name__ == "__main__":
    main()