NAME = shell
MAIN = shell.c
SRCS = command.c joblist.c terminal.c
HS = $(SRCS:.c=.h)
CC = gcc
CFLAGS = -Wall -Werror -g -lreadline -std=gnu99

$(NAME): $(MAIN) $(SRCS) $(HS)
	$(CC) $(CFLAGS) $(MAIN) $(SRCS) -o $(NAME)

clean:
	rm -f *.o $(NAME)
	rm -rf *.dSYM
