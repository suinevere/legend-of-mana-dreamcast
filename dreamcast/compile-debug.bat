@ECHO OFF
REM ----------------------
REM  | compile-debug.bat
REM  | Description: Builds the debug disc gdi-debug/, whose title menu shows the hidden debug start, inside the lom-dc Docker image, first bringing Ring Ring Land's VMU GAME up to date through compile-vmu.bat.
REM  | Author: suinevere
REM  | Dependencies: compile-vmu.bat, dockerfiles/dreamcast.Dockerfile, makefile (debug-gdi)
REM  | Globals: N/A
REM  | Params: N/A
REM  | Returns: make's exit code
REM ----------------------
SETLOCAL
CALL "%~dp0compile-vmu.bat"
IF ERRORLEVEL 1 GOTO done
docker run --rm -v "%~dp0..:/work" -w /work lom-dc bash -lc "make -C dreamcast debug-gdi"
:done
ENDLOCAL & EXIT /B %ERRORLEVEL%
