
all:matioLua.c
	gcc matioLua.c -g -shared -fPIC -o matioLua.dll -IC:/msys64/mingw64/include/lua5.3 -LC:/msys64/mingw64/lib -llua5.3 -lm  -lmatio


