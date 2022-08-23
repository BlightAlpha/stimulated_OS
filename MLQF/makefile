final: simos.exe

simos.exe: system.c process.o term.o loader.o paging.o cpu.o\
			   clock.o memory.o idle.o swap.o admin.o submit.c -lpthread -lm
	gcc -o simos.exe system.c process.o term.o loader.o paging.o cpu.o\
	 			 clock.o memory.o idle.o swap.o admin.o submit.c -lpthread -lm 

admin.o: admin.c simos.h
	gcc -g -c admin.c -std=c99 -lm

clock.o: clock.c simos.h
	gcc -g -c clock.c -std=c99 -lm

cpu.o: cpu.c simos.h
	gcc -g -c cpu.c -std=c99 -lm

memory.o: memory.c simos.h
	gcc -g -c memory.c -std=c99 -lm


system.o: system.c simos.h
	gcc -g -c system.c -std=c99 -lm

paging.o: paging.c simos.h
	gcc -g -c paging.c -std=c99 -lm

process.o: process.c simos.h
	gcc -g -c process.c -std=c99 -lm

submit.o: submit.c simos.h
	gcc -g -c submit.c -std=c99 -lm


clean:
	rm *.o simos.exe swap.disk terminal.out