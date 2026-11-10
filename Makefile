CXX = g++
WINDRES = windres

CXXFLAGS = -std=c++17 -O2 -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -Iinclude -static -static-libgcc -static-libstdc++
LDFLAGS = -liphlpapi -lws2_32 -lgdi32 -luser32 -lcomctl32 -lcomdlg32 -lshell32 -ladvapi32 -ldwmapi -luxtheme

SRCS = src/main.cpp \
       src/network/NetworkMonitor.cpp \
       src/network/SpeedCalculator.cpp \
       src/settings/SettingsManager.cpp \
       src/ui/TaskbarWidget.cpp \
       src/ui/TrayIcon.cpp \
       src/ui/DashboardWindow.cpp \
       src/ui/SettingsWindow.cpp

TEST_SRCS = tests/TestRunner.cpp \
            src/network/SpeedCalculator.cpp \
            src/settings/SettingsManager.cpp

all: dirs bin/NetSpeed.exe bin/NetSpeedTests.exe

dirs:
	mkdir -p build bin

build/resource.o: resources/resource.rc resources/app.manifest
	$(WINDRES) -I resources resources/resource.rc -O coff -o build/resource.o

bin/NetSpeed.exe: $(SRCS) build/resource.o
	$(CXX) $(CXXFLAGS) -mwindows -municode $(SRCS) build/resource.o $(LDFLAGS) -o bin/NetSpeed.exe

bin/NetSpeedTests.exe: $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) -mconsole $(TEST_SRCS) -liphlpapi -lws2_32 -luser32 -ladvapi32 -lshell32 -o bin/NetSpeedTests.exe

test: bin/NetSpeedTests.exe
	./bin/NetSpeedTests.exe

clean:
	rm -rf build bin

.PHONY: all test clean dirs
