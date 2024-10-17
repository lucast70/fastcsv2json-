all:
	g++ -Wall -Werror -std=c++20 -fomit-frame-pointer -O3 fastcsv2jsonxx.cpp -o fastcsv2jsonxx

clean:
	rm -rf fastcsv2jsonxx

install:
	cp fastcsv2jsonxx /usr/bin		
	
	