# Arduino setup

- This [tutorial](https://www.youtube.com/watch?v=LGoTi9iqgDI) and corresponding (code)[https://github.com/jakobwesthoff/inkplate10-first-tinkering/] is very useful to get started

- Install Visual Studio Code

  - Open the source folder
  - This will prompt the install:
    - PlatformIO extension
    - [Quick start](https://docs.platformio.org/en/latest/integration/ide/vscode.html#quick-start)

- Build (Ctrl-Alt-b)

- Upload to the board (LHS PlatformIO icon in VSCode / esp32 / General / Upload)

- Notes:
  - In "platform.ini" add -DARDUINO_INKPLATE10 or -DARDUINO_INKPLATE10V2 to "build_flags" depending on which version of the board you have
  - Added lib_ldf_mode=deep to platform.ini to fix "FS.h" not found errors
  - SD card needs to be FAT32 formatted. Used ["guiformat"](http://ridgecrop.co.uk/index.htm?guiformat.htm)
  - Installed the [CH340](https://e-radionica.com/en/blog/ch340-driver-installation-croduino-basic3-nova2/) drivers.  Not 100% sure it is really required.

