#
# In order to execute this "Makefile" just type "make"
#

OBJS 	= main.o handlersAndHelpers.o
SOURCE	= main.c handlersAndHelpers.c
HEADER  = handlersAndHelpers.h
OUT  	= myexe
CC	= gcc
FLAGS   = -std=gnu11 -c
# -g option enables debugging mode
# -c flag generates object code for separate files

$(OUT): $(OBJS)
	$(CC) $(OBJS) -o $@

# create/compile the individual files >>separately<<
main.o: main.c
	$(CC)  $(FLAGS) main.c
handlersAndHelpers.o: handlersAndHelpers.c
	$(CC)  $(FLAGS) handlersAndHelpers.c

# clean house
clean:
	rm -f $(OBJS) $(OUT)

# do a bit of accounting
count:
	wc $(SOURCE) $(HEADER)