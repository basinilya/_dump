TARGET = testprog
OFILES = testprog.o logging.o samples.o saytimespan.o

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

CPPFLAGS = -DSAYTIMESPAN_SAMPLES=\"$(current_dir)/samples\"

$(TARGET): $(OFILES)

$(OFILES): saytimespan.h fillrand.inc.h mylogging.h

samples.c:
	./gensamples.c.sh

clean:
	rm -f *.o samples.c

check: $(TARGET)
	./$(TARGET)
