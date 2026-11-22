all:
	$(CC) $(CFLAGS) -Wall -Wextra -o guess_client guess_client.c $(LDFLAGS)

romfs:
	$(ROMFSINST) /usr/bin/guess_client
	$(ROMFSINST) -p +x /sbin/guess_game_init.sh

clean:
	rm -f guess_client
