# Simple Makefile for Project 1

srcdir  = .

CC      = gcc
EXE	= 
CFLAGS  = -m32

TARGETS = system$(EXE)

all: $(TARGETS)

system$(EXE): $(srcdir)/kernel.o $(srcdir)/drivers.o $(srcdir)/hardware.o
	$(CC) -o system$(EXE) $(CFLAGS) $(srcdir)/kernel.o  $(srcdir)/hardware.o $(srcdir)/drivers.o

