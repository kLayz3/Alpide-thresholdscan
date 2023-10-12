-include Makefile.local
ifndef DRASI_DIR
DRASI_DIR=$(HOME)/mlAlpideDevelopment/bl/drasi
endif

ifndef ALPIDE_DIR
ALPIDE_DIR=$(HOME)/mlAlpideDevelopmentbl/alpide_software
endif

SRC_DIR:=src
INC_DIR:=includes
BUILD_DIR:=build

CC = g++ -std=c++17

CFLAGS:=-c -g -Wall -O3 \
		-I$(INC_DIR) -I. \
		-I$(ALPIDE_DIR)/framework/src/mosaic \
        -I$(ALPIDE_DIR)/framework/src/manager \
        -I$(ALPIDE_DIR)/framework/src/common \
		 $(shell root-config --cflags) \
		 $(shell root-config --auxcflags)

LDFLAGS:=$(shell root-config --ldflags)
LIBS:=$(ALPIDE_DIR)/framework/lib/libMANAGER.a \
	$(ALPIDE_DIR)/framework/lib/libMOSAIC.a \
	$(ALPIDE_DIR)/framework/lib/libCOMMON.a -lusb-1.0 \
	$(shell root-config --libs)


SRC:=$(SRC_DIR)/thresholdscan.cc
INC=$(wildcard $(INC_DIR)/*.cxx)
OBJ:=$(patsubst $(SRC_DIR)/%.cc,  $(BUILD_DIR)/%.o,   $(SRC)) \
	 $(patsubst $(INC_DIR)/%.cxx, $(BUILD_DIR)/%.oxx, $(INC))

TARGET:=$(patsubst $(SRC_DIR)/%.cc, %, $(SRC))

MKDIR=mkdir -p $(@D)


.PHONY: all
all: $(TARGET)

$(TARGET) : $(OBJ)
	$(CC) $(LDFLAGS) $(BUILD_DIR)/$@.o $(BUILD_DIR)/*.oxx -o $@ $(LIBS)

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.cc
	$(MKDIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.oxx : $(INC_DIR)/%.cxx
	$(MKDIR)
	$(CC) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) *~
	rm -f $(TARGET)
