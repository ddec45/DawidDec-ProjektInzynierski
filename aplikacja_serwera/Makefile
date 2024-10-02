cc=g++
cflags=-g

https_server: https_server.o
	${cc} $< -o $@ -L/usr/lib -L/usr/local/lib -lhttpserver

%.o : %.cpp %.hpp
	${cc} ${cflags} -c $<

clean:
	rm https_server https_server.o
