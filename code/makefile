test3:BaseCode.o test3.o MyThread.o
	g++ BaseCode.o test3.o MyThread.o -o test3 -L /usr/include/boost/thread -lpthread
test3.o: test3.cpp BaseCode.h MyThread.h
	g++ -c test3.cpp 
BaseCode.o: BaseCode.cpp BaseCode.h BaseType.h MyTLS.h
	g++ -c BaseCode.cpp 
MyThread.o: MyThread.cpp MyThread.h  BaseCode.h BaseType.h
	g++ -c MyThread.cpp 
.PHONY: clean
clean:
	rm *.o