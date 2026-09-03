"""Independent SNES 2bpp HUD decode used only by evidence verifiers."""
import numpy as np
from PIL import Image, ImageDraw


def decode(vram, cgram):
    v = np.frombuffer(vram, dtype=np.uint8)
    entries = np.frombuffer(vram[0x800:0x1000], dtype="<u2").reshape(32, 32)
    y, x = np.indices((224, 256))
    e = entries[y // 8, x // 8]
    tx = np.where(e & 0x4000, 7 - (x & 7), x & 7)
    ty = np.where(e & 0x8000, 7 - (y & 7), y & 7)
    address = 0x2000 + (e & 1023) * 16 + ty * 2
    color = ((v[address] >> (7 - tx)) & 1) | (((v[address + 1] >> (7 - tx)) & 1) << 1)
    palette = np.frombuffer(cgram, dtype="<u2")
    c = palette[((e >> 10) & 7) * 4 + color]
    rgb = np.stack([(((c >> shift) & 31) << 3) | (((c >> shift) & 31) >> 2)
                    for shift in (0, 5, 10)], axis=-1).astype(np.uint8)
    return rgb, color != 0


def contacts(directory, records):
    """Show every captured HUD frame at native size; retain complete PNGs."""
    pages = []
    for start in range(0, len(records), 48):
        group = records[start:start + 48]
        sheet = Image.new("RGB", (1040, ((len(group) + 3) // 4) * 92), (24, 25, 30))
        draw = ImageDraw.Draw(sheet)
        for i, row in enumerate(group):
            x, y = (i % 4) * 260, (i // 4) * 92
            draw.text((x + 2, y + 2), f"frame {row['frame']}  timer {row['timer']}", fill="white")
            with Image.open(directory / row["file"]) as im:
                sheet.paste(im.crop((0, 48, 256, 112)), (x, y + 21))
        path = directory / f"contact_{start // 48 + 1:02d}.png"
        sheet.save(path)
        pages.append(path.name)
    return pages
