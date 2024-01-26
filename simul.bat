@echo off
SET /A "count=0"

:loop
IF %count%==100 (
    GOTO end
)

START curl http://localhost:8080/
SET /A "count+=1"
GOTO loop

:end
ECHO Done!
PAUSE
