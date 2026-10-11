@ECHO OFF
REM ----------------------
REM  | compile.bat
REM  | Description: Builds the Legend of Mana Dreamcast port inside the lom-dc Docker image.
REM  | Author: suinevere
REM  | Dependencies: dockerfiles/dreamcast.Dockerfile, makefile
REM  | Globals: N/A
REM  | Params: %1 -- "clean" to clean, "pending" to regenerate pending stubs, "test-image" to build the scripted lom_test.cdi, otherwise builds
REM  | Returns: make's exit code
REM ----------------------
SETLOCAL
SET "TARGET=all"
IF /I "%~1"=="clean" SET "TARGET=clean"
IF /I "%~1"=="pending" SET "TARGET=pending"
IF /I "%~1"=="test-image" SET "TARGET=test-image"
docker run --rm -v "%~dp0..:/work" -w /work lom-dc bash -lc "make -C dreamcast %TARGET%"
ENDLOCAL & EXIT /B %ERRORLEVEL%
