CC            := gcc
RM            := rm
CFLAGS        += -fomit-frame-pointer -O3
LDFLAGS       += -lJudy

all: chasm

chasm: chasm.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	$(RM) chasm
