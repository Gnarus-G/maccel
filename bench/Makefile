bench_driver: build
	@bin/evdriverlag

bench_handler: build
	@bin/evinputlag

build: evdriverlag.c evinputlag.c
	@mkdir -p bin
	@cc evdriverlag.c -o bin/evdriverlag
	@cc evinputlag.c -o bin/evinputlag

clean:
	rm *.o
