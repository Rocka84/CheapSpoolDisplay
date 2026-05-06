import struct

def create_opentag_mock(filename):
    # Core Memory Map (0x00 to 0x6F)
    payload = bytearray(200)

    # Offset 0x00: Tag Version (int, 2 bytes, 1.000)
    struct.pack_into(">H", payload, 0x00, 1000)

    # Offset 0x02: Base Material Name (utf8, 5 bytes)
    payload[0x02:0x07] = b"PLA  "

    # Offset 0x1B: Filament Manufacturer (utf8, 16 bytes)
    payload[0x1B:0x1B+16] = b"Rocka84 3D      "

    # Offset 0x2B: Color Name (utf8, 32 bytes)
    payload[0x2B:0x2B+32] = b"Sunset Orange                   "

    # Offset 0x4B: Color 1 Hex (rgba, 4 bytes)
    # RGBA: 255, 100, 0, 255 (FF, 64, 00, FF)
    payload[0x4B:0x4F] = bytes([0xFF, 0x64, 0x00, 0xFF])

    # Offset 0x5C: Target Diameter (int, 2 bytes, 1.750mm -> 1750um)
    struct.pack_into(">H", payload, 0x5C, 1750)

    # Offset 0x5E: Target Weight (int, 2 bytes, 1000g)
    struct.pack_into(">H", payload, 0x5E, 1000)

    # Offset 0x60: Print Temperature (int, 1 byte, 215C -> 215/5 = 43)
    payload[0x60] = 43

    # Offset 0x61: Bed Temperature (int, 1 byte, 60C -> 60/5 = 12)
    payload[0x61] = 12

    # Offset 0x70: Online Data URL (ascii, 32 bytes)
    # URL: spoolman.local/spool/show/123
    payload[0x70:0x70+32] = b"spoolman.local/spool/show/123   "

    with open(filename, "wb") as f:
        f.write(payload)
    print(f"Created {filename}")

if __name__ == "__main__":
    create_opentag_mock("/home/dilli/src/CheapSpoolDisplay/simulator/spool_mocks/opentag1.bin")
