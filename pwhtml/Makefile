GNM_DIR=/home/cepe/Downloads/gnumeric-1.12.18
GNM_SRC_DIR=$(GNM_DIR)/src

GODEPS=libspreadsheet-1.12
INCLUDES = `pkg-config --cflags $(GODEPS)`
LIBS = `pkg-config --libs $(GODEPS)`

CFLAGS=$(INCLUDES) -I$(GNM_DIR)

%.o: %.c
	gcc $(CFLAGS) -fPIC -o $@ -c $<

pwhtml.so: boot.o html.o font.o
	gcc $(LIBS) -shared -o $@ $^

