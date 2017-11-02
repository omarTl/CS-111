# NAME: Omar Tleimat
# EMAIL: otleimat@ucla.edu
# ID: 404844706

all_files = lab0-404844706.tar.gz Makefile lab0.c README.md

.SILENT:

default: 
	gcc -Wall -Wextra -g -o lab0 -g lab0.c

clean:
	rm -f lab0 lab0-404844706.tar.gz

check: default tests

dist:
	tar -czvf $(all_files)

input_file_doesnt_exist:
	echo "Testing that nonexistent input file throws error..."
	touch imnotafile
	rm imnotafile
	# Redirect expected error to dev/null
	./lab0 -i imnotafile 2> /dev/null; \
	code=$$?; \
	if [ $$code -ne 2 ]; then \
		echo "Non existent file test failed"; \
	else \
		echo "Passed"; \
	fi; \

files_match:
	echo "Testing that provided input file correctly copies to output file..."
	echo "test text" > test_input.txt; \
	./lab0 -i test_input.txt -o test_out.txt; \
	if ! cmp test_input.txt test_out.txt; then \
		echo "ERROR: Output doesnt match input"; \
	else \
		echo "Passed"; \
	fi; \
	rm -f test_input.txt test_out.txt

normal_run:
	echo "Testing that program finishes with successful exit code..."
	echo "meow" > test_input.txt; \
	./lab0 -i test_input.txt -o test_out.txt; \
	code=$$?; \
	if [ $$code -ne 0 ]; then \
		echo "Program didnt execute successfully"; \
	else \
		echo "Passed"; \
	fi; \
	rm -f test_input.txt test_out.txt;

no_read_permission: 
	echo "testing input file with no read permission.."
	echo "sorry!" > cant_read.txt; \
	chmod -r cant_read.txt; \
	./lab0 -i cant_read.txt 2> /dev/null; \
	code=$$?; \
	if [ $$code -ne 2 ]; then \
		echo "Program didnt catch read permission"; \
	else \
		echo "Passed"; \
	fi; \
	rm cant_read.txt


tests: input_file_doesnt_exist files_match normal_run no_read_permission