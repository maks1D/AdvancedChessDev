# AdvancedChess

AdvancedChess is an open-source cross-platform dependency-free chess server written in C.

## Prerequisites

### Windows

- Microsoft Visual Studio 2019
- Platform toolset v142
- Windows 10 SDK (preferably the latest version)

### Linux

- clang (version 6 or newer)
## Building

### Windows

Open `AdvancedChess.sln` in Visual Studio 2019, select **Release** configuration and press <kbd>CTRL</kbd> + <kbd>B</kbd>.

### Linux

```
chmod +x build.sh
./build.sh
```

**Warning!** Server on linux has to be running with root permissions!
## License

This project is licensed under the MIT License – see the `LICENSE` file for details.