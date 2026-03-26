# Wrapper Makefile to automate CMake build and execution

.PHONY: build run clean

# Default target: Creates build dir, runs CMake, and compiles
build:
	@mkdir -p build
	@cd build && cmake .. && make

# Build the game and then run it from the root directory
run: build
	@echo "Signing the application bundle..."
	codesign --force --deep --sign - ./build/MyRacingGame.app
	@echo "Starting the game..."
	./build/MyRacingGame.app/Contents/MacOS/MyRacingGame

# Completely remove the build folder for a fresh start
clean:
	rm -rf build
	@echo "Build directory removed."