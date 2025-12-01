# My Robot Dog Board

A quadruped robot dog platform powered by ESP32, using oscillator-based gait control for smooth and natural locomotion.

## Hardware

### Servo Configuration (4 servos)

- **Front Left Leg**: GPIO 17
- **Front Right Leg**: GPIO 18
- **Rear Left Leg**: GPIO 8
- **Rear Right Leg**: GPIO 3

### Audio

- **Microphone**: I2S (WS: GPIO4, SCK: GPIO5, DIN: GPIO6)
- **Speaker**: I2S (BCLK: GPIO15, LRCK: GPIO16, DOUT: GPIO7)

### Display

- **SSD1315 OLED 128x64**: I2C (SDA: GPIO10, SCL: GPIO9)
- **I2C Address**: 0x3C
- **Display Type**: Monochrome (1-bit)
- **Emoji System**: Font Awesome icons (not Twemoji)

### Power

- **Charge Detect**: GPIO21
- **Battery ADC**: ADC2_CH3

## Features

### Oscillator-Based Gait Control

This robot dog uses **Central Pattern Generator (CPG)** inspired oscillators for natural locomotion. Each leg follows a sinusoidal motion pattern with configurable:

- **Amplitude**: Range of leg movement (degrees)
- **Offset**: Center position adjustment (degrees)
- **Period**: Cycle duration (milliseconds)
- **Phase**: Timing offset relative to other legs (radians)

### Unified Walking Gait System

The robot dog uses a single **unified walking gait** that automatically adapts based on speed parameters. This simplifies control while providing varied movement styles.

#### Walk (Adaptive Gait)
**Single gait with speed-based style selection** - The same walk command produces different movement patterns based on the speed parameter:

- **Fast Speed (300-400ms)**: **Trot-like diagonal movement** - Energetic diagonal coordination for quick travel and exploration
- **Medium Speed (400-700ms)**: **4-beat stable walking** - Natural sequential leg movement for general transportation
- **Slow Speed (700+ms)**: **Pace-like lateral movement** - Careful side-by-side coordination for precise navigation

#### Turn
**In-place rotation** - Rotate the robot dog without changing position
- **Direction**: Left (1) or Right (-1)
- **Use case**: Repositioning, changing orientation, navigating tight spaces

### Special Behaviors

- **Sit**: Rear legs bend and front legs position for stable sitting posture
- **Lay Down**: All legs extend to sides for lying flat on the ground
- **Shake**: Quick side-to-side body shake gesture (like shaking off water)
- **Jump**: Vertical jumping motion with crouch, leap, and landing phases
- **Bow**: Play bow gesture - front legs lowered while rear stays elevated
- **HandShake**: Lift and shake one front paw repeatedly (traditional handshake greeting)
- **HighFive**: Lift one front paw high and hold it still (for high-five greeting)
- **Home**: Return to default neutral standing position with proper forward stance

## MCP Tools (AI Control Interface)

### Action Control

**Tool**: `self.dog.action`

**Parameters**:
- `action` (string): Action name
- `steps` (integer, 1-20): Number of cycles (default: 4)
- `speed` (integer, 300-2000ms): Cycle period - **lower = faster** (default: 600)
- `direction` (integer, -1 or 1): 1=forward/left, -1=backward/right (default: 1)

**Available Actions**:

#### Gaits (require steps/speed/direction):
- `walk`: **Adaptive walking gait** - Automatically selects movement style based on speed parameter
  - Fast (300-400ms): Energetic trot-like diagonal movement
  - Medium (400-700ms): Stable 4-beat walking gait
  - Slow (700+ms): Careful pace-like lateral movement
- `turn`: Rotate in place (left or right)

#### Behaviors (no parameters needed unless specified):
- `sit`: Sit down in stable posture
- `laydown`: Lay flat on ground
- `shake`: Body shake gesture
- `jump`: Vertical jumping motion
- `bow`: Play bow gesture
- `handshake`: Shake paw repeatedly (requires direction/steps/speed, direction: 1=left paw, -1=right paw)
- `highfive`: High-five gesture (requires direction/speed, direction: 1=left paw, -1=right paw, speed=hold_time in ms)
- `home`: Return to neutral standing position

**Examples**:

```json
// Walk forward with medium speed (4-beat walking)
{"name": "self.dog.action", "arguments": {"action": "walk", "steps": 5, "speed": 600, "direction": 1}}

// Fast walk - produces trot-like diagonal movement
{"name": "self.dog.action", "arguments": {"action": "walk", "steps": 3, "speed": 350, "direction": 1}}

// Slow walk - produces pace-like lateral movement
{"name": "self.dog.action", "arguments": {"action": "walk", "steps": 3, "speed": 900, "direction": -1}}

// Turn left
{"name": "self.dog.action", "arguments": {"action": "turn", "steps": 4, "speed": 800, "direction": 1}}

// Sit down
{"name": "self.dog.action", "arguments": {"action": "sit"}}

// Shake left paw (handshake - repeated shaking)
{"name": "self.dog.action", "arguments": {"action": "handshake", "direction": 1, "steps": 5, "speed": 400}}

// Shake right paw
{"name": "self.dog.action", "arguments": {"action": "handshake", "direction": -1, "steps": 5, "speed": 400}}

// High-five with left paw (hold up for 1.5 seconds)
{"name": "self.dog.action", "arguments": {"action": "highfive", "direction": 1, "speed": 1500}}

// High-five with right paw (quick, hold for 1 second)
{"name": "self.dog.action", "arguments": {"action": "highfive", "direction": -1, "speed": 1000}}

// High-five and hold for 3 seconds (for photos!)
{"name": "self.dog.action", "arguments": {"action": "highfive", "direction": 1, "speed": 3000}}
```

### System Tools

| MCP Tool | Description | Parameters |
|----------|-------------|------------|
| `self.dog.stop` | Stop all actions and return home | None |
| `self.dog.get_status` | Get robot status | Returns: "moving" or "idle" |
| `self.dog.set_trim` | Calibrate servo position | `servo_type`: front_left/front_right/rear_left/rear_right<br>`trim_value`: -50 to 50 degrees |
| `self.dog.get_trims` | Get current trim settings | Returns JSON with all trims |
| `self.battery.get_level` | Get battery status | Returns: {"level": %, "charging": bool} |

## Speed Guidelines

**Period values** (ms) - **Lower numbers = faster motion**:

| Speed | Period | Walking Style | Use Case |
|-------|--------|--------------|----------|
| Very Fast | 300-400ms | **Trot-like** diagonal | Quick exploration, fast travel |
| Fast | 400-700ms | **4-beat stable walking** | General movement, normal walking |
| Medium | 700-900ms | **Pace-like** lateral | Careful navigation, precise movement |
| Slow | 900-2000ms | **Very controlled** walking | Precise positioning, demonstration |

## Amplitude Guidelines

**Amplitude values** (degrees) - How much the leg swings:

| Amplitude | Range | Use Case |
|-----------|-------|----------|
| Small | 10-15° | Subtle movements, wiggle |
| Medium | 20-30° | Normal walking gaits |
| Large | 35-45° | Fast gaits, jumping |

## Voice Command Examples

- "Move forward" / "Walk forward 5 steps"
- "Walk fast" / "Walk slowly" / "Run" (speed-based walking)
- "Turn left" / "Turn right"
- "Sit down" / "Lay down"
- "Shake" / "Shake your body"
- "Jump" / "Play bow"
- "Shake hands" / "Give me your paw" / "Shake your left paw" (traditional handshake)
- "Give me five" / "High five!" / "Show me your paw" / "Put your paw up" (high-five)
- "Walk slowly" / "Walk quickly" / "Run fast"
- "Stop"

## Calibration

Each servo can be calibrated using the `self.dog.set_trim` tool to correct mechanical alignment:

```json
// Example: Adjust front left leg by +5 degrees
{"name": "self.dog.set_trim", "arguments": {"servo_type": "front_left", "trim_value": 5}}
```

Trim values are **permanently saved** to NVS flash.

## HandShake vs HighFive - Key Differences

| Feature | HandShake | HighFive |
|---------|-----------|----------|
| **Voice Commands** | "Shake hands", "Give me your paw" | "Give me five", "High five!", "Show me your paw" |
| **Paw Height** | Medium (70°) | High (60°/120°) - raised up like human high-five |
| **Motion Type** | **Dynamic** - Shaking up/down | **Static** - Hold position |
| **Duration** | Multiple shake cycles (e.g., 5 × 400ms = 2s) | Single hold (default 1.5s) |
| **Parameters** | `steps`, `speed`, `direction` | `speed` (=hold_time), `direction` |
| **Use Case** | Traditional paw shake greeting | High-five greeting, showing paw, photos |
| **Action** | Oscillates paw up and down repeatedly | Lifts and holds paw still |

**Example Comparison:**

```json
// HandShake: Dog shakes paw 5 times
{"action": "handshake", "direction": 1, "steps": 5, "speed": 400}
// Result: Paw oscillates up/down 5 times over 2 seconds

// HighFive: Dog lifts paw and holds for 1.5 seconds
{"action": "highfive", "direction": 1, "speed": 1500}
// Result: Paw lifts high and stays still for 1.5 seconds (ready for slap!)
```

## Technical Details

### Oscillator Algorithm

Each leg follows the equation:

```
position(t) = offset + amplitude × sin(ωt + φ)

where:
  ω = 2π / period (angular frequency)
  φ = phase_diff (phase offset in radians)
  t = current time
```

### Unified Walking Gait Implementation

The robot dog uses a **single adaptive walking algorithm** that automatically selects different phase patterns based on the speed parameter:

**Fast Speed (300-400ms)** - **Trot-like Pattern**:
```
Front Left:  phase = 0°
Front Right: phase = 180°
Rear Left:   phase = 180°
Rear Right:  phase = 0°
```

**Medium Speed (400-700ms)** - **4-beat Walking Pattern**:
```
Front Left:  phase = 0°
Front Right: phase = 180°
Rear Left:   phase = 270°
Rear Right:  phase = 90°
```

**Slow Speed (700+ms)** - **Pace-like Pattern**:
```
Front Left:  phase = 0°
Front Right: phase = 180°
Rear Left:   phase = 0°
Rear Right:  phase = 180°
```

This approach provides **simplified control** while maintaining movement diversity through speed-based gait selection.

### Update Rate

- **Servo refresh**: 33Hz (every 30ms)
- **Loop frequency**: ~143Hz (every ~7ms)
- **Task delay**: 5ms (cooperative multitasking)

## Version

**Version**: 1.0.0

## Architecture

```
MyRobotDogBoard (main board class)
    ↓
DogController (MCP tool registration, action queue)
    ↓
RobotDog (gait algorithms)
    ↓
Oscillator × 4 (sine wave generators)
    ↓
LEDC PWM (hardware servo control)
```

## Display System

### OLED Display (128×64 monochrome)

This board uses a **SSD1315 OLED display** (SSD1306 compatible) with I2C interface.

**Features:**
- **Resolution**: 128×64 pixels (monochrome)
- **Interface**: I2C (400kHz)
- **Display class**: `OledDisplay`
- **Emoji system**: Font Awesome icons

**Display Layout:**
```
┌────────────────────────────┐
│ [Icon] Status Text         │  ← Status bar (top)
├────────────────────────────┤
│                            │
│     [Emoji Icon]           │  ← Emotion icon (center)
│                            │
│  Scrolling chat message    │  ← Chat text (bottom)
└────────────────────────────┘
```

### Emotion Icons (Font Awesome)

Instead of colored Twemoji images, this OLED display uses **Font Awesome icon font** for emotions:

| Emotion | Icon | Unicode |
|---------|------|---------|
| Neutral | 😐 | U+F11A (meh) |
| Happy | 😊 | U+F118 (smile) |
| Sad | ☹️ | U+F119 (frown) |
| Angry | 😡 | U+F556 (angry) |
| Surprised | 😲 | U+F5A4 (surprise) |
| Thinking | 🤔 | U+F4FC (head-side) |
| Sleepy | 😴 | U+F5B1 (bed) |
| Listening | 🎤 | U+F130 (microphone) |
| Speaking | 🔊 | U+F028 (speaker) |
| Connecting | 🔗 | U+F0C1 (link) |

**Why Font Awesome instead of Twemoji?**
- OLED displays are **monochrome** (can't show colors)
- Font Awesome icons are **scalable** (look good at any size)
- Icons use **minimal memory** (~10KB vs 42KB for Twemoji)
- Icons are **crisp** on small displays

### Display Pinout

```
SSD1315 OLED Module:
┌─────────────┐
│  VCC → 3.3V │
│  GND → GND  │
│  SDA → GP10 │
│  SCL → GP9  │
└─────────────┘
```

### Usage

The display automatically shows:
- **Status indicators**: Listening (🎤), Speaking (🔊), etc.
- **Emotion icons**: Based on AI responses
- **Chat messages**: Scrolling text at bottom

No manual configuration needed - the system handles all display updates.

## Build Configuration

To build for this board:

```bash
cd /path/to/xiaozhi-esp32

# Set target
idf.py set-target esp32s3

# Configure board
idf.py menuconfig
# → Xiaozhi Assistant → Board Type → My Robot Dog Board

# Build
idf.py build

# Flash
idf.py flash monitor
```

**Board Registration:**
- **Kconfig**: `CONFIG_BOARD_TYPE_MY_ROBOT_DOG_BOARD`
- **Board directory**: `boards/my-robot-dog-board/`
- **CMake**: Configured in `main/CMakeLists.txt`

## Credits

Based on the oscillator gait control system inspired by:
- Otto DIY bipedal robot
- Central Pattern Generator (CPG) research
- Quadruped locomotion patterns from robotics literature

---

**Note**: This is a demonstration platform. Adjust servo pins, display configuration, and power management according to your specific hardware setup.
