import serial
import time
for port in ['/dev/ttyUSB0', '/dev/ttyACM0']:
    try:
        with serial.Serial(port, 115200, timeout=2) as ser:
            ser.write(b'\nget config\n')
            time.sleep(1)
            response = ser.read_all().decode('utf-8', errors='ignore')
            if 'config' in response.lower() or 'webhook' in response.lower():
                print(f"--- Config from {port} ---")
                print(response)
    except Exception as e:
        print(f"Failed on {port}: {e}")
