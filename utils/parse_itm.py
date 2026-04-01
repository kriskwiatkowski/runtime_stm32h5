import socket
import struct
import time
import argparse
from symbol_wrapper import SymbolMapper

def parse_itm_stream(buffer, is_log_SWO=False, map=None):
    while len(buffer) > 0:
        header = buffer[0]

        # Skip sync/unknown bytes
        if header == 0x00 or header == 0x80:
            buffer = buffer[1:]
            continue

        # Instrumentation: 1-byte on channel 0 (ITM->PORT[0].u8)
        # Header = (channel << 3) | 0x01
        elif header == 0x01 and len(buffer) >= 2:
            print(chr(buffer[1]), end='', flush=True)
            buffer = buffer[2:]

        # Instrumentation: 4-byte on channel 0 (ITM->PORT[0].u32)
        # Header = (channel << 3) | 0x03
        elif header == 0x03 and len(buffer) >= 5:
            val = struct.unpack("<I", buffer[1:5])[0]
            print(f"{val}")
            buffer = buffer[5:]

        # Instrumentation: 1-byte on channel 1 (ITM->PORT[1].u32)
        # Header = (1 << 3) | 0x01 = 0x09
        elif header == 0x09 and len(buffer) >= 2:
            char = chr(buffer[1])
            print(char, end='', flush=True) # Print chars as they arrive
            buffer = buffer[2:]

        # Hardware: PC sample (DWT)
        # Header = 0x17
        elif header == 0x17 and is_log_SWO:
            if len(buffer) < 5: break
            pc = struct.unpack("<I", buffer[1:5])[0]
            if map:
                print(map.get_function(pc))
            else:
                print(f"[PC] 0x{pc:08X}")
            buffer = buffer[5:]

        else:
            buffer = buffer[1:]

    return buffer

def parse_itm(host, port, elf, swo):
    HOST, PORT = host, port  # pyOCD SWV raw port
    map = SymbolMapper(elf) if elf else None

    while True:
        try:
            print(f"Connecting to {HOST}:{PORT}...")
            sock = socket.create_connection((HOST, PORT))
            sock.settimeout(5.0)
            print("Connected.")

            buffer = b''
            while True:
                try:
                    chunk = sock.recv(4096)
                    if not chunk:
                        print("Connection closed by server.")
                        break
                    buffer += chunk
                    buffer = parse_itm_stream(buffer, swo, map)
                except socket.timeout:
                    # No data — target may be idle, just keep waiting
                    continue

        except (ConnectionRefusedError, ConnectionResetError, OSError) as e:
            print(f"Connection error: {e}. Retrying in 2s...")
            time.sleep(2)

if __name__ == "__main__":
    try:
        parser = argparse.ArgumentParser(description="ITM stream parser")
        parser.add_argument("--host", default="localhost", help="pyOCD SWV host")
        parser.add_argument("--port", type=int, default=3443, help="pyOCD SWV port")
        parser.add_argument("--elf", help="ELF file with debug symbols for symbol lookup")
        parser.add_argument("--swo", help="Log program counter samples from SWO", action="store_true")
        args = parser.parse_args()
        parse_itm(args.host, args.port, args.elf, args.swo)
    except KeyboardInterrupt:
        print("\nStopped.")
