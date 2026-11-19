# dirt

A very basic, read-only file manager built on tint.

    make && ./dirt [folder]

- Double-click or Enter on a folder to open it; "Up" goes to the parent.
- Shows type (DIR/FILE), size and name. Folders first, dotfiles hidden.
- Needs a C compiler and an X11/XWayland session. No Xlib headers required (tint speaks the X11 protocol itself).

## Credits

dirt is built with [tint](https://github.com/FacePafes/tint) by FacePafes (LGPL-3.0-or-later).
tint's sources and license files are included unchanged (see LICENSE.md, COPYING.md, OFL.md).
