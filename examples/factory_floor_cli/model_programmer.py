#!/usr/bin/env python3
"""
ESP32 Model Number Programming Station

This script automatically detects ESP32 devices connected via USB and programs
them with a specified model number. It monitors for new device connections and
handles multiple devices concurrently.

Devices are tracked by MAC address rather than USB port, allowing port reuse
in production environments. When the program exits, it displays a list of all
MAC addresses that were successfully programmed during the session.
"""

import argparse
import threading
import time
import serial
import serial.tools.list_ports
from typing import Set, Optional
import sys
import re


class DeviceTracker:
    """Tracks which devices have been processed to avoid reprogramming."""
    
    def __init__(self):
        self.processed_mac_addresses: Set[str] = set()
        self.checked_ports: dict = {}  # Maps port -> (mac_address, timestamp)
        self.lock = threading.Lock()
    
    def is_processed(self, mac_address: str) -> bool:
        """Check if a device has already been processed."""
        with self.lock:
            return mac_address in self.processed_mac_addresses
    
    def mark_processed(self, mac_address: str):
        """Mark a device as processed."""
        with self.lock:
            self.processed_mac_addresses.add(mac_address)
    
    def get_processed_macs(self) -> Set[str]:
        """Get the set of all programmed MAC addresses."""
        with self.lock:
            return self.processed_mac_addresses.copy()
    
    def get_port_mac(self, port: str) -> Optional[str]:
        """Get the cached MAC address for a port if recently checked."""
        with self.lock:
            if port in self.checked_ports:
                mac_address, timestamp = self.checked_ports[port]
                # Cache for 5 seconds to avoid repeated connections
                if time.time() - timestamp < 5.0:
                    return mac_address
            return None
    
    def cache_port_mac(self, port: str, mac_address: str):
        """Cache the MAC address for a port."""
        with self.lock:
            self.checked_ports[port] = (mac_address, time.time())
    
    def get_current_ports(self) -> Set[str]:
        """Get currently connected serial ports."""
        ports = serial.tools.list_ports.comports()
        return {port.device for port in ports}


class ESP32Programmer:
    """Handles communication with ESP32 device to set model number."""
    
    def __init__(self, port: str, model_number: str, baudrate: int = 115200, debug: bool = False):
        self.port = port
        self.model_number = model_number
        self.baudrate = baudrate
        self.debug = debug
    
    def wait_for_prompt(self, ser: serial.Serial, timeout: float = 5.0) -> bool:
        """Wait for the widget> prompt to appear by sending Enter and looking for prompt."""
        start_time = time.time()
        buffer = ""
        last_enter_time = 0
        bytes_received = 0
        
        if self.debug:
            print(f"[{self.port}] DEBUG: Starting prompt detection, timeout={timeout}s")
        
        while time.time() - start_time < timeout:
            # Send Enter periodically to force a new prompt line
            current_time = time.time()
            if current_time - last_enter_time > 0.5:
                if self.debug:
                    print(f"[{self.port}] DEBUG: Sending Enter keypress (elapsed: {current_time - start_time:.1f}s)")
                ser.write(b"\r\n")
                ser.flush()
                last_enter_time = current_time
            
            if ser.in_waiting > 0:
                try:
                    # Read until newline or timeout
                    chunk = ser.read_until().decode('utf-8', errors='ignore')
                    bytes_received += len(chunk)
                    
                    if self.debug:
                        print(f"[{self.port}] DEBUG: Received {len(chunk)} chars: {repr(chunk)}")
                    
                    # Respond to ANSI escape sequences for terminal capability detection
                    # The ESP32 linenoise sends ESC[6n to query cursor position
                    if '\x1b[6n' in chunk or '[6n' in chunk:
                        if self.debug:
                            print(f"[{self.port}] DEBUG: Responding to cursor position query")
                        # Respond with cursor at position 1,1
                        ser.write(b'\x1b[1;1R')
                        ser.flush()
                        # Remove the escape sequence from the chunk
                        chunk = chunk.replace('\x1b[6n', '').replace('[6n', '')
                    
                    buffer += chunk
                    
                    if self.debug:
                        print(f"[{self.port}] DEBUG: Buffer (last 100 chars): {repr(buffer[-100:])}")
                    
                    # Look for the prompt - check last few characters for efficiency
                    # The prompt is "widget>" so we look for this pattern
                    if "widget>" in buffer[-20:]:
                        if self.debug:
                            print(f"[{self.port}] DEBUG: Prompt detected!")
                        return True
                    
                    # Keep buffer manageable - only keep recent data
                    if len(buffer) > 2000:
                        buffer = buffer[-1000:]
                except Exception as e:
                    print(f"[{self.port}] Error reading: {e}")
                    import traceback
                    traceback.print_exc()
            else:
                time.sleep(0.01)
        
        if self.debug:
            print(f"[{self.port}] DEBUG: Timeout waiting for prompt")
            print(f"[{self.port}] DEBUG: Total bytes received: {bytes_received}")
            print(f"[{self.port}] DEBUG: Final buffer ({len(buffer)} chars): {repr(buffer)}")
        
        return False
    
    def send_command(self, ser: serial.Serial, command: str) -> Optional[str]:
        """Send a command and read the response until the next prompt."""
        try:
            # Clear any pending input first
            time.sleep(0.1)
            if ser.in_waiting > 0:
                discarded = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                if self.debug:
                    print(f"[{self.port}] DEBUG: Discarded pending input: {repr(discarded)}")
            
            if self.debug:
                print(f"[{self.port}] DEBUG: Sending command: {repr(command)}")
            
            # Send command with carriage return
            ser.write(f"{command}\r".encode('utf-8'))
            ser.flush()
            
            # Read response - collect multiple lines until we see the prompt
            response = ""
            start_time = time.time()
            lines_read = 0
            
            while time.time() - start_time < 5.0:
                if ser.in_waiting > 0:
                    try:
                        # Read one line at a time
                        line = ser.read_until().decode('utf-8', errors='ignore')
                        lines_read += 1
                        
                        # Respond to ANSI escape sequences
                        if '\x1b[6n' in line or '[6n' in line:
                            if self.debug:
                                print(f"[{self.port}] DEBUG: Responding to cursor position query in command response")
                            ser.write(b'\x1b[1;1R')
                            ser.flush()
                            line = line.replace('\x1b[6n', '').replace('[6n', '')
                        
                        response += line
                        
                        if self.debug:
                            print(f"[{self.port}] DEBUG: Response line {lines_read}: {repr(line)}")
                        
                        # Check if we've received a prompt (look for widget> pattern)
                        if "widget>" in line:
                            if self.debug:
                                print(f"[{self.port}] DEBUG: Prompt detected in response")
                            # Give a tiny bit of time for any remaining data
                            time.sleep(0.05)
                            if ser.in_waiting > 0:
                                final_chunk = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                                response += final_chunk
                                if self.debug:
                                    print(f"[{self.port}] DEBUG: Final chunk: {repr(final_chunk)}")
                            break
                    except Exception as e:
                        if self.debug:
                            print(f"[{self.port}] DEBUG: Exception reading line: {e}")
                        break
                else:
                    time.sleep(0.01)
            
            if self.debug:
                print(f"[{self.port}] DEBUG: Full response ({lines_read} lines): {repr(response)}")
            
            return response
        except Exception as e:
            print(f"[{self.port}] Error sending command: {e}")
            import traceback
            traceback.print_exc()
            return None
    
    def get_mac_address(self, ser: serial.Serial) -> Optional[str]:
        """Get the device MAC address by sending the 'id' command."""
        try:
            if self.debug:
                print(f"[{self.port}] DEBUG: Getting MAC address via 'id' command")
            
            # Send id command
            id_response = self.send_command(ser, "id")
            if id_response is None:
                print(f"[{self.port}] ERROR: Failed to send id command")
                return None
            
            if self.debug:
                print(f"[{self.port}] DEBUG: ID response: {repr(id_response)}")
            
            # Parse MAC address from response
            # Expected format: "MAC: XX:XX:XX:XX:XX:XX" or similar
            mac_match = re.search(r'MAC:\s*([0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2})', id_response)
            if mac_match:
                mac_address = mac_match.group(1).upper()
                if self.debug:
                    print(f"[{self.port}] DEBUG: Parsed MAC address: {mac_address}")
                return mac_address
            else:
                print(f"[{self.port}] ERROR: Could not parse MAC address from 'id' response")
                print(f"[{self.port}] Response: {repr(id_response)}")
                return None
        except Exception as e:
            print(f"[{self.port}] Error getting MAC address: {e}")
            import traceback
            traceback.print_exc()
            return None
    
    def program_device(self, tracker: DeviceTracker) -> tuple[bool, Optional[str]]:
        """Program the device with the model number and verify.
        
        Args:
            tracker: DeviceTracker to check if MAC was already processed
        
        Returns:
            A tuple of (success: bool, mac_address: Optional[str])
        """
        if self.debug:
            print(f"[{self.port}] Attempting to program device...")
        
        try:
            if self.debug:
                print(f"[{self.port}] DEBUG: Opening serial port at {self.baudrate} baud")
            
            # Open serial connection
            ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=1,
            )
            
            if self.debug:
                print(f"[{self.port}] DEBUG: Serial port opened successfully")
                print(f"[{self.port}] DEBUG: Port info - baudrate={ser.baudrate}, timeout={ser.timeout}")
            
            # Give device time to initialize and clear buffers
            time.sleep(0.5)
            
            if self.debug:
                print(f"[{self.port}] DEBUG: Clearing input/output buffers")
            
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            
            if self.debug:
                print(f"[{self.port}] Waiting for console prompt...")
            
            # Wait for prompt (this will send Enter keypresses to force new prompt lines)
            if not self.wait_for_prompt(ser, timeout=8.0):
                if self.debug:
                    print(f"[{self.port}] ERROR: No prompt detected. Device may not be an ESP32 console.")
                    print(f"[{self.port}] Make sure the device is running the console application.")
                ser.close()
                return (False, None)
            
            # Get MAC address first
            if self.debug:
                print(f"[{self.port}] Getting device MAC address...")
            mac_address = self.get_mac_address(ser)
            if mac_address is None:
                if self.debug:
                    print(f"[{self.port}] ERROR: Failed to retrieve MAC address")
                ser.close()
                return (False, None)
            
            if self.debug:
                print(f"[{self.port}] Device MAC: {mac_address}")
            
            # Check if already processed
            if tracker.is_processed(mac_address):
                if self.debug:
                    print(f"[{self.port}] Device already programmed in this session (MAC: {mac_address})")
                ser.close()
                return (False, mac_address)  # Return False but with MAC so we know it was already done
            
            # Device needs programming - show operator-friendly output
            print(f"[{self.port}] Programming device {mac_address}...")
            
            # Send model set command
            set_response = self.send_command(ser, f"model set {self.model_number}")
            if set_response is None:
                print(f"[{self.port}] ERROR: Failed to send model set command")
                ser.close()
                return (False, mac_address)
            
            # Check if set was successful
            if f"Model Number set to: {self.model_number}" in set_response:
                if self.debug:
                    print(f"[{self.port}] Model number set successfully")
            else:
                print(f"[{self.port}] WARNING: Unexpected response to model set")
                if self.debug:
                    print(f"[{self.port}] Response: {repr(set_response)}")
            
            # Verify with model get command
            if self.debug:
                print(f"[{self.port}] Verifying with 'model get' command...")
            get_response = self.send_command(ser, "model get")
            if get_response is None:
                print(f"[{self.port}] ERROR: Failed to send model get command")
                ser.close()
                return (False, mac_address)
            
            # Verify the model number
            if f"Model Number: {self.model_number}" in get_response:
                print(f"[{self.port}] ✓ SUCCESS: {mac_address} programmed with '{self.model_number}'")
                ser.close()
                return (True, mac_address)
            else:
                print(f"[{self.port}] ERROR: Verification failed")
                print(f"[{self.port}] Expected: 'Model Number: {self.model_number}'")
                if self.debug:
                    print(f"[{self.port}] Response: {repr(get_response)}")
                ser.close()
                return (False, mac_address)
                
        except serial.SerialException as e:
            if self.debug:
                print(f"[{self.port}] Serial error: {e}")
            return (False, None)
        except Exception as e:
            if self.debug:
                print(f"[{self.port}] Unexpected error: {e}")
            return (False, None)


def program_device_thread(port: str, model_number: str, tracker: DeviceTracker, processed_ports: Set[str], debug: bool = False):
    """Thread function to program a single device.
    
    Args:
        port: The serial port to program
        model_number: The model number to set
        tracker: DeviceTracker instance for tracking MACs
        processed_ports: Set of ports being processed (to avoid duplicate threads)
        debug: Enable debug output
    """
    try:
        programmer = ESP32Programmer(port, model_number, debug=debug)
        success, mac_address = programmer.program_device(tracker)
        
        # Cache the MAC for this port to avoid repeated connections
        if mac_address:
            tracker.cache_port_mac(port, mac_address)
        
        if success and mac_address:
            tracker.mark_processed(mac_address)
            # Success message already printed in program_device
        elif mac_address and tracker.is_processed(mac_address):
            # Device was already programmed - only show in debug mode
            if debug:
                print(f"[{port}] DEBUG: Skipping already programmed device (MAC: {mac_address})\n")
        elif mac_address:
            # Failed but we got MAC - mark as processed to avoid retry
            tracker.mark_processed(mac_address)
            print(f"[{port}] FAILED: Programming failed for {mac_address}\n")
        else:
            # No MAC address obtained - only show in debug mode
            if debug:
                print(f"[{port}] DEBUG: Could not retrieve MAC address, device can be retried\n")
    finally:
        # Always remove from processing set when done
        with tracker.lock:
            processed_ports.discard(port)


def monitor_usb_devices(model_number: str, poll_interval: float = 1.0, debug: bool = False):
    """Monitor for new USB devices and program them automatically."""
    tracker = DeviceTracker()
    active_threads = []
    processed_ports = set()  # Track ports currently being processed
    
    print(f"Monitoring for ESP32 devices to program with model number: '{model_number}'")
    if debug:
        print("DEBUG MODE ENABLED - Verbose output will be shown")
    print("Press Ctrl+C to stop...\n")
    
    try:
        while True:
            # Get current ports
            current_ports = tracker.get_current_ports()
            
            if debug and current_ports:
                print(f"DEBUG: Current ports detected: {current_ports}")
            
            # Check for new ports
            for port in current_ports:
                # Skip if already processing this port
                if port in processed_ports:
                    continue
                
                # Check if we recently checked this port and found a programmed device
                cached_mac = tracker.get_port_mac(port)
                if cached_mac and tracker.is_processed(cached_mac):
                    if debug:
                        print(f"DEBUG: Skipping {port} - recently checked, already programmed (MAC: {cached_mac})")
                    continue
                
                # Mark port as being processed
                with tracker.lock:
                    processed_ports.add(port)
                
                if debug:
                    print(f"[{port}] New device detected!")
                
                # Start programming thread
                thread = threading.Thread(
                    target=program_device_thread,
                    args=(port, model_number, tracker, processed_ports, debug),
                    name=port
                )
                thread.daemon = True
                thread.start()
                active_threads.append(thread)
            
            # Clean up finished threads
            active_threads = [t for t in active_threads if t.is_alive()]
            
            # Sleep before next poll
            time.sleep(poll_interval)
            
    except KeyboardInterrupt:
        print("\n\nShutting down...")
        print("Waiting for active programming operations to complete...")
        
        # Wait for all threads to complete
        for thread in active_threads:
            if thread.is_alive():
                thread.join(timeout=5.0)
        
        # Print summary of programmed devices
        programmed_macs = tracker.get_processed_macs()
        if programmed_macs:
            print("\n" + "="*60)
            print(f"Successfully programmed {len(programmed_macs)} device(s):")
            print("="*60)
            for mac in sorted(programmed_macs):
                print(f"  {mac}")
            print("="*60)
        else:
            print("\nNo devices were successfully programmed.")
        
        print("\nShutdown complete.")


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="ESP32 Model Number Programming Station",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
This program monitors for ESP32 devices connected via USB and automatically
programs them with the specified model number. It can handle multiple devices
connecting simultaneously.

Devices are tracked by MAC address (not USB port), so the same port can be
reused for different devices. The program will not reprogram a device that
has already been programmed in the current session.

The program expects devices to present a console with a 'widget>' prompt and
support the following commands:
  - id (returns device MAC address)
  - model set <model_number>
  - model get

On exit, the program displays a list of all MAC addresses that were
successfully programmed.

Example:
  %(prog)s ABC-123
        """
    )
    
    parser.add_argument(
        'model_number',
        help='The model number to program into connected devices'
    )
    
    parser.add_argument(
        '-b', '--baudrate',
        type=int,
        default=115200,
        help='Serial baudrate (default: 115200)'
    )
    
    parser.add_argument(
        '-i', '--interval',
        type=float,
        default=1.0,
        help='USB polling interval in seconds (default: 1.0)'
    )
    
    parser.add_argument(
        '-d', '--debug',
        action='store_true',
        help='Enable debug output to see all serial communication'
    )
    
    args = parser.parse_args()
    
    # Validate model number (simple validation)
    if not args.model_number or len(args.model_number) > 63:
        print("ERROR: Model number must be between 1 and 63 characters")
        sys.exit(1)
    
    # Start monitoring
    monitor_usb_devices(args.model_number, args.interval, args.debug)


if __name__ == "__main__":
    main()
