REM Automatically generated from Makefile

C:\dev\gameboy\gbdk\bin\lcc -c -o main.o main.c
C:\dev\gameboy\gbdk\bin\lcc -c -o font.o lib\font.c
C:\dev\gameboy\gbdk\bin\lcc -c -o utils.o lib\utils.c
C:\dev\gameboy\gbdk\bin\lcc -c -o game.o game.c
C:\dev\gameboy\gbdk\bin\lcc -c -o title.o title.c
C:\dev\gameboy\gbdk\bin\lcc -c -o gameover.o gameover.c
C:\dev\gameboy\gbdk\bin\lcc -o square_fall.gb main.o title.o game.o gameover.o font.o utils.o

rem C:\dev\gameboy\gbdk\bin\lcc  -Wa-l -Wf-bo2 -c -o game.o game.c
rem C:\dev\gameboy\gbdk\bin\lcc  -o square_fall.gb main.c 
