.PHONY: all clean distclean install uninstall package dput documentation

all: build | documentation
	$(MAKE) -C build

clean: build
	$(MAKE) -C build clean

distclean:
	rm -rf build/

documentation: build
	$(MAKE) -C build documentation

install: build | documentation
	$(MAKE) -C build install

uninstall: build
	$(MAKE) -C build uninstall

package: build | documentation
	$(MAKE) -C build package

dput:
	@if [ -d build/Debian ]; then \
		git dch --snapshot --since=465fa781b93e66cfb7080c55880969cb4631d584; \
		$(MAKE) -C build dput; \
	else \
		echo 'Use ./configure -DENABLE_PPA=True to activate PPA support.'; \
	fi

build:
	@echo "Please run ./configure (with appropriate parameters)!"
	@exit 1
