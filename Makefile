TARGET = testprog
OFILES = testprog.o logging.o samples.o

$(TARGET): $(OFILES)

$(OFILES): saytimespan.h fillrand.inc.h

samples.c:
	./gensamples.c.sh

clean:
	rm -f *.o samples.c

check: $(TARGET)
	./$(TARGET)
