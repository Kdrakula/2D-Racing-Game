# Wrapper Makefile to automate CMake build and execution

.PHONY: build run clean test

# Default target: Creates build dir, runs CMake, and compiles
build:
	@mkdir -p build
	@cd build && cmake .. && make
	@xattr -cr build/MyRacingGame.app/Contents/MacOS/MyRacingGame # todo fix

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

# Build and run the doctest unit tests
test:
	@mkdir -p build
	@cd build && cmake .. && make run_tests
	@./build/run_tests