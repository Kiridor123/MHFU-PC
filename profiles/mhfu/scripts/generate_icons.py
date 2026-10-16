#!/usr/bin/env python3
"""Generate application icons from MHFU-Logo.png for:
- Windows executable icon (emblem.ico)
- Linux AppImage icon (mhfu_icon.png)
- Window & taskbar RGBA array (mhfu_window_icon.inc)
- Android launcher mipmap icons (ic_launcher, ic_launcher_round, ic_launcher_foreground, ic_launcher_monochrome)
"""

import os
import sys
from PIL import Image

def generate_icons(logo_path, project_root):
    logo = Image.open(logo_path).convert("RGBA")

    def make_square_icon(size, padding_ratio=0.08):
        canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        avail_w = int(size * (1.0 - padding_ratio * 2))
        avail_h = int(size * (1.0 - padding_ratio * 2))
        ratio = min(avail_w / logo.width, avail_h / logo.height)
        new_w = max(1, int(logo.width * ratio))
        new_h = max(1, int(logo.height * ratio))
        resized = logo.resize((new_w, new_h), Image.Resampling.LANCZOS)
        x = (size - new_w) // 2
        y = (size - new_h) // 2
        canvas.paste(resized, (x, y), resized)
        return canvas

    # 1. Linux AppImage icon (512x512)
    appimage_icon = os.path.join(project_root, "profiles", "mhfu", "packaging", "mhfu_icon.png")
    os.makedirs(os.path.dirname(appimage_icon), exist_ok=True)
    icon_512 = make_square_icon(512, padding_ratio=0.04)
    icon_512.save(appimage_icon, format="PNG")
    print(f"[+] Saved AppImage icon: {appimage_icon}")

    # 2. Window & taskbar icon header (mhfu_window_icon.inc: 64x64 RGBA)
    window_inc = os.path.join(project_root, "profiles", "mhp3rd", "host", "gpu", "mhfu_window_icon.inc")
    icon_64 = make_square_icon(64, padding_ratio=0.05)
    raw_bytes = icon_64.tobytes("raw", "RGBA")
    with open(window_inc, "w", encoding="utf-8") as f:
        f.write("// Generated 64x64 RGBA32 icon for MHFU window and taskbar from MHFU-Logo.png\n")
        f.write("inline constexpr int kMhfuWindowIconWidth = 64;\n")
        f.write("inline constexpr int kMhfuWindowIconHeight = 64;\n")
        f.write("alignas(4) inline constexpr unsigned char kMhfuWindowIconRgba[64 * 64 * 4] = {\n")
        for i in range(0, len(raw_bytes), 16):
            chunk = raw_bytes[i:i + 16]
            hex_str = ", ".join(f"0x{b:02X}u" for b in chunk)
            comma = "," if (i + 16 < len(raw_bytes)) else ""
            f.write(f"    {hex_str}{comma}\n")
        f.write("};\n")
    print(f"[+] Saved window icon header: {window_inc}")

    # 3. Windows executable icon (emblem.ico)
    ico_path = os.path.join(project_root, "profiles", "mhp3rd", "packaging", "windows", "emblem.ico")
    os.makedirs(os.path.dirname(ico_path), exist_ok=True)
    icon_256 = make_square_icon(256, padding_ratio=0.06)
    icon_256.save(ico_path, format="ICO", sizes=[(256, 256), (128, 128), (64, 64), (48, 48), (32, 32), (16, 16)])
    print(f"[+] Saved Windows emblem.ico: {ico_path}")

    # 4. Android launcher icons
    android_res = os.path.join(project_root, "profiles", "mhfu", "packaging", "android", "res")
    densities = {
        "mipmap-mdpi": (48, 108),
        "mipmap-hdpi": (72, 162),
        "mipmap-xhdpi": (96, 216),
        "mipmap-xxhdpi": (144, 324),
        "mipmap-xxxhdpi": (192, 432),
    }

    for folder, (std_sz, fore_sz) in densities.items():
        folder_path = os.path.join(android_res, folder)
        os.makedirs(folder_path, exist_ok=True)

        # Standard & Round icon
        std_icon = make_square_icon(std_sz, padding_ratio=0.08)
        std_icon.save(os.path.join(folder_path, "ic_launcher.png"), format="PNG")
        std_icon.save(os.path.join(folder_path, "ic_launcher_round.png"), format="PNG")

        # Foreground adaptive icon (content centered in ~66% safe zone of 108dp canvas)
        fore_icon = make_square_icon(fore_sz, padding_ratio=0.22)
        fore_icon.save(os.path.join(folder_path, "ic_launcher_foreground.png"), format="PNG")

        # Monochrome icon (for Android 13+ themed icons: white logo with alpha)
        mono_canvas = Image.new("RGBA", (fore_sz, fore_sz), (0, 0, 0, 0))
        r, g, b, a = fore_icon.split()
        white = Image.new("L", (fore_sz, fore_sz), 255)
        mono_canvas = Image.merge("RGBA", (white, white, white, a))
        mono_canvas.save(os.path.join(folder_path, "ic_launcher_monochrome.png"), format="PNG")

    print("[+] All Android launcher icons generated successfully!")

if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root = os.path.abspath(os.path.join(script_dir, "..", "..", ".."))
    logo = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "MHFU-Logo.png")
    if not os.path.isfile(logo):
        print(f"Error: {logo} not found")
        sys.exit(1)
    generate_icons(logo, root)
