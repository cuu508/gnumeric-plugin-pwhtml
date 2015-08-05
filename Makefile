GNM_DIR=/home/vagrant/gnumeric-1.12.9

GODEPS=libspreadsheet
INCLUDES = `PKG_CONFIG_PATH=$(GNM_DIR) pkg-config --cflags $(GODEPS)`
LIBS = `PKG_CONFIG_PATH=$(GNM_DIR) pkg-config --libs $(GODEPS)`

CFLAGS=$(INCLUDES) -I$(GNM_DIR) -I$(GNM_DIR)/src

compile: pwhtml/boot.o pwhtml/html.o pwhtml/font.o
	gcc $(LIBS) -shared -o pwhtml/pwhtml.so $^

%.o: %.c
	gcc $(CFLAGS) -fPIC -o $@ -c $<


