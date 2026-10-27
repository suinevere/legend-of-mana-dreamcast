@ECHO OFF
REM ----------------------
REM  | compile.bat
REM  | Description: Builds the Legend of Mana Dreamcast port inside the lom-dc Docker image, first bringing Ring Ring Land's VMU GAME up to date through compile-vmu.bat, since the disc carries it.
REM  | Author: suinevere
REM  | Dependencies: compile-vmu.bat, dockerfiles/dreamcast.Dockerfile, makefile
REM  | Globals: N/A
REM  | Params: %1 -- "clean" to clean, "pending" to regenerate pending stubs, "test-image" to build the scripted lom_test.cdi, otherwise ("gdi" too) builds lom.elf, the GDEMU folder gdi/ and lom.cdi, rebuilding both discs when either is missing
REM  | Returns: make's exit code
REM ----------------------
SETLOCAL
SET "TARGET=all"
IF /I "%~1"=="clean" SET "TARGET=clean"
IF /I "%~1"=="pending" SET "TARGET=pending"
IF /I "%~1"=="gdi" SET "TARGET=gdi"
IF /I "%~1"=="test-image" SET "TARGET=test-image"
IF "%TARGET%"=="clean" GOTO build
IF "%TARGET%"=="pending" GOTO build
CALL "%~dp0compile-vmu.bat"
IF ERRORLEVEL 1 GOTO done
:build
docker run --rm -v "%~dp0..:/work" -w /work lom-dc bash -lc "make -C dreamcast %TARGET%"
:done
ENDLOCAL & EXIT /B %ERRORLEVEL%
