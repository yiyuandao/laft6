#Makefile for general project
#**************************************************************
#Compiler
#**************************************************************
CC          = gcc

#**************************************************************
#Directories
#**************************************************************
ROOT        = .
SRCDIR := $(ROOT)/source
OBJ_DIR := $(ROOT)/obj
#APP_SRCDIR := $(ROOT)/app

#SRCDIR     += $(SYS_SRCDIR) $(APP_SRCDIR)
#SRCDIR     += $(SYS_SRCDIR)

VPATH       = $(SRCDIR)

#**************************************************************
#Compiler options
#**************************************************************

INCLUDE      = -I$(ROOT)/include
CC_OPT      = -g -c  -Wall -O2 $(INCLUDE)
LIBS		= pthread

#**************************************************************
#The name of executable file
#**************************************************************
PROGRAM    = tcmgrdisplay

#**************************************************************
#Files to be compiled
#**************************************************************
#SRC_C      := $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
SRC=$(wildcard ${SRCDIR}/*.c)

#**************************************************************
#All link files for the object
#**************************************************************
#OBJ_C      := $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJ=$(patsubst %.c, ${OBJ_DIR}/%.o, $(notdir ${SRC}))

#**************************************************************
#Rules for make
#**************************************************************
$(PROGRAM):$(OBJ)
	$(CC) -o $@ $^ $(addprefix -l,$(LIBS))

$(OBJ_DIR)/%.o:$(SRCDIR)/%.c
	$(CC) $(CC_OPT) -o $@ $<

.PHONY:clean
clean:
	rm -f $(OBJ_DIR)/*.o $(PROGRAM)
