CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread
TARGET = wifihack
SOURCE = main.cpp

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

install:
	sudo cp $(TARGET) /usr/local/bin/

run: $(TARGET)
	sudo ./$(TARGET)

help:
	@echo "make       - скомпилировать программу"
	@echo "make clean - удалить скомпилированный файл"
	@echo "make run   - скомпилировать и запустить"
	@echo "make install - установить в систему"
