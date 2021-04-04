# AdvancedChess

AdvancedChess is an open-source cross-platform dependency-free chess server written in C99.

## This project is still in development state. Working features:

- [ ] Creating accounts
- [ ] Logging in
- [ ] Join messages
- [ ] Online members list
- [ ] Sending messages
- [ ] Seeing who is typing
- [ ] Unread messages
- [ ] Deleting messages
- [ ] Editing messages
- [ ] Adding reactions
- [ ] Removing reactions
- [ ] Creating games
- [ ] Making moves
- [ ] Games analysis

## Prerequisites

### Windows

- Microsoft Visual Studio 2019
- Platform toolset v142
- Windows 10 SDK (preferably the latest version)

### Linux

- clang (version 6 or newer)
## Building

### Windows

Open `AdvancedChess.sln` in Visual Studio 2019, select `Release` configuration and press <kbd>CTRL</kbd> + <kbd>B</kbd>.

### Linux

```
chmod +x build.sh
./build.sh
```

## Adding configuration file

Before you can run the server for the first time, you need to create a copy of `settings.txt.example` and name it `settings.txt`. You can change the server settings by editing this file.

## License

This project is licensed under the MIT License – see the `LICENSE` file for details.