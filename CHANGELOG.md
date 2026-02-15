# Changelog

## hypr-tofi

### [0.1.0] - Fork of tofi 0.9.1

Initial fork release with extended features:

- **Math expressions**: Evaluate math using `=` prefix or auto-detect on no matches
- **URL detection**: Auto-detect and launch URLs when no entries match  
- **Calc result entries**: Results appear as selectable entries, copy on selection
- Config directory: `~/.config/hypr-tofi/`

---

## Upstream tofi changelog

*From original [tofi](https://github.com/philj56/tofi) by Philip Jones*

## [0.9.1] - 2023-04-10
### Fixed
- Fixed broken line spacing for some fonts with the HarfBuzz backend.

## [0.9.0] - 2023-04-09
### Added
- Added support for a text cursor. This can be enabled with the `text-cursor`
  option, the style of cursor chosen with `text-cursor-style`, and themed
  similarly to other text.
- Added support for fractional scaling, correcting the behaviour of percentage
  sizes when fractional scaling is used.
- Added Ctrl-n, Ctrl-p, Page-Up and Page-Down keybindings.
- Added `auto-accept-single` option, to automatically accept the last remaining
  result when there is only one.

### Changed
- The `font` option now performs home path substitution for paths starting with
  `~/`.

### Fixed
- Fixed some more potential errors from malformed config files.
- Fixed some potential memory leaks when generating caches.
- Fixed rounded corners when a background padding of -1 is specified.
- Fixed broken text rendering with some versions of Harfbuzz.
- Fixed some man page typos.


## [0.8.1] - 2022-12-01
### Fixed
- Stop debug logs printing in release builds.


## [0.8.0] - 2022-12-01
### Deprecated
Text styling has been overhauled in this update, and as a result the
`selection-padding` option has been replaced with
`selection-background-padding`, to avoid ambiguity and match the other
available options. `selection-padding` is therefore deprecated, and will be
removed in a future version of tofi, so please update your configs.

### Added
- Added `placeholder-text` option.
- Overhaul text styling. Each piece of text in tofi is now styleable in a
  similar way, with foreground and background colours. The pieces of text
  that can be individually styled are:
    - `prompt`
    - `placeholder`
    - `input`
    - `default-result`
    - `alternate-result`
    - `selection`
