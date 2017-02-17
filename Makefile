TARGET = testprog
OFILES = testprog.o logging.o samples.o

$(TARGET): $(OFILES)

samples.c:
	./gensamples.c.sh

clean:
	rm -f *.o samples.c

check: $(TARGET)
	./$(TARGET)
