@ECHO OFF
REM ----------------------
REM  | compile-vmu.bat
REM  | Description: Builds Ring Ring Land's VMU GAME (pocketstation/build/vmu_game/rrl.vms), first running the Dreamcast disc converter for the RES1506.BIN it reads; make skips both when they are up to date.
REM  | Author: suinevere
REM  | Dependencies: dockerfiles/dreamcast.Dockerfile (lom-dc), dockerfiles/pocketstation.dockerfile (lom-pocket), pre.makefile, pocketstation/src/vmu_game/Makefile
REM  | Globals: N/A
REM  | Params: N/A
REM  | Returns: the first failing make's exit code, else 0
REM ----------------------
SETLOCAL
docker run --rm -v "%~dp0..:/work" -w /work lom-dc bash -lc "make -C dreamcast gen/.pre_build.stamp"
IF ERRORLEVEL 1 GOTO done
docker run --rm -v "%~dp0..:/work" -w /work lom-pocket make -C pocketstation/src/vmu_game game
:done
ENDLOCAL & EXIT /B %ERRORLEVEL%
