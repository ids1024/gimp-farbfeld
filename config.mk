INCLUDES := $(shell pkg-config --cflags gimp-2.0)
LIBS := $(shell pkg-config --libs gimp-2.0) -lbz2

GIMP_VERSION := $(basename $(shell pkg-config --modversion gimp-2.0))
INSTALL_DIR = ${HOME}/.gimp-${GIMP_VERSION}/plug-ins
