all_files = lab1b-404844706.tar.gz lab1b-server.c lab1b-client.c README.md Makefile

default: l1s l1c

l1s: 
	gcc  -Wall -Wextra  -o lab1b-server -g lab1b-server.c -lmcrypt
l1c: 
	gcc -Wall -Wextra  -o lab1b-client -g lab1b-client.c -lmcrypt
clean:
	rm -f lab1b-client lab1b-server lab1b-404844706.tar.gz

dist:
		tar -czvf $(all_files)