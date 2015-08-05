GNM_DIR=/home/cepe/Downloads/gnumeric-1.12.18
GNM_SRC_DIR=$(GNM_DIR)/src

GODEPS=libspreadsheet-1.12
INCLUDES = `pkg-config --cflags $(GODEPS)`
LIBS = `pkg-config --libs $(GODEPS)`

CFLAGS=$(INCLUDES)

compile: pwhtml/boot.o pwhtml/html.o pwhtml/font.o
	gcc $(LIBS) -shared -o pwhtml/pwhtml.so $^

%.o: %.c
	gcc $(CFLAGS) -fPIC -o $@ -c $<


