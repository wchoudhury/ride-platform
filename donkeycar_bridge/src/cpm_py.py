"""
Python bindings for CPM Lab framework to be used with Donkeycar.
This module provides access to CPM's DDS communication system.
"""

import ctypes
import os
import sys
import time
import numpy as np
from enum import Enum

# Load the CPM library paths
CPM_LIB_PATH = os.environ.get('CPM_LIB_PATH', '/home/icarus/school/RIDE-project/cpm_lab/cpm_lib/build/lib')
sys.path.append(CPM_LIB_PATH)

try:
    # Try to import the CPM library
    import cpm_lib as _cpm
except ImportError:
    print("Warning: Could not import CPM library. Using mock implementation.")
    # Create a mock implementation for testing without CPM
    class _MockCPM:
        """Mock implementation of CPM library for testing"""
        
        class LogLevel(Enum):
            Debug = 0
            Info = 1
            Warn = 2
            Error = 3
            Fatal = 4
        
        class Logging:
            _instance = None
            
            @classmethod
            def Instance(cls):
                if cls._instance is None:
                    cls._instance = cls()
                return cls._instance
                
            def set_id(self, id_str):
                print(f"Mock: Setting ID to {id_str}")
                
            def write(self, level, message):
                levels = ["DEBUG", "INFO", "WARN", "ERROR", "FATAL"]
                print(f"Mock Log [{levels[level.value]}]: {message}")
        
        class Reader:
            def __init__(self, topic):
                self.topic = topic
                print(f"Mock: Created reader for topic {topic}")
                
            def read(self):
                return None
        
        class AsyncReader:
            def __init__(self, topic, callback):
                self.topic = topic
                self.callback = callback
                print(f"Mock: Created async reader for topic {topic}")
        
        class Writer:
            def __init__(self, topic):
                self.topic = topic
                print(f"Mock: Created writer for topic {topic}")
                
            def write(self, data):
                print(f"Mock: Writing to topic {self.topic}: {data}")
        
        class ParameterReceiver:
            def __init__(self):
                self.params = {}
                
            def get_parameter_double(self, key):
                return self.params.get(key, 0.0)
                
            def get_parameter_int(self, key):
                return self.params.get(key, 0)
                
            def get_parameter_bool(self, key):
                return self.params.get(key, False)
                
            def get_parameter_string(self, key):
                return self.params.get(key, "")
        
        @staticmethod
        def init(id_str):
            print(f"Mock: Initializing CPM with ID {id_str}")
            
        @staticmethod
        def get_time_ns():
            return int(time.time() * 1e9)
    
    # Use the mock implementation
    _cpm = _MockCPM()

# Re-export CPM classes and functions
LogLevel = _cpm.LogLevel
Logging = _cpm.Logging
Reader = _cpm.Reader
AsyncReader = _cpm.AsyncReader
Writer = _cpm.Writer
ParameterReceiver = _cpm.ParameterReceiver
init = _cpm.init
get_time_ns = _cpm.get_time_ns

# Vehicle State class to match CPM's VehicleState
class VehicleState:
    """Represents the state of a vehicle in the CPM system"""
    
    def __init__(self):
        self.vehicle_id = 0
        self.time_stamp = 0
        self.pose = Pose()
        self.steering = 0.0
        self.velocity = 0.0
        
class Pose:
    """Represents a 2D pose (position and orientation)"""
    
    def __init__(self):
        self.x = 0.0
        self.y = 0.0
        self.yaw = 0.0

# Vehicle Command class to match CPM's VehicleCommand
class VehicleCommand:
    """Command message for a vehicle"""
    
    def __init__(self):
        self.vehicle_id = 0
        self.time_stamp = 0
        self.command = VehicleCommandDirect()

class VehicleCommandDirect:
    """Direct control command for a vehicle"""
    
    def __init__(self):
        self.vehicle_id = 0
        self.time_stamp = 0
        self.steering_target = 0.0
        self.velocity_target = 0.0

# HLC Ready message to match CPM's HLCReady
class HLCReady:
    """Message to indicate that a vehicle's high-level controller is ready"""
    
    def __init__(self):
        self.vehicle_id = 0
        self.ready = False
        self.time_stamp = 0