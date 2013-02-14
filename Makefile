
#CFLAGS="-g"
CFLAGS="-O2"

genserv: genserv.c
	gcc genserv.c -o genserv

clean:
	rm genserv

