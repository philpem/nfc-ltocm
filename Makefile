nfc-ltocm:	nfc-ltocm.o nfc-utils.o
	$(CC) -o $@ $^ -lnfc
