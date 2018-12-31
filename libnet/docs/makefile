all: html text info texinfo clean

html: build
	rm -f libnet.html
	cp -f src/libnet.html .

text: build
	rm -f libnet.txt
	cp -f src/libnet.txt .

info: build
	rm -f libnet.info
	cp -f src/libnet.info .

texinfo: build
	rm -f libnet.txi
	cp -f src/libnet.txi .

build:
	$(MAKE) -C src

clean:
	$(MAKE) -C src veryclean

veryclean: clean
	rm -f libnet*.*

