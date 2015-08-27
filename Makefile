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

target = gnumeric-plugin-pwhtml_1.2
dist: clean
	rm -rf $(target)
	mkdir $(target)
	cp -r pwhtml $(target)
	cp Makefile $(target)

	rm -f $(target).tar.gz
	tar cfzh $(target).orig.tar.gz $(target)
	rm -rf $(target)

debuild: dist
	rm -rf .debuild && mkdir .debuild
	cp -r debian .debuild
	cp -r pwhtml .debuild
	cp Makefile .debuild
	cd .debuild && debuild -S -sa

upload:
	dput ppa:cuu508/ppa $(target)-0ubuntu1_source.changes
	backportpackage -s vivid -d trusty -u ppa:cuu508/ppa $(target)-0ubuntu1.dsc

