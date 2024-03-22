CC = gcc
CFLAGS = -pedantic -Ofast
LDFLAGS = -lm -lc -lX11 -lXrandr -lXext -lXrender -pthread

SRC = src/main.c
EXEC = xtrail

all: $(EXEC)

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(EXEC)
