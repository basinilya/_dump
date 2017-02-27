TARGET = testprog
OFILES = testprog.o logging.o samples.o saytimespan.o

CPPFLAGS = -DSAYTIMESPAN_SAMPLES=\"/home/il/saytimespan/samples\"

$(TARGET): $(OFILES)

$(OFILES): saytimespan.h fillrand.inc.h mylogging.h

samples.c:
	./gensamples.c.sh

clean:
	rm -f *.o samples.c

check: $(TARGET)
	./$(TARGET)
