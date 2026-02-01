
.PHONY: all
all:
	cmake -E make_directory "build"
	cmake -E chdir "build" cmake ../
	cmake --build "build"

.PHONY: test
test:
	$(MAKE) test -C build

.PHONY: clean
clean:
	rm -rf ./build

.PHONY: fmt
fmt:
	@find ./libv/* -iname "*.h" -o -iname "*.c" | xargs clang-format -i

