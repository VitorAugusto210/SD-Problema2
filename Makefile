all: programa_final

programa_final: menu.o api_fpga.o
	gcc -o programa_final menu.o api_fpga.o

menu.o: menu.c constantes.h
	gcc -std=c99 -c -o menu.o menu.c

api_fpga.o: api_fpga.pp.s
	as -o api_fpga.o api_fpga.pp.s

api_fpga.pp.s: api_fpga.s constantes.h
	gcc -E -x assembler-with-cpp -o api_fpga.pp.s api_fpga.s

clean:
	rm -f programa_final menu.o api_fpga.o api_fpga.pp.s

.PHONY: all clean
