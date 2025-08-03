#!/usr/bin/env python3
"""
Simple simulation controller for Donkeycar
This script provides a basic simulated controller that can interface with 
Donkeycar without requiring the full CPM Lab components and RTI Connext DDS license
"""

import os
import sys
import time
import math
import argparse
import logging

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("Simulation Controller")

# Import local configuration
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import config

class SimulationController:
    """Simple controller that generates steering and throttle commands"""
    
    def __init__(self, vehicle_id=1, controller_type="circle"):
        self.vehicle_id = vehicle_id
        self.controller_type = controller_type
        self.time_start = time.time()
        self.frequency = config.CONTROLLER_FREQUENCY
        self.max_speed = config.MAX_SPEED
        self.max_steering = config.MAX_STEERING_ANGLE
        logger.info(f"Initialized simulation controller for vehicle {vehicle_id}")
        logger.info(f"Controller type: {controller_type}")
        
    def get_circle_control(self, t):
        """Generate control signals for circular path"""
        # Simple sine wave for steering to create circular motion
        period = 5.0  # seconds to complete one cycle
        amplitude = 0.8  # steering amplitude
        
        steering = amplitude * math.sin(2 * math.pi * t / period)
        throttle = 0.3  # constant throttle
        
        return steering, throttle
        
    def get_figure_eight_control(self, t):
        """Generate control signals for figure-eight path"""
        # Using lemniscate of Bernoulli formula
        period = 10.0  # seconds to complete figure eight
        amplitude = 0.9  # steering amplitude
        
        # Figure-eight pattern oscillates between positive and negative steering
        # with varying magnitudes
        steering = amplitude * math.sin(2 * math.pi * t / period)
        
        # Throttle varies slightly to maintain speed in turns
        throttle_base = 0.3
        throttle_vary = 0.1
        throttle = throttle_base + throttle_vary * abs(math.cos(2 * math.pi * t / period))
        
        return steering, throttle
    
    def run(self):
        """Main control loop"""
        logger.info(f"Starting {self.controller_type} controller")
        
        try:
            while True:
                # Calculate elapsed time
                t = time.time() - self.time_start
                
                # Get control signals based on controller type
                if self.controller_type == "circle":
                    steering, throttle = self.get_circle_control(t)
                elif self.controller_type == "figure-eight":
                    steering, throttle = self.get_figure_eight_control(t)
                else:
                    steering, throttle = 0.0, 0.0
                
                # Scale to actual values if needed
                scaled_steering = steering * self.max_steering
                scaled_throttle = throttle * self.max_speed
                
                # Print control signals (this would normally send to Donkeycar)
                logger.info(f"t={t:.2f}s, Steering: {scaled_steering:.2f} degrees, Throttle: {scaled_throttle:.2f} m/s")
                
                # Simulate Donkeycar interface - in a real setup, this would send commands
                # to the Donkeycar through the bridge
                
                # Sleep to maintain control frequency
                time.sleep(1.0 / self.frequency)
                
        except KeyboardInterrupt:
            logger.info("Controller stopped by user")
        except Exception as e:
            logger.error(f"Error in controller: {e}")

def main():
    """Parse arguments and run controller"""
    parser = argparse.ArgumentParser(description='Simulation Controller for Donkeycar')
    parser.add_argument('--vehicle-id', type=int, default=1, help='Vehicle ID')
    parser.add_argument('--controller', type=str, default='circle', 
                        choices=['circle', 'figure-eight'], 
                        help='Controller type (circle or figure-eight)')
    
    args = parser.parse_args()
    
    controller = SimulationController(args.vehicle_id, args.controller)
    controller.run()

if __name__ == "__main__":
    main()