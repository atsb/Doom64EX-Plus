#!/bin/bash
i686-w64-mingw32.static-windres -O coff -i Launcher.rc -o Launcher.res
i686-w64-mingw32.static-gcc -D_CRT_SECURE_NO_WARNINGS -DNDEBUG -D_WINDOWS Launcher.c tk_lib.c -o DOOM64EX-Plus-Launcher.exe -lkernel32 -luser32 -lgdi32 -lwinspool -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lodbc32 -lodbccp32 -lcomctl32 Launcher.res

