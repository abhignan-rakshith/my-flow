# Wispr Flow

Push-to-talk voice dictation for Linux, macOS, and Windows. Hold a hotkey, speak, release — your words are transcribed and typed into the focused window.

## How It Works

1. Hold **Ctrl+Option+Space** (macOS) or **Ctrl+Alt+D** (Linux) to start recording
2. Speak naturally
3. Release the hotkey — audio is sent to a speech-to-text API
4. Transcribed text is cleaned up by an LLM and typed into the active window

## Quick Start

### Prerequisites

**macOS:**
- Qt 6 (via [Qt Online Installer](https://www.qt.io/download-qt-installer))
- Grant Accessibility permission when prompted (System Settings → Privacy → Accessibility)

**Linux (Fedora/GNOME):**
```bash
sudo dnf install gcc-c++ ydotool mesa-libGL-devel mesa-libEGL-devel
sudo systemctl start ydotool
```
- Qt 6 installed at `~/Qt/6.11.0/gcc_64`
- GNOME AppIndicator extension for tray icon (see [DEVELOPMENT.md](DEVELOPMENT.md))

### Build & Run

```bash
cmake --preset default
cmake --build build
./build/wispr-flow        # Linux
open build/wispr-flow.app # macOS
```

### Configure

Right-click the tray icon → **Settings** to set:
- **Backend:** Groq (recommended, free tier) or OpenAI
- **API Key:** Get one from [Groq Console](https://console.groq.com) or [OpenAI Platform](https://platform.openai.com)
- **Audio Device:** Select your microphone

## Architecture

```
Hotkey held → AudioCapture starts (48kHz/16-bit/mono)
Hotkey released → AudioCapture stops
    → AudioEncoder::toWav()
    → TranscriptionService::transcribe() (Groq or OpenAI Whisper API)
    → LLM post-processing (grammar, punctuation, filler word removal)
    → TextInjector::type() into focused window
```

See [DEVELOPMENT.md](DEVELOPMENT.md) for technical decisions, platform quirks, and debugging tips.

## Project Structure

| File | Purpose |
|------|---------|
| `Application` | Orchestrator — wires all components via signals/slots |
| `AudioCapture` | Pull-mode recording with post-processing (filter, gate, normalize) |
| `AudioEncoder` | Raw PCM → WAV (44-byte RIFF header, no external deps) |
| `GlobalHotkey` | Platform-specific global hotkey (GNOME DBus / Carbon / WinAPI) |
| `GroqApiService` | Groq Whisper API client |
| `WhisperApiService` | OpenAI Whisper API client |
| `TextInjector` | Text injection (ydotool / CGEventPost / SendInput) |
| `TrayIcon` | System tray with state indicators |
| `OverlayWidget` | Floating "Recording..." indicator |
| `Settings` | Persistent config via QSettings |
| `SettingsDialog` | UI for backend, API key, and audio device |

## License

All rights reserved.
