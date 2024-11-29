cc=g++
cflags=-g

make: cryptominer_server xmrig_miner_handler

cryptominer_server: cryptominer_server.o cryptominer_server_functions.o
	${cc} $^ -o $@ -L/usr/lib -L/usr/local/lib -lhttpserver simdjson.cpp

xmrig_miner_handler: xmrig_miner_handler.o
	${cc} $< -o $@

%.o : %.cpp
	${cc} ${cflags} -c $<

clean:
	rm cryptominer_server cryptominer_server.o xmrig_miner_handler xmrig_miner_handler.o cryptominer_server_functions.o
