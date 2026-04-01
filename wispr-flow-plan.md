# Wispr Flow for Linux — Build Plan

## Context
Build a Wispr Flow clone: a push-to-talk voice dictation app for Fedora/Wayland that records audio, sends it to a cloud STT API, and types the transcribed text into the active window. Written in C++/Qt6, API-based transcription (OpenAI Whisper or Groq).

## System Prerequisites

```bash
# Install before building
sudo dnf install gcc-c++ ydotool
sudo systemctl enable --now ydotoold.service
```

**Already available:**
- Qt 6.11.0 at `~/Qt/6.11.0/gcc_64` (Multimedia, Network, DBus, Widgets all present)
- CMake at `~/Qt/Tools/CMake/bin/cmake`
- Ninja at `~/Qt/Tools/Ninja/ninja`
- XDG GlobalShortcuts portal v1 (confirmed working)

## Project Structure

```
~/Projects/wispr-flow/
├── CMakeLists.txt
├── CMakePresets.json
├── src/
│   ├── main.cpp
│   ├── Application.h / .cpp        # Orchestrator: connects all components
│   ├── Settings.h / .cpp           # QSettings wrapper (API key, hotkey, backend)
│   ├── AudioCapture.h / .cpp       # QAudioSource recording (16kHz/16-bit/mono)
│   ├── AudioEncoder.h / .cpp       # Raw PCM -> WAV (44-byte header prepend)
│   ├── TranscriptionService.h      # Abstract interface
│   ├── WhisperApiService.h / .cpp  # OpenAI Whisper API POST
│   ├── GroqApiService.h / .cpp     # Groq API POST (same format, different URL)
│   ├── GlobalHotkey.h / .cpp       # XDG GlobalShortcuts portal via QDBus
│   ├── TextInjector.h / .cpp       # QProcess -> ydotool type
│   ├── TrayIcon.h / .cpp           # QSystemTrayIcon with state
│   ├── OverlayWidget.h / .cpp      # Floating recording indicator
│   └── SettingsDialog.h / .cpp     # Config UI (API key, backend, hotkey)
└── resources/
    ├── resources.qrc
    └── icons/  (mic-on.svg, mic-off.svg)
```

## Build Configuration

**CMakePresets.json** — points to Qt install:
- Generator: Ninja
- `CMAKE_PREFIX_PATH`: `~/Qt/6.11.0/gcc_64`
- PATH includes `~/Qt/Tools/CMake/bin` and `~/Qt/Tools/Ninja`

**CMakeLists.txt** — links Qt6::Core, Qt6::Gui, Qt6::Widgets, Qt6::Multimedia, Qt6::Network, Qt6::DBus. Uses `CMAKE_AUTOMOC ON`, C++20.

**Build commands:**
```bash
~/Qt/Tools/CMake/bin/cmake --preset default
~/Qt/Tools/CMake/bin/cmake --build build
./build/wispr-flow
```

---

## Implementation Phases (in order)

### Phase 1: Project Skeleton + Build
**Files:** `CMakeLists.txt`, `CMakePresets.json`, `src/main.cpp`

1. Create project directory and build files
2. `main.cpp`: minimal `QApplication` that creates a `QSystemTrayIcon` and enters event loop
3. Verify it compiles and shows a tray icon

**Verify:** `./build/wispr-flow` shows a tray icon, right-click -> Quit works.

---

### Phase 2: Audio Recording
**Files:** `AudioCapture.h/.cpp`, `AudioEncoder.h/.cpp`

**AudioCapture:**
- Uses `QAudioSource` with format: 16000 Hz, 16-bit signed int, 1 channel (mono)
- Internal `QBuffer` accumulates raw PCM data
- `start()` -> begins recording from default input device
- `stop()` -> stops, emits `recordingFinished(QByteArray rawPcm)`

**AudioEncoder:**
- Static method `QByteArray toWav(const QByteArray& pcm, int sampleRate=16000, int bitsPerSample=16, int channels=1)`
- Prepends 44-byte RIFF/WAV header to raw PCM data
- No external library needed

**Verify:** Wire tray icon left-click to toggle record. Save WAV to `/tmp/test.wav`, play it back with `aplay`.

---

### Phase 3: Transcription API
**Files:** `TranscriptionService.h`, `WhisperApiService.h/.cpp`, `GroqApiService.h/.cpp`, `Settings.h/.cpp`

**TranscriptionService (abstract):**
```cpp
virtual void transcribe(const QByteArray& wavData) = 0;
// signals: transcriptionReady(QString text), transcriptionError(QString err)
```

**WhisperApiService:**
- POST `multipart/form-data` to `https://api.openai.com/v1/audio/transcriptions`
- Fields: `file` (wav data, filename="audio.wav"), `model` = "whisper-1"
- Header: `Authorization: Bearer <key>`
- Uses `QNetworkAccessManager` + `QHttpMultiPart`
- Parse response JSON `{"text": "..."}` with `QJsonDocument`

**GroqApiService:**
- Same format, URL: `https://api.groq.com/openai/v1/audio/transcriptions`
- Model: `whisper-large-v3-turbo`

**Settings:**
- `QSettings` stored in `~/.config/wispr-flow/wispr-flow.conf`
- Keys: `api/backend` (openai|groq), `api/key`, `hotkey/shortcut`

**Verify:** Record audio -> POST to API -> print transcribed text to stdout.

---

### Phase 4: Text Injection
**Files:** `TextInjector.h/.cpp`

- `type(const QString& text)` spawns `QProcess("ydotool", {"type", "--clearmodifiers", "--", text})`
- Waits up to 5s for process to finish
- Emits `injectionError(QString)` if ydotool fails or isn't found
- On construction, verify `ydotool` is in PATH, warn if `ydotoold` isn't running

**Verify:** Full pipeline: record -> transcribe -> text appears in a text editor.

---

### Phase 5: Global Hotkey (Push-to-Talk)
**Files:** `GlobalHotkey.h/.cpp`

Uses XDG Desktop Portal `org.freedesktop.portal.GlobalShortcuts` via `QDBusInterface`:

1. `CreateSession()` -> get session handle
2. `BindShortcuts()` with shortcut id "record-toggle", preferred trigger `"<Super>d"` (configurable)
3. Connect to `Activated` signal -> emit `pressed()`
4. Connect to `Deactivated` signal -> emit `released()`
5. Push-to-talk: `pressed` -> `AudioCapture::start()`, `released` -> `AudioCapture::stop()` -> transcribe -> inject

**Fallback:** If portal unavailable, allow tray icon click as manual toggle.

**Verify:** Hold Super+D, speak, release -> text appears in focused window.

---

### Phase 6: UI Polish
**Files:** `TrayIcon.h/.cpp`, `OverlayWidget.h/.cpp`, `SettingsDialog.h/.cpp`, `Application.h/.cpp`

**TrayIcon:**
- Icon states: idle (grey mic), recording (red mic), transcribing (spinner/yellow mic)
- Right-click menu: Settings, About, Quit
- Left-click: manual toggle recording

**OverlayWidget:**
- Small frameless `QWidget` (Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool)
- Shows during recording: red dot + "Recording..." label
- Fixed position: top-right corner of primary screen
- Semi-transparent background

**SettingsDialog:**
- Backend selector: QComboBox (OpenAI / Groq)
- API key: QLineEdit (EchoMode::Password) with show/hide toggle
- Hotkey: QPushButton that captures next key combo
- Audio device: QComboBox populated from `QMediaDevices::audioInputs()`
- Save/Cancel buttons

**Application:**
- Owns all components, wires signals/slots:
  ```
  GlobalHotkey::pressed  -> AudioCapture::start + OverlayWidget::show
  GlobalHotkey::released -> AudioCapture::stop
  AudioCapture::recordingFinished -> AudioEncoder::toWav -> TranscriptionService::transcribe
  TranscriptionService::transcriptionReady -> TextInjector::type + OverlayWidget::hide
  TranscriptionService::transcriptionError -> TrayIcon::showError + OverlayWidget::hide
  ```

**Verify:** Full end-to-end: hotkey -> overlay appears -> speak -> overlay disappears -> text injected. Settings dialog saves/loads correctly.

---

## Key Technical Details

| Concern | Solution |
|---------|----------|
| Audio format | 16kHz, 16-bit, mono PCM via QAudioSource (PipeWire backend automatic) |
| WAV encoding | 44-byte RIFF header prepended to raw PCM, no library needed |
| HTTP client | QNetworkAccessManager + QHttpMultiPart (built into Qt6::Network) |
| JSON parsing | QJsonDocument (built into Qt6::Core) |
| Text injection | ydotool (kernel uinput level, works on GNOME Wayland) |
| Global hotkey | XDG GlobalShortcuts portal v1 via QDBusInterface |
| Config storage | QSettings INI at ~/.config/wispr-flow/ |
| No external C++ libs | Everything uses Qt6 built-in modules |

## Signal Flow

```
[Super+D press] -> GlobalHotkey::pressed()
    -> AudioCapture::start()
    -> OverlayWidget::show()

[Super+D release] -> GlobalHotkey::released()
    -> AudioCapture::stop()
    -> emits recordingFinished(pcmData)

recordingFinished(pcmData)
    -> AudioEncoder::toWav(pcmData) -> wavData
    -> TranscriptionService::transcribe(wavData)

transcriptionReady(text)
    -> TextInjector::type(text)
    -> OverlayWidget::hide()
    -> TrayIcon::setIdle()
```
