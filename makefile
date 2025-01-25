CC=gcc
FLAGS=-Wall -Wextra

ifdef DEBUG
	FLAGS+=-ggdb
else
	FLAGS+=-O3
endif


SRCDIR := src
SRC := $(shell find $(SRCDIR) -name "*.c")

BUILDDIR := build
_OBJ := $(SRC:%.c=%.o)
OBJ := $(subst $(SRCDIR),$(BUILDDIR),$(_OBJ))

INCDIR := include
INC := $(shell find $(INCDIR) -name "*.h")

OUTNAME := cac

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(INC) | $(BUILDDIR)
	$(CC) -o $@ -c $< $(FLAGS) -I $(INCDIR)


$(OUTNAME): $(OBJ)
	$(CC) -o $(BUILDDIR)/$@ $^ -I $(INCDIR) $(LDLIBS) $(FLAGS)


$(BUILDDIR):
	mkdir $@


.PHONY: clean run

run: | $(OUTNAME)
	./build/$(OUTNAME)

clean:
	rm -r $(BUILDDIR)
