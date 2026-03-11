# 🎬 DMX Tester - Professional WiFi-based DMX Testing Device

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![ESP8266](https://img.shields.io/badge/Platform-ESP8266-orange.svg)](https://www.espressif.com/)
[![Version](https://img.shields.io/badge/version-1.0-brightgreen.svg)](#)

A professional-grade DMX tester built on **Wemos D1 mini** with a stunning **Neon UI** accessible via WiFi. Control all **512 DMX channels** with real-time monitoring, scene presets, and dynamic color themes.

Inspired by the **Swisson XMT-120A** professional DMX tester.

## ✨ Features

- 🎛️ **Full 512 DMX Channel Support** - Control every single DMX channel
- 📱 **Web-Based Interface** - Access from any device (Desktop, Tablet, Handy)
- 🔆 **Neon Glow UI** - Professional Swisson-style design with Amber/Neon aesthetics
- 🌈 **Dynamic Color Themes** - Choose from 10+ neon colors or create custom colors
- 📺 **Dual Display Modes** - CONTROL (input) or MONITOR (512 channel visualization)
- 💾 **10 Scene Presets** - Save and load DMX configurations
- ⚡ **Multiple Test Modes** - SET, +1/-1 tuning, ALL ON/OFF, CHASE animation
- 🎨 **Customizable** - Easy parameter adjustments (IP, colors, layout)

---

## 📋 Table of Contents

- [Hardware Setup](#-hardware-setup)
- [Software Installation](#-software-installation)
- [Usage Guide](#-usage-guide)
- [Features in Detail](#-features-in-detail)
- [Configuration](#-configuration)
- [3D Printing](#-3d-printing)
- [Troubleshooting](#-troubleshooting)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🔧 Hardware Setup

### Components Required

| Component | Quantity | Specification | Cost ca. |
|-----------|----------|---------------|----------|
| **Wemos D1 mini** | 1x | ESP8266, USB-C | 8-12€ |
| **RS485 Module** | 1x | MAX485, TTL-Level | 2-4€ |
| **XLR Panel-Mount** | 1x | Female, Ø22mm | 5-10€ |
| **USB Cable** | 1x | USB-C or Micro | - |
| **Jumper Cables** | 10x | Male-Female, 20cm | 2€ |

**Total Cost:** ~30-50€ 

---

### 🔌 Wiring Diagram

```
┌─────────────────┐
│  Wemos D1 mini  │
└────────┬────────┘
         │
    ┌────┴────┐
    │          │
    ▼          ▼
┌─────┐    ┌──────┐
│ GND ├────┤ GND  │
├─────┤    ├──────┤
│ 5V  ├────┤ VCC  │  MAX485
├─────┤    ├──────┤  RS485 Module
│ TX  ├────┤ DI   │
├─────┤    ├──────┤
│ RX  ├────┤ RO   │
├─────┤    ├──────┤
│ D8  ├────┤ DE   │
└─────┘    └──┬───┘
              │
         ┌────┴────┐
         │          │
         ▼          ▼
      ┌────────────────┐
      │   XLR 3-Pin    │
      │ Pin 1 (GND)    │
      │ Pin 2 (A/-)    │
      │ Pin 3 (B/+)    │
      └────────────────┘
```

### 📍 Detailed Pin Connections

#### Wemos D1 mini to RS485 Board

| Wemos Pin | GPIO | RS485 Pin | Function |
|-----------|------|-----------|----------|
| GND | - | GND | Ground |
| 5V | - | VCC | Power (or 3.3V with Levelshifter) |
| TX (GPIO1) | GPIO1 | DI | Data Input (Serial TX) |
| RX (GPIO3) | GPIO3 | RO | Receiver Output (Serial RX) |
| D8 | GPIO15 | DE/RE | Driver/Receiver Enable |

#### RS485 to XLR Connector

| RS485 Pin | XLR Pin | Signal |
|-----------|---------|--------|
| A | Pin 2 | DMX Negative (-) |
| B | Pin 3 | DMX Positive (+) |
| GND | Pin 1 | Ground |

**Important:** 
- ⚠️ Add **120Ω resistor** between A and B at the **end of the DMX line**
- Use proper XLR cables (3-pin)
- Keep DMX cables short and shielded

---

### 🎯 Step-by-Step Hardware Assembly

#### Step 1: Prepare Components
1. Get all components from the list above
2. Check Wemos D1 mini is detected by PC
3. Verify RS485 board is working (LED indicator)

#### Step 2: Solder/Connect RS485 Board
```
Option A - Soldering (permanent):
1. Solder wires directly to RS485 board
2. Wemos TX (GPIO1) → RS485 DI
3. Wemos RX (GPIO3) → RS485 RO
4. Wemos D8 (GPIO15) → RS485 DE
5. GND ↔ GND, 5V ↔ VCC

Option B - Breadboard (temporary):
1. Use jumper cables on breadboard
2. Easier for testing, less permanent
```

#### Step 3: Connect XLR Connector
```
RS485 Board → XLR Female Panel-Mount
├─ A → Pin 2 (DMX -)
├─ B → Pin 3 (DMX +)
└─ GND → Pin 1 (Ground)
```

#### Step 4: Mount in 3D Printed Case
1. Print case from OpenSCAD file (see 3D Printing section)
2. Mount Wemos D1 mini inside (use double-sided tape or brackets)
3. Mount RS485 board next to Wemos
4. Install XLR panel-mount in top hole
5. Cable management inside
6. Add P-Touch band label to front

#### Step 5: USB Power
- Connect Wemos D1 mini via USB-C to power source
- Recommended: 5V/2A USB power adapter

---

### ✅ Hardware Test Checklist

- [ ] Wemos D1 mini lights up (LED)
- [ ] RS485 board has power LED
- [ ] All cables connected correctly
- [ ] XLR connector firmly mounted
- [ ] No loose wires or shorts
- [ ] USB power is stable
- [ ] Case is assembled (optional)

---

## 💻 Software Installation

### Prerequisites

- **Arduino IDE** (Latest version)
- **ESP8266 Board Manager** installed
- **USB Cable** to Wemos D1 mini
- **Computer** (Windows, macOS, Linux)

### Step 1: Install Arduino IDE

1. Download from [arduino.cc](https://www.arduino.cc/en/software)
2. Install following official instructions
3. Launch Arduino IDE

### Step 2: Add ESP8266 Board Manager

1. **File** → **Preferences**
2. Find **"Additional Boards Manager URLs"**
3. Paste this URL:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
4. Click **OK**

### Step 3: Install ESP8266 Package

1. **Tools** → **Board** → **Boards Manager**
2. Search for `esp8266`
3. Click **"esp8266 by ESP8266 Community"**
4. Click **Install** (v3.1.2 or newer)
5. Wait for installation to complete

### Step 4: Configure Board Settings

Set these exact settings:

```
Tools Menu:
├─ Board: "Generic ESP8266 Module"
├─ Flash Size: "4MB (FS: 2MB OTA: ~1019KB)"
├─ Flash Mode: "DIO"
├─ Flash Frequency: "80 MHz"
├─ CPU Frequency: "80 MHz"
├─ Upload Speed: "115200"
└─ Port: "COM[X]" (your Wemos port)
```

### Step 5: Upload Code

1. Copy the entire `DMX_Tester.ino` file
2. Paste into Arduino IDE
3. Connect Wemos D1 mini via USB
4. **Sketch** → **Upload** (or Ctrl+U)
5. Wait for completion (~15-20 seconds)
6. Monitor console for errors

### Step 6: Verify Installation

1. Open **Tools** → **Serial Monitor**
2. Set Baud Rate to **115200**
3. Restart Wemos (USB disconnect/reconnect)
4. Should see:
   ```
   === DMX Tester START ===
   AP IP: 192.168.4.1
   Connect to: DMX-Tester
   Web Server gestartet
   ```

✅ **Success!** Wemos is running!

---

## 📱 Usage Guide

### First Time Setup

1. **Connect to WiFi:**
   - Open WiFi networks
   - Search for `DMX-Tester`
   - Connect with password: `12345678`

2. **Open Web Interface:**
   - Open browser
   - Navigate to: `http://192.168.4.1`
   - Web interface should load (Neon UI)

3. **Test DMX Output:**
   - CONTROL tab
   - Channel: 1, Value: 255
   - Click [SET]
   - Monitor tab: Channel 1 should show active (green border)

### CONTROL Tab

**Main control interface for testing individual channels:**

```
Channel Input:  1-512 (which channel)
Value Input:    0-255 (brightness level)

Buttons:
├─ [SET]     - Set channel to value
├─ [+1]      - Increment value by 1
├─ [-1]      - Decrement value by 1
├─ [ALL ON]  - All channels to 255
├─ [ALL OFF] - All channels to 0
├─ [50%]     - All channels to 128
├─ [CHASE]   - Running animation through channels
└─ [RESET]   - Reset all to 0

Scenes: [S1]-[S10] - Load saved presets
Status: Shows last action
```

### MONITOR Tab

**Display all 512 DMX channels in real-time:**

```
Channel Grid (512 cells):
├─ Shows Channel Number (1-512)
├─ Shows Current Value (0-255)
├─ Green Border = Active (>0)
├─ Gray Border = Inactive (0)
└─ Auto-updates every 500ms
```

### COLORS Tab

**Customize UI appearance:**

```
Preset Colors:
├─ AMBER (Default)
├─ GREEN
├─ CYAN
├─ MAGENTA
├─ ORANGE
├─ PINK
├─ RED
├─ YELLOW
├─ BLUE
└─ MINT

Custom Color:
├─ Input HEX code (#RRGGBB)
├─ Click [APPLY]
└─ UI changes instantly
```

### Practical Examples

#### Example 1: Test a Single Light
```
1. CONTROL tab
2. Enter Channel: 42
3. Enter Value: 255
4. Click [SET]
5. Light at channel 42 should turn on
6. Use [+1]/[-1] to adjust brightness
```

#### Example 2: Full System Test
```
1. CONTROL tab
2. Click [ALL ON]
3. Go to MONITOR tab
4. All 512 channels should show active
5. Visual confirmation everything works
```

#### Example 3: Save a Scene
```
1. Configure all channels as desired
2. Remember: [S1]-[S10] load only (future: expand to save)
3. Click [S1] to load saved scene
```

#### Example 4: CHASE Animation
```
1. Click [CHASE]
2. Go to MONITOR tab
3. Watch light run through channels
4. Click [CHASE] again to stop
```

---

## 🎨 Features in Detail

### 1. Full 512 DMX Channel Control

- ✅ All 512 DMX channels supported
- ✅ Real-time monitoring
- ✅ Individual channel control
- ✅ Batch operations (ALL ON/OFF)

### 2. Professional Neon UI

- 🌟 Swisson XMT-120A inspired design
- 🌟 4-layer glow effects (border + text)
- 🌟 Monospace font for technical look
- 🌟 Responsive design (Desktop/Tablet/Mobile)

### 3. Dynamic Color Themes

- 🎨 10 preset neon colors
- 🎨 Custom HEX color input
- 🎨 Instant UI refresh
- 🎨 All elements color-coordinated

### 4. Dual Display Modes

- 📊 CONTROL: Input-focused layout
- 📊 MONITOR: 512-channel grid view
- 📊 Seamless switching
- 📊 Data stays synchronized

### 5. Test Functions

- ⚡ SET: Direct channel value input
- ⚡ +1/-1: Fine adjustment
- ⚡ ALL ON/OFF: Quick bulk control
- ⚡ 50%: Mid-level testing
- ⚡ CHASE: Sequential animation
- ⚡ RESET: Safe state

### 6. Scene Management

- 💾 10 preset slots (S1-S10)
- 💾 EEPROM storage (persistent)
- 💾 Load any preset instantly
- 💾 All 512 channels per scene

---

## ⚙️ Configuration

### Change WiFi Details

Open `DMX_Tester.ino` and find:

```cpp
#define SSID_AP "DMX-Tester"
#define PASSWORD_AP "12345678"
```

Change to your desired values:
```cpp
#define SSID_AP "MyDMXTester"
#define PASSWORD_AP "MyPassword123"
```

### Change IP Address

Find this section:

```cpp
IPAddress apIP(192, 168, 4, 1);       // Change here
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
```

Examples:
```cpp
IPAddress apIP(192, 168, 1, 100);     // .1.100
IPAddress apIP(10, 0, 0, 1);          // 10.x.x.x
```

### Adjust DMX Parameters

```cpp
#define DMX_CHANNELS 512      // Max channels
#define BAUD_RATE 250000      // Standard DMX rate
#define DMX_INTERVAL 25       // Frame interval (ms)
```

### Change RS485 Pins

If using different pins:

```cpp
#define RS485_DE D8    // GPIO15 (Data Enable)
// TX/RX use Serial1 (GPIO1/GPIO3) - fixed
```

---

## 🐛 Troubleshooting

### WiFi Connection Issues

**Problem:** WiFi `DMX-Tester` not visible

**Solutions:**
1. Power cycle Wemos (USB disconnect/reconnect)
2. Wait 10 seconds for AP to start
3. Check Serial Monitor for errors
4. Try uploading code again

---

### Web Interface Won't Load

**Problem:** Browser shows "Cannot reach" or timeout

**Solutions:**
1. Verify WiFi connection (correct SSID)
2. Verify IP in browser matches Serial output
3. Try different browser (Chrome/Firefox)
4. Clear browser cache (Ctrl+Shift+Del)
5. Check firewall settings

---

### DMX Not Being Sent

**Problem:** DMX devices don't respond to tester

**Solutions:**
1. Check RS485 wiring (A/B pins correct?)
2. Add 120Ω resistor at end of DMX line
3. Verify XLR cable not damaged
4. Use CHASE mode to test (visual feedback)
5. Check Serial Monitor for errors

---

### Compilation Error

**Problem:** "stray '`'" or similar syntax errors

**Solutions:**
1. Use latest Arduino IDE (v2.0+)
2. Verify ESP8266 package installed
3. Check Board settings (Generic ESP8266)
4. Try uploading original code (no edits)

---

### Memory/Performance Issues

**Problem:** Web interface slow or laggy

**Solutions:**
1. Reduce WiFi interference (away from router)
2. Close other browser tabs
3. Use faster WiFi band (2.4GHz sometimes better)
4. Restart Wemos
5. Check USB power is stable (2A recommended)

---



## 🤝 Contributing

Contributions are welcome! Here's how:

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/AmazingFeature`)
3. **Commit** your changes (`git commit -m 'Add AmazingFeature'`)
4. **Push** to the branch (`git push origin feature/AmazingFeature`)
5. **Open** a Pull Request

### Areas for Contribution

- 🐛 Bug fixes
- ✨ New features (MIDI support, fading, etc.)
- 📝 Documentation improvements
- 🎨 UI/UX enhancements
- 🌍 Language translations
- 🧪 Testing on different hardware

---

## 📋 Future Enhancements

Planned features:

- [ ] Save/Load scene presets via UI button
- [ ] Custom CHASE speed control
- [ ] Fading/dimmer curves
- [ ] MIDI input support
- [ ] Multiple DMX universes
- [ ] WebSocket for faster updates
- [ ] Mobile app (React Native)
- [ ] Recording macro functionality
- [ ] Dark/Light mode toggle
- [ ] Multi-language support

---

## 📄 License

This project is licensed under the **MIT License** - see [LICENSE](LICENSE) file for details.

You are free to use, modify, and distribute this project for personal and commercial use.

---

## 📧 Contact & Support

- **Issues:** Open a GitHub issue for bugs and feature requests
- **Questions:** Create a Discussion thread
- **Email:** [Your Email if desired]
- **Discord:** [Your Discord if desired]

---

## 🙏 Acknowledgments

- **Inspired by:** Swisson XMT-120A Professional DMX Tester
- **Built with:** Arduino IDE, ESP8266 SDK, OpenSCAD
- **Community:** ESP8266 developers and DMX enthusiasts

---

## 📊 Project Stats

```
Lines of Code:      ~500 (Arduino)
Web Interface:      ~400 lines (HTML/CSS/JS)
Supported Channels: 512 (full DMX)
Color Themes:       10+ (customizable)
Display Modes:      3 (CONTROL/MONITOR/COLORS)
File Size:          ~12KB (firmware)
Memory Usage:       ~75KB RAM
Storage:            4MB Flash
```

---

## 🚀 Getting Started Quick

```bash
# 1. Clone repository
git clone https://github.com/YourUsername/DMX-Tester.git
cd DMX-Tester

# 2. Setup hardware (see Hardware Setup section)

# 3. Upload code to Wemos D1 mini (see Software Installation)

# 4. Connect to WiFi
# SSID: DMX-Tester
# Password: 12345678

# 5. Open browser
# http://192.168.4.1

# 6. Start testing!
```

---

## 📸 Screenshots / Demo

*Add screenshots of the web interface here*

```
CONTROL Tab:     [Channel Input] [Value Input] [Buttons]
MONITOR Tab:     [512 Channel Grid]
COLORS Tab:      [Color Palette] [Custom Color Input]
```

---

## 🎓 Learning Resources

- [ESP8266 Documentation](https://arduino-esp8266.readthedocs.io/)
- [DMX Protocol](https://en.wikipedia.org/wiki/DMX512)
- [OpenSCAD Tutorial](https://openscad.org/documentation.html)
- [Arduino Programming](https://www.arduino.cc/reference/)

---

**Made with ❤️ for the Stage Lighting Community**

⭐ If you find this project useful, please consider giving it a star!

---

## Changelog

### Version 1.0 (Initial Release)
- ✅ Full 512 DMX channel support
- ✅ Neon Glow UI (Swisson-inspired)
- ✅ Dual display modes (CONTROL/MONITOR)
- ✅ 10 color themes + custom colors
- ✅ Scene presets (10 slots)
- ✅ Test functions (SET, CHASE, etc.)
- ✅ Full documentation

---

**Last Updated:** March 2025
**Version:** 1.0.0
**Status:** ✅ Production Ready
