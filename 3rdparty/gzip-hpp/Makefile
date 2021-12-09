# Whether to turn compiler warnings into errors
export WERROR ?= true

default: release

release:
	mkdir -p build && cd build && cmake ../ -DCMAKE_BUILD_TYPE=Release -DWERROR=$(WERROR) && VERBOSE=1 cmake --build .

debug:
	mkdir -p build && cd build && cmake ../ -DCMAKE_BUILD_TYPE=Debug -DWERROR=$(WERROR) && VERBOSE=1 cmake --build .

test:
	@if [ -f ./build/unit-tests ]; then ./build/unit-tests; else echo "Please run 'make release' or 'make debug' first" && exit 1; fi

bench:
	@if [ -f ./build/bench-tests ]; then ./build/bench-tests; else echo "Please run 'make release' or 'make debug' first" && exit 1; fi

coverage:
	./scripts/coverage.sh

tidy:
	./scripts/clang-tidy.sh

format:
	./scripts/format.sh

clean:
	rm -rf ./build
	# remove remains from running 'make coverage'
	rm -f *.profraw
	rm -f *.profdata
	@echo "run 'make distclean' to also clear mason_packages, .mason, and .toolchain directories"

distclean: clean
	rm -rf mason_packages
	# remove remains from running './scripts/setup.sh'
	rm -rf .mason
	rm -rf .toolchain
	rm -f local.env

.PHONY: test bench

# TODO: once gzip has its own repo, consider adding `-Wold-style-cast` as a debug flag and moving to C-style casting to static_cast
# Not sure where this should live, now that flags are no longer in the Makefile. Perhaps in setup.sh?