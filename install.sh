#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BINARY="$SCRIPT_DIR/build/wispr-flow"

if [ ! -f "$BINARY" ]; then
    echo "Binary not found. Building first..."
    ~/Qt/Tools/CMake/bin/cmake --preset default
    ~/Qt/Tools/CMake/bin/cmake --build build
fi

echo "Installing wispr-flow..."

# Binary
mkdir -p ~/.local/bin
cp "$BINARY" ~/.local/bin/wispr-flow
chmod +x ~/.local/bin/wispr-flow

# Qt libraries need to be findable — create a wrapper script
cat > ~/.local/bin/wispr-flow-run << 'WRAPPER'
#!/bin/bash
export LD_LIBRARY_PATH="$HOME/Qt/6.11.0/gcc_64/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$HOME/Qt/6.11.0/gcc_64/plugins"
exec "$HOME/.local/bin/wispr-flow" "$@"
WRAPPER
chmod +x ~/.local/bin/wispr-flow-run

# Desktop entry (launcher)
mkdir -p ~/.local/share/applications
sed "s|Exec=wispr-flow|Exec=$HOME/.local/bin/wispr-flow-run|" \
    "$SCRIPT_DIR/wispr-flow.desktop" > ~/.local/share/applications/wispr-flow.desktop

# Autostart entry
mkdir -p ~/.config/autostart
cp ~/.local/share/applications/wispr-flow.desktop ~/.config/autostart/wispr-flow.desktop

echo ""
echo "Installed:"
echo "  Binary:    ~/.local/bin/wispr-flow"
echo "  Wrapper:   ~/.local/bin/wispr-flow-run"
echo "  Launcher:  ~/.local/share/applications/wispr-flow.desktop"
echo "  Autostart: ~/.config/autostart/wispr-flow.desktop"
echo ""
echo "Wispr Flow will now:"
echo "  - Appear in your app launcher"
echo "  - Start automatically on login"
echo "  - Use Ctrl+Alt+D to toggle recording"
