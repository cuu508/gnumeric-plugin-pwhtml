GODEPS=libspreadsheet-1.12
INCLUDES = `pkg-config --cflags $(GODEPS)`
LIBS = `pkg-config --libs $(GODEPS)`

CFLAGS=$(INCLUDES) -Wall

compile: pwhtml/boot.o pwhtml/html.o pwhtml/font.o
	gcc $(LIBS) -O3 -g -shared -o pwhtml/pwhtml.so $^

%.o: %.c
	gcc $(CFLAGS) -fPIC -o $@ -c $<

clean:
	rm -f pwhtml/*.o
	rm -f pwhtml/*.so

install:
	mkdir -p $(DESTDIR)/usr/lib/gnumeric/1.12.18/plugins/pwhtml
	cp  pwhtml/plugin.xml $(DESTDIR)/usr/lib/gnumeric/1.12.18/plugins/pwhtml
	cp  pwhtml/pwhtml.so $(DESTDIR)/usr/lib/gnumeric/1.12.18/plugins/pwhtml

target = gnumeric-plugin-pwhtml-1.0
dist: clean
	rm -rf $(target)
	mkdir $(target)
	cp -r pwhtml $(target)
	cp Makefile $(target)

	rm -f $(target).tar.gz
	tar cfzh $(target).tar.gz $(target)
	rm -rf $(target)

