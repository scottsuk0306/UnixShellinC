# Macros
CC = gcc209
CCFLAGS = -D_GNU_SOURCE

# Pattern rule
%.o: %.c
	$(CC) $(CCFLAGS) -c $<

# Dependency rules for non-file targets
all: ish
clobber: clean
	rm -f *~ \#*\# core
clean:
	rm -f ish *.o
	rm -f execute *.o
	rm -f builtin *.o
	rm -f parser *.o
	rm -f dynarray *.o


# Dependency rules for file targets
ish: ish.o builtin.o execute.o parser.o dynarray.o
	$(CC) $(CCFLAGS) $? -o $@

ish.o: execute.h parser.h builtin.h dynarray.h
execute.o: execute.h parser.h builtin.h dynarray.h
builtin.o: builtin.h
parser.o: parser.h dynarray.h
dynarray.o: dynarray.h

