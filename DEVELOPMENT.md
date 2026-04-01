# Wispr Flow — Development Notes

## What It Is

A push-to-talk voice dictation app for Fedora/GNOME/Wayland. Press Ctrl+Alt+D, speak, press again — transcribed text is typed into the focused window.

## Architecture

```
Ctrl+Alt+D → GNOME keybinding → dbus-send → GlobalHotkey::onToggle()
                                                    ↓
                                            Application toggles
                                            AudioCapture start/stop
                                                    ↓
                                            AudioEncoder::toWav()
                                                    ↓
                                            GroqApiService::transcribe()
                                              (POST multipart/form-data)
                                                    ↓
                                            TextInjector::type()
                                              (ydotool type --key-delay 0)
```

## Key Technical Decisions

### Global Hotkey (the hard part)

**XDG GlobalShortcuts portal — didn't work.** The portal (`org.freedesktop.portal.GlobalShortcuts`) registered bindings but GNOME 49 requires user approval via a system dialog that never appeared reliably. Issues encountered:
- `QDBusArgument` marshalling for `a(sa{sv})` required registering a custom `PortalShortcut` struct with `qDBusRegisterMetaType`
- Portal accepted `BindShortcuts` but Activated/Deactivated signals never fired on GNOME
- Portal is more suited for Flatpak/sandboxed apps

**Solution: GNOME custom keybinding via gsettings + DBus.** The app:
1. Registers a DBus service (`com.wispr.Flow`) with a `Q_SCRIPTABLE` slot `onToggle()`
2. Creates a GNOME custom keybinding via `gsettings` that runs `dbus-send` to call that method
3. Cleans up the keybinding on exit via `removeKeybinding()`

Important: `Q_SCRIPTABLE` + `ExportScriptableSlots` is required — `ExportAllSlots` only exports public slots without Q_SCRIPTABLE. The interface name Qt generates is `local.wispr_flow.GlobalHotkey` (derived from the binary name and class), not the DBus service name.

### Audio Capture

**Pull mode, not push mode.** Initially used `QAudioSource::start(&QBuffer)` (push mode) but QBuffer didn't grow reliably. Switched to pull mode:
- `m_ioDevice = m_source->start()` returns a `QIODevice*`
- Connect to `readyRead` signal, call `readAll()` to accumulate PCM data
- More reliable for long recordings

Format: 16kHz, 16-bit signed int, mono — optimal for Whisper.

### Bluetooth AirPods Issues

AirPods Pro 2 mic drops audio after 3-4 seconds on Linux. This is a BlueZ/PipeWire issue:
- WirePlumber has `autoswitch-bluetooth-profile.lua` that switches between A2DP (playback) and HFP/HSP (mic)
- The `PROFILE_RESTORE_TIMEOUT_MSEC = 2000` reverts to A2DP too aggressively
- Even disabling the autoswitch config and manually switching profiles via `pactl set-card-profile`, the mic still drops
- Confirmed with Audacity — same behavior, not app-specific
- **Workaround:** Use the built-in laptop mic. Audio device is selectable in Settings.

### Text Injection

`ydotool type` with `--key-delay 0` for fast typing. Works in both GUI apps and terminals (unlike clipboard paste which needs Ctrl+V in GUI but Ctrl+Shift+V in terminals). Requires `ydotoold` service running:
```bash
sudo systemctl start ydotool
```

### Tray Icon on GNOME

GNOME/Wayland doesn't show `QSystemTrayIcon` by default. Requires:
```bash
sudo dnf install gnome-shell-extension-appindicator
gnome-extensions enable appindicatorsupport@rgcjonas.gmail.com
```
Log out/in after installing. The AppIndicator extension doesn't distinguish left/right click — always shows context menu. Added "Start/Stop Recording" as first menu item.

### WAV Encoding

44-byte RIFF header prepended to raw PCM. No external library needed — just `QDataStream` with `LittleEndian` byte order. Requires `#include <QIODevice>` for `QIODevice::WriteOnly`.

## Build

### Prerequisites

```bash
sudo dnf install gcc-c++ ydotool mesa-libGL-devel mesa-libEGL-devel
sudo systemctl start ydotool
```

Qt 6.11.0 installed at `~/Qt/6.11.0/gcc_64` (via Qt online installer).

### Commands

```bash
~/Qt/Tools/CMake/bin/cmake --preset default
~/Qt/Tools/CMake/bin/cmake --build build
./build/wispr-flow
```

## Install / Uninstall

```bash
./install.sh    # Copies to ~/.local/bin, adds launcher + autostart
./uninstall.sh  # Removes everything including GNOME keybinding
```

## Files

| File | Purpose |
|------|---------|
| `Application.cpp` | Orchestrator — wires all components via signals/slots |
| `AudioCapture.cpp` | QAudioSource pull-mode recording |
| `AudioEncoder.cpp` | Raw PCM → WAV (44-byte RIFF header) |
| `GlobalHotkey.cpp` | GNOME keybinding + DBus service for toggle |
| `GroqApiService.cpp` | Groq Whisper API (POST multipart/form-data) |
| `WhisperApiService.cpp` | OpenAI Whisper API (same format, different URL) |
| `TextInjector.cpp` | ydotool type with zero key delay |
| `TrayIcon.cpp` | System tray with state (idle/recording/transcribing) |
| `OverlayWidget.cpp` | Floating "Recording..." indicator |
| `SettingsDialog.cpp` | Backend, API key, audio device selection |
| `Settings.cpp` | QSettings wrapper (~/.config/wispr-flow/) |

## Debug Files

- `/tmp/wispr-flow-last.wav` — last recorded audio
- `/tmp/wispr-flow-transcripts.log` — all transcriptions with timestamps

## API Setup

**Groq (recommended — free tier, fast):** https://console.groq.com → API Keys → Create
**OpenAI:** https://platform.openai.com → API Keys → Create

Configure via tray icon → Settings.
