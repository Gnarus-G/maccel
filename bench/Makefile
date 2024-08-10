bench_driver: build
	bin/evdriverlag

bench_handler: build
	bin/evinputlag

build: evdriverlag.o evinputlag.o
	mkdir -p bin
	cc evdriverlag.o -o bin/evdriverlag
	cc evinputlag.o -o bin/evinputlag

evdriverlag.o: evdriverlag.c
	cc -c evdriverlag.c

evinputlag.o: evinputlag.c
	cc -c evinputlag.c

clean:
	rm *.o
