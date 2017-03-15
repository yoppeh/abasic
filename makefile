
cc=gcc
ld=gcc
sc=strip
cflags=-O2 -g0
lflags=-O2 -g0 -lc -lm

obj=main.o io.o parse.o prog.o scan.o util.o var.o

abasic : $(obj)
	$(ld) -o $@ $(obj) $(lflags)
	$(sc) $@

main.o : io.h parse.h prog.h scan.h util.h var.h
io.o : io.h util.h
parse.o : io.h parse.h prog.h scan.h util.h var.h
prog.o : prog.h var.h
scan.o : scan.h
util.o : util.h
var.o : var.h

%.o : %.c
	$(cc) $(cflags) -c $<

clean:
	if [ -f abasic ] ; then rm abasic ; fi
	if ls *.o 1> /dev/null 2>&1 ; then rm *.o ; fi
