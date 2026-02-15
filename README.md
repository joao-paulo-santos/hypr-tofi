# hypr-tofi

A fast dmenu/rofi replacement for Wayland compositors, forked from [tofi](https://github.com/philj56/tofi) by Philip Jones.

This fork extends tofi with smart input detection and calculation capabilities.

## Features

- All original tofi features (fast startup, theming, drun/run modes)
- **Math expressions**: Type `=4+5*4` to evaluate and add result as a selectable entry
- **Smart detection**: When no entries match, automatically detects math expressions or URLs
- **URL launching**: Type a URL directly or prefix with `url:` to open in browser

## Differences from tofi

| Feature | tofi | hypr-tofi |
|---------|------|-----------|
| Math calculation | No | Yes (`=` prefix or auto-detect) |
| URL detection | No | Yes |
| Config directory | `~/.config/tofi` | `~/.config/hypr-tofi` |

## Install

### Building

Install dependencies (Arch):
```sh
sudo pacman -S freetype2 harfbuzz cairo pango wayland libxkbcommon meson scdoc wayland-protocols
```

Build:
```sh
meson build && ninja -C build install
```

## Usage

Basic usage (same as tofi):
```sh
# dmenu mode - read from stdin
echo -e "foo\nbar\nbaz" | hypr-tofi

# Run mode - executables in $PATH
hypr-tofi-run

# Drun mode - desktop applications
hypr-tofi-drun
```

### Math

Explicit calculation (with `=` prefix):
```
=4+5*4        → adds "calc > 24" entry
=sin(45)*10   → adds "calc > 7.07..." entry
```

Implicit calculation (no matches, valid math detected):
```
4+5*4         → adds "calc > 24" entry
```

Selecting a calc entry copies the result to clipboard.

### URLs

Explicit URL (with `url:` prefix):
```
url:https://example.com
```

Implicit URL (no matches, URL pattern detected):
```
https://example.com
www.example.com
```

Selecting a URL entry opens it with `xdg-open`.

## Configuration

Config file: `~/.config/hypr-tofi/config`

See `doc/config` for all options (same as tofi).

## Credits

- **Original author**: [Philip Jones](https://github.com/philj56) - [tofi](https://github.com/philj56/tofi)
- **License**: MIT

## License

MIT License - see [LICENSE](LICENSE)
