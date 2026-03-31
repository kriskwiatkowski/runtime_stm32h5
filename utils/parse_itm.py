import socket
import struct

def parse_itm():
    # Connect to the pyOCD SWO raw port
    sock = socket.create_connection(("localhost", 6666))
    print("Connected to SWO stream. Waiting for cycle counts...")

    buffer = b''
    while True:
        chunk = sock.recv(1024)
        if not chunk:
            break
        buffer += chunk

        while len(buffer) > 0:
            header = buffer[0]

            # Check for 1-byte character packet (Header 0x0B)
            if header == 0x09 and len(buffer) >= 2:
                char = chr(buffer[1])
                print(char, end='', flush=True) # Print chars as they arrive
                buffer = buffer[2:]

            # Check for 4-byte integer packet (Header 0x03)
            elif header == 0x03 and len(buffer) >= 5:
                val = struct.unpack("<I", buffer[1:5])[0]
                print(f"{val}\n", end='', flush=True) # Print cycle count
                buffer = buffer[5:]

            # Skip sync or unknown bytes (0x80, 0x00, etc.)
            else:
                buffer = buffer[1:]

if __name__ == "__main__":
    try:
        parse_itm()
    except KeyboardInterrupt:
        print("\nStopped.")
