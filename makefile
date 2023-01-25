SOURCES = chip8.c
EXE = chip8.exe
CFLAGS = -Wall -Wextra -Wpedantic -Werror
LDFLAGS = -I include -L lib
LIBS = -lraylib -lopengl32 -lgdi32 -lwinmm
CC = gcc

all: $(EXE)

$(EXE): $(SOURCES)
	$(CC) $(SOURCES) -o $(EXE) $(LDFLAGS) $(LIBS) $(CFLAGS)

clean:
	rm -f $(EXE)


