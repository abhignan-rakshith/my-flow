#!/bin/bash
set -e

echo "Uninstalling wispr-flow..."

# Stop running instance
pkill -f wispr-flow 2>/dev/null || true

# Remove GNOME keybinding
KEYBINDING_PATH="/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/wispr-flow/"
CURRENT=$(gsettings get org.gnome.settings-daemon.plugins.media-keys custom-keybindings 2>/dev/null)
if echo "$CURRENT" | grep -q "$KEYBINDING_PATH"; then
    NEW=$(echo "$CURRENT" | sed "s|'$KEYBINDING_PATH'||" | sed 's/, ,/,/g' | sed 's/\[, /[/' | sed 's/, \]/]/')
    [ "$NEW" = "[]" ] && NEW="@as []"
    gsettings set org.gnome.settings-daemon.plugins.media-keys custom-keybindings "$NEW"
    echo "  Removed keybinding"
fi

rm -f ~/.local/bin/wispr-flow
rm -f ~/.local/bin/wispr-flow-run
rm -f ~/.local/share/applications/wispr-flow.desktop
rm -f ~/.config/autostart/wispr-flow.desktop

echo "  Removed all installed files"
echo "Done."
