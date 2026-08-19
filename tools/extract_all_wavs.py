import os, sys, struct, wave

rom_path = r"F:\Games\SNES\NBA Live 95 (USA).sfc"
if len(sys.argv) > 1:
    rom_path = sys.argv[1]

if not os.path.exists(rom_path):
    print(f"Error: ROM not found at {rom_path}")
    sys.exit(1)

with open(rom_path, "rb") as f:
    rom = f.read()

out_dir = r"c:\Users\joshs\Projects\nba-live-95-c-port\extracted_wavs"
os.makedirs(out_dir, exist_ok=True)

def decode_brr_fast(data):
    pcm = []
    p1 = 0
    p2 = 0
    pos = 0
    data_len = len(data)
    
    while pos + 9 <= data_len:
        h = data[pos]
        shift = h >> 4
        f = (h >> 2) & 3
        end = (h & 1) != 0
        if shift > 12:
            break
        
        pos += 1
        block_bytes = data[pos:pos+8]
        for byte_val in block_bytes:
            n1 = (byte_val >> 4) & 0xF
            n2 = byte_val & 0xF
            for nibble in (n1, n2):
                sample = nibble if nibble < 8 else nibble - 16
                sample = (sample << shift) >> 1
                if f == 0:
                    out = sample
                elif f == 1:
                    out = sample + p1 + ((-p1) >> 4)
                elif f == 2:
                    out = sample + (p1 << 1) + ((-((p1 << 1) + p1)) >> 5) - p2 + (p2 >> 4)
                else:
                    out = sample + (p1 << 1) + ((-(p1 + (p1 << 2) + (p1 << 3))) >> 6) - p2 + (((p2 << 1) + p2) >> 4)
                
                out = max(-32768, min(32767, int(out)))
                pcm.append(out)
                p2 = p1
                p1 = out
        pos += 8
        if end:
            return pcm, pos
            
    return pcm, pos

# Quickly scan block aligned streams
# BRR samples always start on block boundaries
print("Scanning ROM for BRR audio streams...")
pos = 0
samples_found = []

# Known audio banks in this ROM: Bank 4, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20
# Let's scan every 2 bytes or 9 bytes
while pos < len(rom) - 18:
    h = rom[pos]
    # Check if plausible BRR block header: range <= 12, end == 0
    if (h >> 4) <= 12 and (h & 1) == 0 and ((h >> 2) & 3) <= 3:
        # Check if 9-byte sequence is a valid BRR chain
        pcm, consumed = decode_brr_fast(rom[pos:])
        if len(pcm) >= 480 and consumed >= 270:
            bank = (pos // 0x8000) + 0x80
            addr = 0x8000 + (pos % 0x8000)
            samples_found.append((pos, bank, addr, consumed, pcm))
            pos += consumed
            continue
    pos += 1

print(f"Decoded {len(samples_found)} audio samples from ROM.")

manifest = []
for idx, (offset, bank, addr, consumed, pcm) in enumerate(samples_found, 1):
    duration_16k = len(pcm) / 16000.0
    wav_filename = f"sample_{idx:02d}_rom_0x{offset:06X}_bank{bank:02X}_{duration_16k:.2f}s.wav"
    wav_path = os.path.join(out_dir, wav_filename)
    
    with wave.open(wav_path, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(16000)
        wf.writeframes(struct.pack(f"<{len(pcm)}h", *pcm))
    
    manifest.append((idx, wav_filename, offset, bank, addr, consumed, len(pcm), duration_16k))

print(f"\nSuccessfully extracted {len(manifest)} WAV files into: {out_dir}\n")
for item in manifest:
    idx, name, offset, bank, addr, consumed, frames, dur = item
    print(f"  Sample #{idx:02d}: {name} (ROM 0x{offset:06X}, {consumed} bytes BRR, {dur:.2f}s)")
