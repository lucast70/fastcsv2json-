all:
	g++ -Wall -Werror -std=c++20 -fomit-frame-pointer -O3 fastcsv2json++.cpp -o fastcsv2json++

clean:
	rm -rf fastcsv2json++

install:
	cp fastcsv2json++ /usr/bin		
	
	