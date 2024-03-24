CC = gcc
CFLAGS = -pedantic -Ofast
LDFLAGS = -lm -lc -lX11 -lXrandr -lXext -lXrender -pthread
MAN_DIR = /usr/local/share/man/man1

SRC = src/main.c
EXEC = xtrail
MAN_PAGE = xtrail.1

all: $(EXEC)

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(EXEC) $(LDFLAGS)

install: $(EXEC) $(MAN_PAGE)
	install -Dm755 $(EXEC) $(DESTDIR)/usr/local/bin/$(EXEC)
	install -Dm644 $(MAN_PAGE) $(DESTDIR)$(MAN_DIR)/$(MAN_PAGE)

uninstall:
	rm -f $(DESTDIR)/usr/local/bin/$(EXEC)
	rm -f $(DESTDIR)$(MAN_DIR)/$(MAN_PAGE)

clean:
	rm -f $(EXEC)
