# dirt

A very basic, read-only file manager built on tint.

    make && ./dirt [folder]

- Double-click or Enter on a folder to open it; "Up" goes to the parent.
- Shows type, modified date, size and name. Folders always first.
- Sort by Name, Size (biggest first) or Date (newest first); tick "Show hidden files" for dotfiles.
- Up and Home buttons, plus a path bar: type a path (or ~/...) and press Enter.
- Needs a C compiler and an X11/XWayland session. No Xlib headers required (tint speaks the X11 protocol itself).

## License

dirt is free software under the **GNU GPL, version 3 or any later version**. See `LICENSE`.

## Credits

dirt is built with [tint](https://github.com/FacePafes/tint) by FacePafes, which stays under its own
license (LGPL-3.0-or-later, see `LICENSE.md`; `COPYING.md` is the GPL text it builds on).
The bundled fonts keep their own license (SIL OFL 1.1, see `OFL.md`).
tint's sources are included unchanged.