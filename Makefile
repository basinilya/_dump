TARGET = testprog
OFILES = testprog.o logging.o

$(TARGET): $(OFILES)

clean:
	rm -f *.o

check: $(TARGET)
	./$(TARGET)
