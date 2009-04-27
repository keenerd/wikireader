#
# "WikiReaderMakefile" - a Makefile for setting up Wiki Reader
#
# (C) Copyright 2008, 2009 OpenMoko, Inc.
# Author: xiangfu liu <xiangfu@openmoko.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 3 as published by the Free Software Foundation.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA

BOOTLOADER_TTY ?= $(shell echo -n /dev/ttyUSB && [ -e /dev/ttyUSB1 ] && echo 1 || echo 0)

# ----- Toolchain configuration data --------------------------------------

GCC_VERSION=3.3.2
GCC_PACKAGE=gcc-$(GCC_VERSION).tar.gz
GCC_URL=ftp://ftp.gnu.org/gnu/gcc/$(GCC_PACKAGE)

BINUTILS_VERSION=2.10.1
BINUTILS_PACKAGE=binutils-$(BINUTILS_VERSION).tar.gz
BINUTILS_URL= \
  ftp://ftp.gnu.org/gnu/binutils/$(BINUTILS_PACKAGE)

DL=./toolchain/dl
export PATH:=$(PWD)/install/bin:$(PATH)

# ----- configuration data --------------------------------------

.PHONY: all
all:    mini-libc \
	bootloader \
	toppers \
	kernel \
	simulator-qt4 \
	simulator-console

.PHONY: bootloader
bootloader:mini-libc fatfs
	$(MAKE) -C bootloader

.PHONY: bootloader232
bootloader232:mini-libc fatfs
	$(MAKE) -C bootloader bootloader232

.PHONY: toppers
toppers: mini-libc fatfs
	( cd jsp && \
	$(MAKE) -C cfg && \
	$(MAKE) -C wikireader && \
	cp wikireader/sample1.elf ../KERNEL.toppers)

.PHONY: kernel
kernel: mini-libc fatfs
	( cd kernel && \
	$(MAKE) && \
	cp mahatma.elf ../KERNEL)

.PHONY: mahatma
mahatma: mini-libc fatfs
	$(MAKE) -C kernel
	cp -p kernel/mahatma.elf KERNEL

# ----- lib stuff   -------------------------------------------
.PHONY:mini-libc
mini-libc: gcc
	$(MAKE) -C toolchain/mini-libc/

.PHONY: fatfs

fatfs: mini-libc
	$(MAKE) -C fatfs/

# ----- toolchain stuff  --------------------------------------
gcc-download:
	mkdir -p $(DL)
	wget -c -O $(DL)/$(GCC_PACKAGE) $(GCC_URL)
	touch $@

binutils-download:
	mkdir -p $(DL)
	wget -c -O $(DL)/$(BINUTILS_PACKAGE) $(BINUTILS_URL)
	touch $@

binutils-patch: binutils-download
	mkdir -p install
	rm -rf toolchain/binutils-$(BINUTILS_PACKAGE)
	tar -xvzf $(DL)/$(BINUTILS_PACKAGE) -C toolchain/
	( cd toolchain && \
	cd binutils-$(BINUTILS_VERSION) && \
	cat ../patches/0001-binutils-EPSON-changes-to-binutils.patch | patch -p1 && \
	cat ../patches/0002-binutils-EPSON-make-it-compile-hack-for-recent-gcc.patch | patch -p1)
	touch $@

binutils: binutils-patch
	(cd toolchain && \
	cd binutils-$(BINUTILS_VERSION) && \
	mkdir -p build && \
	cd build  && \
	CPPFLAGS="-D_FORTIFY_SOURCE=0" ../configure --prefix $(PWD)/install --target=c33-epson-elf && \
	CPPFLAGS="-D_FORTIFY_SOURCE=0" $(MAKE) && \
	$(MAKE) install)
	touch $@

gcc-patch: gcc-download
	mkdir -p install
	tar -xvzf $(DL)/$(GCC_PACKAGE) -C toolchain/
	( cd toolchain && \
	cd gcc-$(GCC_VERSION) && \
	cat ../patches/0001-gcc-EPSON-modified-sources.patch | patch -p1 && \
	cat ../patches/0002-gcc-Force-that-the-assembly-of-libgcc-complies-wit.patch | patch -p1 && \
	cat ../patches/0003-gcc-Use-the-C-implementations-for-division-and-mod.patch | patch -p1)
	touch $@

gcc: binutils gcc-patch
	( cd toolchain && \
	export PATH=$(PWD)/install/bin:$(PATH) && \
	cd gcc-$(GCC_VERSION) && \
	mkdir -p build && \
	cd build && \
	CPPFLAGS="-D_FORTIFY_SOURCE=0" ../configure --prefix $(PWD)/install --target=c33-epson-elf --enable-languages=c && \
	CPPFLAGS="-D_FORTIFY_SOURCE=0" $(MAKE) && \
	$(MAKE) install)
	touch $@

.PHONY: simulator-qt4
simulator-qt4:
	( cd host-tools/simulator/Qt4/WikiSim && qmake-qt4 && $(MAKE) )

.PHONY: simulator-console
simulator-console:
	( cd host-tools/simulator/console && $(MAKE) )

# ----- wiki Dump and Algorithm  --------------------------------------
.PHONY: getwikidump
getwikidump:
	wget http://download.wikimedia.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2

.PHONY: webkit
webkit:
	svn co -r 41057 http://svn.webkit.org/repository/webkit/trunk webkit
	(cd webkit && \
	patch_path="../host-tools/rendering/patches/"; for file in `ls $$patch_path`; do echo "processing file: $$file"; patch -p1 < $$patch_path/$$file; done && \
	./WebKitTools/Scripts/build-webkit --gtk --release)

.PHONY: flash-bootloader
flash-bootloader: fatfs
	$(MAKE) -C bootloader BOOTLOADER_TTY="${BOOTLOADER_TTY}" flash-bootloader

.PHONY: print-flash-config
print-flash-config:
	@echo BOOTLOADER_TTY = "${BOOTLOADER_TTY}"

# ----- forth -----------------------------------------------
# items for testing

.PHONY: forth
forth:  gcc mini-libc
	$(MAKE) -C forth

.PHONY: mbr
mbr: gcc fatfs
	$(MAKE) -C $@

.PHONY: mbr-rs232
mbr-rs232: gcc fatfs
	$(MAKE) -C mbr mbr-rs232

.PHONY: flash-mbr
flash-mbr: mbr
	$(MAKE) -C bootloader rs232 e07load/e07load
	$(MAKE) -C mbr BOOTLOADER_TTY="${BOOTLOADER_TTY}" $@

# ----- clean and help --------------------------------------
.PHONY: complete-clean
complete-clean: clean clean-toolchain

.PHONY: clean
clean: clean-sim-qt4 clean-sim-console
	$(MAKE) clean -C bootloader
	$(MAKE) clean -C toolchain/mini-libc
	$(MAKE) clean -C fatfs
	$(MAKE) clean -C mbr
	$(MAKE) clean -C forth
	$(MAKE) clean -C kernel
	cd jsp && $(MAKE) clean -C wikireader

.PHONY: clean-toolchain
clean-toolchain:
	rm -rf toolchain/gcc-$(GCC_VERSION)
	rm -rf toolchain/binutils-$(BINUTILS_VERSION)
	rm -f binutils-download binutils-patch binutils
	rm -f gcc-download gcc-patch gcc

.PHONY: clean-sim-qt4
clean-sim-qt4:
	(cd host-tools/simulator/Qt4/WikiSim; $(MAKE) distclean || true)

.PHONY: clean-sim-console
clean-sim-console:
	$(MAKE) clean -C host-tools/simulator/console

.PHONY:help
help:
	@echo -e "\n\
all:			compile all the source.\n\
setup:			get all the source we need.\n\
bootloader:		compile bootloader.\n\
toppers:		compile a toppers kernel.\n\
binutils: 		compile binutils.\n\
gcc:			compile gcc.\n\
mini-libc:			compile mini-libc (libc.a).\n\
flash-bootloader: 	flash bootloader to you E07 board\n\
				-make sure the serial console is /dev/ttyUSB0.\n\
simulator-qt4		compile the Qt4 simulator\n\
simulator-console	compile the console simulator\n\
clean: 			clean all.\n\
				openmoko, Inc.\n "

.PHONY:testhelp
testhelp:
	$(MAKE) --print-data-base --question |	\
	awk '/^[^.%][-A-Za-z0-9_]*:/		\
		{ print substr($$1, 1, length($$1)-1) }' | 	\
	sort |	\
	pr --omit-pagination --width=80 --columns=1
