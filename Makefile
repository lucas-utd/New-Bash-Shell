all: UM.exe user.exe admin.exe
UM.exe: UM.c shsh.c
	gcc -o UM.exe UM.c shsh.c -lm -std=c99 -D_XOPEN_SOURCE=700 -D_BSD_SOURCE -D_REENTRANT -lpthread
user.exe: user.c
	gcc -o user.exe user.c -lm -std=c99 -D_XOPEN_SOURCE=700 -D_BSD_SOURCE -D_REENTRANT -lpthread
admin.exe: admin.c
	gcc -o admin.exe admin.c -std=c99 -D_XOPEN_SOURCE=700 -D_BSD_SOURCE
clean:
	rm -f UM.exe user.exe admin.exe
