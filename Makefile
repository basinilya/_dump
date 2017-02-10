TARGET = testprog
OFILES = testprog.o

$(TARGET): $(OFILES)

clean:
	rm -f *.o

check: $(TARGET)
	./$(TARGET)
