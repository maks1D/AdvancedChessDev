#!/bin/bash

mkdir -p obj
clang src/main.c -c -o obj/main.o -std=gnu99
clang src/base64.c -c -o obj/base64.o -std=gnu99
clang src/captcha.c -c -o obj/captcha.o -std=gnu99
clang src/chess.c -c -o obj/chess.o -std=gnu99
clang src/configuration.c -c -o obj/configuration.o -std=gnu99
clang src/database.c -c -o obj/database.o -std=gnu99
clang src/endianness.c -c -o obj/endianness.o -std=gnu99
clang src/hasher.c -c -o obj/hasher.o -std=gnu99
clang src/server.c -c -o obj/server.o -std=gnu99
clang obj/main.o obj/base64.o obj/captcha.o obj/chess.o obj/configuration.o obj/database.o obj/endianness.o obj/hasher.o obj/server.o -o AdvancedChess