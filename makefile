
all:matioLua.c
	gcc matioLua.c -g -shared -fPIC -o matioLua.so -I/usr/include/lua5.2 -L/usr/lib -lm -llua5.2 -lmatio


