import socket
import struct

def parse_itm_stream(buffer):
    while len(buffer) > 0:
        header = buffer[0]

        # 1. Skip Sync/Timestamp/Overflow (0x80, 0x00, 0x70, 0xFF, 0xC0-0xDF)
        if header in [0x80, 0x00, 0x70, 0xFF] or (0xC0 <= header <= 0xDF):
            buffer = buffer[1:]
            continue

        # 2. Port 1: Strings (The Header you are seeing: 0x08 or 0x09)
        # Note: 0x08 = Port 1, 1-byte size.
        if (header & 0x07) == 0x00 and (header >> 3) == 1:
            # Or simply: if header == 0x08 or header == 0x09:
            if len(buffer) < 2: break
            print(chr(buffer[1]), end='', flush=True)
            buffer = buffer[2:]

        # 3. Port 0: Manual 4-byte Cycles (Header 0x03)
        elif header == 0x03:
            if len(buffer) < 5: break
            val = struct.unpack("<I", buffer[1:5])[0]
            print(f"\n[CYCLES] {val}")
            buffer = buffer[5:]

        # 4. PC Sampling: Hardware Source ID 2 (Header 0x17)
        elif header == 0x17:
            if len(buffer) < 5: break
            pc = struct.unpack("<I", buffer[1:5])[0]
            func = mapper.get_function(pc)
            print(f"\n[PROFILER] {func} @ {hex(pc)}")
            buffer = buffer[5:]

        else:
            # If we don't know what it is, just skip 1 byte
            buffer = buffer[1:]
    return buffer

def parse_itm():
    ''' Parses ITM data from the pyOCD SWO raw port. It looks for two types of packets:
    1. 1-byte character packets (Header 0x09) - These are printed as they arrive.
    2. 4-byte integer packets (Header 0x03) - These are printed as cycle counts.
    The function also skips over sync or unknown bytes (like 0x80, 0x00, etc.).
    
    NOTE: To calculate header values, use a formula like this: 
            Header = (channel << 1) | type
          For example, for a 1-byte character packet on channel 0: 
            Header = (0 << 1) | 0x01 = 0x01
          For a 4-byte integer packet on channel 1: Header = (1 << 3) | 0x03 = 0x05   
    '''

    # Connect to the pyOCD SWO raw port
    sock = socket.create_connection(("localhost", 6666))
    print("Connected to SWO stream. Waiting for cycle counts...")

    buffer = b''
    while True:
        chunk = sock.recv(1024)
        for b in chunk:
            if b not in [0x00, 0x80, 0x03, 0x09]: # Ignore knowns
                print(f"NEW HEADER: 0x{b:02x}")
        if not chunk:
            break
        buffer += chunk

        while len(buffer) > 0:
            parse_itm_stream(buffer)

if __name__ == "__main__":
    try:
        parse_itm()
    except KeyboardInterrupt:
        print("\nStopped.")
