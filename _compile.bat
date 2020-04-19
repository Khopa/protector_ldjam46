@rem Compile protector
@rem You need CC65 compiler on path

@set CC65_HOME=..\cc65\
@if "%PATH%"=="%PATH:cc65=%" @PATH=%PATH%;%CC65_HOME%bin\

@ca65 crt0.s || goto fail

@cc65 -Oi protector.c --add-source || goto fail

@ca65 protector.s || goto fail

@ld65 -C nrom_128_horz.cfg -o protector.nes crt0.o protector.o runtime.lib || goto fail

protector.nes

@goto exit

:fail

pause

:exit

@del protector.s