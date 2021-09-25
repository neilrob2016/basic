CC=cc -D NO_OS_SET
# The line drawing algorithm in drawLine() now uses integers by default instead
# of floats. Uncomment this to use the old floating point algorithm.
#LFA=-D LINE_FLOAT_ALGO

# Linux 
CC=cc $(LFA)
LIBS=-lm -lcrypt

# MacOS
CC=cc $(LFA)
LIBS=-lm 

# Helvar 950 Router
#CC=aarch64-poky-linux-gcc --sysroot=/opt/fsl-imx-wayland/4.19-warrior/sysroots/aarch64-poky-linux


# Old versions of Linux that only support DES in crypt()
#CC=cc $(LFA) -D DES_ONLY
#LIBS=-lm -lcrypt

# FreeBSD 6
#CC=cc $(LFA) -D DES_ONLY -D NO_LOG2
#LIBS=-lm -lcrypt
 
# For OS's where crypt just won't build
#CC=cc $(LFA) -D NO_CRYPT 
#LIBS=-lm

# -Wextra produces lots of extranious warnings but can be useful now and then
#ARGS=-Wall -Wextra -pedantic -c -g
ARGS=-Wall -pedantic -c -g
OBJS=\
	main.o \
	keyboard.o \
	tokeniser.o \
	execute.o \
	commands.o \
	functions.o \
	variables.o \
	expressions.o \
	defexp.o \
	defmod.o \
	values.o \
	program.o \
	disk.o \
	draw.o \
	watch.o \
	procs.o \
	argv.o \
	misc.o

basic: build_date $(OBJS) Makefile
	$(CC) $(OBJS) $(LIBS) -o basic

build_date:
	echo "#define BUILD_DATE \"`date +'%Y-%m-%d %T %Z'`\"" > build_date.h

main.o: main.c globals.h Makefile
	$(CC) $(ARGS) main.c

keyboard.o: keyboard.c globals.h Makefile
	$(CC) $(ARGS) keyboard.c

tokeniser.o: tokeniser.c globals.h Makefile
	$(CC) $(ARGS) tokeniser.c

program.o: program.c globals.h Makefile
	$(CC) $(ARGS) program.c

execute.o: execute.c globals.h Makefile
	$(CC) $(ARGS) execute.c

commands.o: commands.c globals.h Makefile
	$(CC) $(ARGS) commands.c

functions.o: functions.c globals.h Makefile
	$(CC) $(ARGS) functions.c

variables.o: variables.c globals.h Makefile
	$(CC) $(ARGS) variables.c

expressions.o: expressions.c globals.h Makefile
	$(CC) $(ARGS) expressions.c

defexp.o: defexp.c globals.h Makefile
	$(CC) $(ARGS) defexp.c

defmod.o: defmod.c globals.h Makefile
	$(CC) $(ARGS) defmod.c

values.o: values.c globals.h Makefile
	$(CC) $(ARGS) values.c

disk.o: disk.c globals.h Makefile
	$(CC) $(ARGS) disk.c

draw.o: draw.c globals.h Makefile
	$(CC) $(ARGS) draw.c

watch.o: watch.c globals.h Makefile
	$(CC) $(ARGS) watch.c 

procs.o: procs.c globals.h Makefile
	$(CC) $(ARGS) procs.c 

argv.o: argv.c globals.h Makefile
	$(CC) $(ARGS) argv.c 

misc.o: misc.c globals.h Makefile
	$(CC) $(ARGS) misc.c

clean:
	rm -f basic *.o core* build_date.h
