.PHONY: all clean distclean install uninstall package

all: build
	$(MAKE) -C build

clean: build
	$(MAKE) -C build clean

distclean:
	rm -rf build/

install: build
	$(MAKE) -C build install

uninstall: build
	@if [ -f build/install_manifest.txt ]; then \
	echo 'Uninstalling' ;                       \
	xargs rm < build/install_manifest.txt ;     \
	else 					    \
	echo 'VobSub2Srt does not seem to be installed.'; \
	fi

package: build
	$(MAKE) -C build package

build:
	@echo "Please run ./configure (with appropriate parameters)!"
	@exit 1
