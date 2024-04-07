all: master worker

master: master.cpp
	g++ -o master master.cpp

worker: worker.cpp
	g++ -o worker worker.cpp

clean:
	rm -f master worker

