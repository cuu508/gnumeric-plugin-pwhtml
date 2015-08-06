GNM_DIR=/home/vagrant/gnumeric-1.12.9

GODEPS=libspreadsheet
INCLUDES = `PKG_CONFIG_PATH=$(GNM_DIR) pkg-config --cflags $(GODEPS)`
LIBS = `PKG_CONFIG_PATH=$(GNM_DIR) pkg-config --libs $(GODEPS)`

CFLAGS=$(INCLUDES) -I$(GNM_DIR) -I$(GNM_DIR)/src -Wall

compile: pwhtml/boot.o pwhtml/html.o pwhtml/font.o
	gcc $(LIBS) -shared -o pwhtml/pwhtml.so $^

%.o: %.c
	gcc $(CFLAGS) -fPIC -o $@ -c $<

clean:
	rm pwhtml/*.o
	rm pwhtml/*.so

install:
	cp -r pwhtml /usr/lib/gnumeric/1.12.18/plugins/

dist = gnumeric-plugin-pwhtml-trusty64
binary: compile
	rm -rf $(dist)
	mkdir $(dist)
	mkdir $(dist)/pwhtml
	cp pwhtml/pwhtml.so $(dist)/pwhtml/
	cp pwhtml/plugin.xml $(dist)/pwhtml/
	rm -f $(dist).tar.gz
	tar cfzh $(dist).tar.gz $(dist)
	rm -rf $(dist)

