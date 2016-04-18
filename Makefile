#
# Global Makefile
#

# the sets of directories to do various things in
DIRS = 	examples/bitmapsTest examples/bitmapTest examples/blur_test examples/celltest examples/celltest256 
	examples/celltest256-2x1 examples/compressCube examples/cpu_frt examples/cube examples/flags 
	examples/gouraud examples/hblTest examples/liblangam examples/mesh examples/mic3d examples/parallax 
	examples/rasters examples/reflection examples/rotation examples/schmup examples/scrollfn examples/scu-dma 
	examples/sequencer examples/vdp1ex examples/vdp2-colorcalc examples/vdp2-colorcalc256 examples/zooming 
LIBDIRS = libs/liblangam

BUILDDIRS = $(DIRS:%=build-%)
INSTALLDIRS = $(LIBDIRS:%=install-%)
CLEANDIRS = $(DIRS:%=clean-%) $(LIBDIRS:%=clean-%)

all: $(BUILDDIRS)
$(DIRS): $(BUILDDIRS)
$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

# the utils need the libraries in dev built first
install: $(LIBDIRS) all
$(LIBDIRS):
	$(MAKE) -C $(@:install-%=%) release-install
	
clean: $(CLEANDIRS)
$(CLEANDIRS): 
	$(MAKE) -C $(@:clean-%=%) clean


.PHONY: subdirs $(DIRS)
.PHONY: subdirs $(BUILDDIRS)
.PHONY: subdirs $(CLEANDIRS)
.PHONY: all clean
