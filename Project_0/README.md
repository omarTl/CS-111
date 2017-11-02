NAME: Omar Tleimat
EMAIL: otleimat@ucla.edu
ID: 404844706

I used the following sources:

https://www.gnu.org/software/libc/manual/html_node/Getopt.html

https://www.gnu.org/software/libc/manual/html_node/I_002fO-Overview.html

Used SO for confirming syntax on getopt_long
https://stackoverflow.com/questions/7489093/getopt-long-proper-way-to-use-it


Within the Makefile, I included tests to: 

1) make sure input files that did not have proper read permissions caused errors 
2) make sure program will exit with code 0 if everything was okay 
3) make sure no file found error was caught with a non existant input file 
4) made sure files matched at the end 