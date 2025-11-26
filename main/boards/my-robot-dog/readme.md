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

### Supported Gaits

#### 1. Trot (Default)
**Diagonal gait** - Front-left + Rear-right move together, Front-right + Rear-left move together.
- **Speed**: Medium (600ms period)
- **Stability**: High
- **Use case**: General walking, exploration

#### 2. Walk
**4-beat gait** - Each leg moves individually in sequence: FL → RR → FR → RL
- **Speed**: Slow (800ms period)
- **Stability**: Very high
- **Use case**: Slow, careful movement

#### 3. Pace
**Lateral gait** - Left legs move together, right legs move together
- **Speed**: Medium (700ms period)
- **Stability**: Medium (like camel walk)
- **Use case**: Fast forward motion

#### 4. Bound
**Jumping gait** - Front legs together, rear legs together
- **Speed**: Fast (500ms period)
- **Stability**: Low (dynamic)
- **Use case**: Fast forward motion, jumping

#### 5. Gallop
**Asymmetric gait** - Like a running horse
- **Speed**: Very fast (400ms period)
- **Stability**: Dynamic
- **Use case**: Maximum speed

### Special Behaviors

- **Sit**: Rear legs fold, front legs stay upright
- **Lay Down**: All legs fold to sides
- **Shake**: Quick side-to-side body shake
- **Wiggle**: Tail-wagging motion (rear legs alternate)
- **Jump**: Crouch then extend all legs
- **Bow**: Play bow gesture (front legs down, rear up)
- **HandShake**: Lift and shake one front paw repeatedly (traditional handshake)
- **HighFive**: Lift one front paw high and hold it (for high-five greeting)
- **Turn**: Rotate in place (left or right)

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
- `trot`: Diagonal gait (recommended default)
- `walk`: Slow 4-beat gait
- `pace`: Lateral gait
- `bound`: Bounding gait
- `gallop`: Fast running gait
- `turn`: Rotate in place

#### Behaviors:
- `sit`: Sit down
- `laydown`: Lay flat
- `shake`: Body shake
- `wiggle`: Tail wiggle (requires steps/speed)
- `jump`: Jump up
- `bow`: Play bow
- `handshake`: Shake paw repeatedly (requires direction/steps/speed, direction: 1=left paw, -1=right paw)
- `highfive`: High-five gesture (requires direction/speed, direction: 1=left paw, -1=right paw, speed=hold_time in ms)
- `home`: Return to rest position

**Examples**:

```json
// Trot forward 5 steps at medium speed
{"name": "self.dog.action", "arguments": {"action": "trot", "steps": 5, "speed": 600, "direction": 1}}

// Walk backward slowly
{"name": "self.dog.action", "arguments": {"action": "walk", "steps": 3, "speed": 1000, "direction": -1}}

// Turn left
{"name": "self.dog.action", "arguments": {"action": "turn", "steps": 4, "speed": 800, "direction": 1}}

// Sit down
{"name": "self.dog.action", "arguments": {"action": "sit"}}

// Wiggle tail
{"name": "self.dog.action", "arguments": {"action": "wiggle", "steps": 5, "speed": 400}}

// Fast gallop forward
{"name": "self.dog.action", "arguments": {"action": "gallop", "steps": 3, "speed": 400, "direction": 1}}

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

| Speed | Period | Use Case |
|-------|--------|----------|
| Very Fast | 300-400ms | Gallop, emergency |
| Fast | 400-500ms | Bound, quick movements |
| Medium | 600-800ms | Trot, general walking |
| Slow | 900-1200ms | Walk, careful navigation |
| Very Slow | 1300-2000ms | Precise positioning |

## Amplitude Guidelines

**Amplitude values** (degrees) - How much the leg swings:

| Amplitude | Range | Use Case |
|-----------|-------|----------|
| Small | 10-15° | Subtle movements, wiggle |
| Medium | 20-30° | Normal walking gaits |
| Large | 35-45° | Fast gaits, jumping |

## Voice Command Examples

- "Move forward" / "Trot forward 5 steps"
- "Turn left" / "Turn right"
- "Sit down" / "Lay down"
- "Shake" / "Wiggle your tail"
- "Jump" / "Play bow"
- "Shake hands" / "Give me your paw" / "Shake your left paw" (traditional handshake)
- "Give me five" / "High five!" / "Show me your paw" / "Put your paw up" (high-five)
- "Walk slowly" / "Run fast"
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

### Gait Phase Patterns

**Trot** (diagonal coordination):
```
Front Left:  phase = 0°
Front Right: phase = 180°
Rear Left:   phase = 180°
Rear Right:  phase = 0°
```

**Walk** (sequential):
```
Front Left:  phase = 0°
Front Right: phase = 180°
Rear Left:   phase = 270°
Rear Right:  phase = 90°
```

**Bound** (front/rear pairs):
```
Front Left:  phase = 0°
Front Right: phase = 0°
Rear Left:   phase = 180°
Rear Right:  phase = 180°
```

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
