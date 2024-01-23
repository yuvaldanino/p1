CC = gcc 

CFLAGS = -Wall -Werror -g

all: sshell

sshell: sshell.c
	$(CC) $(CFLAGS) -o sshell sshell.c

clean:
	rm -f sshell