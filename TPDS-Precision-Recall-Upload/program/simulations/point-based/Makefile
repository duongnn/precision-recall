CC=g++
FLAGS=-std=gnu++11 -g

SRC_DIR=src/
SRC_FILES= Process.cpp garg.cpp utility.cpp runsimulation.cpp main.cpp
SRC_FILES_FULL=$(addprefix $(SRC_DIR), $(SRC_FILES))
HDR_FILES= clock.h garg.h Process.h runsimulation.h utility.h
HDR_FILES_FULL=$(addprefix $(SRC_DIR), $(HDR_FILES))

all: wcp

wcp: $(HDR_FILES_FULL) $(SRC_FILES_FULL)
	$(CC) $(FLAGS) $(SRC_FILES_FULL) -o wcp

clean:
	rm wcp debug/wcp_debug*

