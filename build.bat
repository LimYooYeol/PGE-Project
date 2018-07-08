@echo off

SET NoPause=0
SET BuildArgs=
SET MAKE_EXTRA_ARGS=-j 4
SET CMakeIt=0
SET CMakeNinja=0
SET CMakeDeploy=0
SET DebugBuild=0

set SRCDIR=%CD%

:argsloop
if "%1"=="clean" goto cleanX
if "%1"=="update-submodules"  goto updateSubModules
if "%1"=="repair-submodules"  goto repairSubModules
if "%1"=="nopause"  SET NoPause=1
if "%1"=="noeditor" SET BuildArgs=%BuildArgs% CONFIG+=noeditor
if "%1"=="noengine" SET BuildArgs=%BuildArgs% CONFIG+=noengine
if "%1"=="nocalibrator" SET BuildArgs=%BuildArgs% CONFIG+=nocalibrator
if "%1"=="nogifs2png" SET BuildArgs=%BuildArgs% CONFIG+=nogifs2png
if "%1"=="nopng2gifs" SET BuildArgs=%BuildArgs% CONFIG+=nopng2gifs
if "%1"=="nolazyfixtool" SET BuildArgs=%BuildArgs% CONFIG+=nolazyfixtool
if "%1"=="nomanager" SET BuildArgs=%BuildArgs% CONFIG+=nomanager
if "%1"=="nomaintainer" SET BuildArgs=%BuildArgs% CONFIG+=nomaintainer
if "%1"=="nomusicplayer" SET BuildArgs=%BuildArgs% CONFIG+=nomusicplayer
if "%1"=="cmake-it" SET CMakeIt=1
if "%1"=="deploy" SET CMakeDeploy=1
if "%1"=="ninja" SET CMakeNinja=1
if "%1"=="debug" SET DebugBuild=1
if "%1"=="--help" goto Usage
shift
if NOT "%1"=="" goto argsloop

rem ------------------------------------------------------------
rem ------------------------------------------------------------
rem ------------------------------------------------------------
goto SkipUsage
:Usage
echo.
echo === Build script for PGE Project for Windows ===
echo.
echo SYNTAX:
echo.
echo     build.bat [arg1] [arg2] [arg3] ...
echo.
echo AVAILABLE ARGUMENTS:
echo.
echo --- Actions ---
echo  clean                - Remove all object files and caches to build from scratch
echo  update-submodules    - Pull all submodules up to their latest states
echo  repair-submodules    - Repair invalid or broken submodules
echo  --help               - Print this manual
echo  cmake-it             - Run build through experimental alternative build on CMake
echo  debug                - Run build in debug configuration
echo  ninja                - Use Ninja build system (CMake only build)
echo  deploy               - Automatically run a deploymed (CMake only build)
echo.
echo --- Flags ---
echo  nopause              - Disable pause on script completion
echo.
echo --- Disable building of components ---
echo  noeditor              - Skip building of PGE Editor compoment
echo  noengine              - Skip building of PGE Engine compoment
echo  nocalibrator          - Skip building of Playable Character Calibrator compoment
echo  nomaintainer          - Skip building of PGE Maintainer compoment
echo  nomanager             - Skip building of PGE Manager compoment
echo  nomusicplayer         - Skip building of PGE MusPlay compoment
echo  nogifs2png            - Skip building of GIFs2PNG compoment
echo  nopng2gifs            - Skip building of PNG2GIFs compoment
echo  nolazyfixtool         - Skip building of LazyFixTool compoment
echo.
set NoPause=1
set OldPATH=%PATH%
goto quit
:SkipUsage

if %DebugBuild%==0 echo ==RELEASE BUILD!==
if %DebugBuild%==1 echo ==DEBUG BUILD!==

if %DebugBuild%==0 set BUILD_DIR_SUFFUX=-release
if %DebugBuild%==1 set BUILD_DIR_SUFFUX=-debug

if %DebugBuild%==0 set CONFIG_QMAKE=CONFIG+=release CONFIG-=debug
if %DebugBuild%==1 set CONFIG_QMAKE=CONFIG-=release CONFIG+=debug

if %DebugBuild%==0 set CONFIG_CMAKE=Release
if %DebugBuild%==1 set CONFIG_CMAKE=Debug

rem ------------------------------------------------------------
rem ------------------------------------------------------------
rem ------------------------------------------------------------

IF NOT EXIST _paths.bat echo _paths.bat is not exist! Run "generate_paths.bat" first!
IF NOT EXIST _paths.bat goto error

call _paths.bat
set OldPATH=%PATH%
PATH=%QtDir%;%MinGW%;%GitDir%;%CMakeDir%;%SystemRoot%\system32;%SystemRoot%;

if %CMakeIt%==1 goto cMakeIt

IF "%MINGWx64Dest%"=="yes" (
	SET BuildArgs=%BuildArgs% CONFIG+=win64
)
IF "%MINGWx32Dest%"=="yes" (
	SET BuildArgs=%BuildArgs% CONFIG+=win32-mingw-w64
)

rem ------------------------------------------------------------
rem ------------------------------------------------------------
rem ------------------------------------------------------------
goto run
:cMakeIt
    
echo ==== Alternative build via CMake will be ran ====
echo It's experimental build yet which will replace
echo this clunky build system soon.
echo I hope you will like it ;3
echo.
echo Wohlstand.
echo =================================================
echo.

set BUILD_DIR=%SRCDIR%/build-pge-cmake%BUILD_DIR_SUFFUX%
set INSTALL_DIR=%SRCDIR%/bin-cmake%BUILD_DIR_SUFFUX%

if NOT EXIST "%BUILD_DIR%\NUL" md "%BUILD_DIR%"

cd "%BUILD_DIR%"

if %CMakeNinja%==1 SET CMAKE_GENERATOR=Ninja
if %CMakeNinja%==0 SET CMAKE_GENERATOR=MinGW Makefiles

@echo on
cmake -G "%CMAKE_GENERATOR%" -DCMAKE_PREFIX_PATH="%QtDir%/../" -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" -DCMAKE_BUILD_TYPE=%CONFIG_CMAKE% -DPGE_INSTALL_DIRECTORY="PGE_Project"  "%SRCDIR%"
@echo off
if ERRORLEVEL 1 goto error

rem ==== WORKAROUND for Ninja that won't allow refer not built yet libraries ====
if %CMakeNinja%==1 cmake --build . --target libs -- %MAKE_EXTRA_ARGS%
if %CMakeNinja%==1 if ERRORLEVEL 1 goto error

cmake --build . --target all -- %MAKE_EXTRA_ARGS%
if ERRORLEVEL 1 goto error

cmake --build . --target install -- %MAKE_EXTRA_ARGS%
if ERRORLEVEL 1 goto error

cmake --build . --target windeploy -- %MAKE_EXTRA_ARGS%
if ERRORLEVEL 1 goto error

if %CMakeDeploy%==0 goto cMakeSkipDeploy
    rem === Small tweaking inside of the repository ===
    cmake --build . --target enable_portable
    if ERRORLEVEL 1 goto error

    cmake --build . --target put_online_help
    if ERRORLEVEL 1 goto error

    rem === Packing ZIP archives to publish them later ===
    cmake --build . --target create_zip -- %MAKE_EXTRA_ARGS%
    if ERRORLEVEL 1 goto error

    cmake --build . --target create_zip_tools -- %MAKE_EXTRA_ARGS%
    if ERRORLEVEL 1 goto error

    cmake --build . --target create_zip_install -- %MAKE_EXTRA_ARGS%
    if ERRORLEVEL 1 goto error
:cMakeSkipDeploy

cd "%SRCDIR%"

echo.
echo =========BUILT!!===========
echo.
exit /B 0
goto quit;

:cleanX
echo ======== Remove all cached object files and automatically generated Makefiles ========

call ./clean_make.bat nopause

IF EXIST .\bin-w32\_build_x32\NUL (
                    echo removing bin-w32/_build_x32 ...
                    rmdir /s /q bin-w32\_build_x32
)

IF EXIST .\bin-w64\_build_x32\NUL (
                    echo removing bin-w64/_build_x32 ...
                    rmdir /s /q bin-w64\_build_x32
)

IF EXIST .\bin-w32\_build_x64\NUL (
                    echo removing bin-w32/_build_x64 ...
                    rmdir /s /q bin-w32\_build_x64
)

IF EXIST .\_Libs\_sources\_build_cache\NUL (
    echo 'removing Dependencies build cache ...'
    rmdir /s /q _Libs\_sources\_build_cache
)

IF EXIST .\_Libs\_sources\_build_cache-w64\NUL (
    echo 'removing Dependencies build cache for 64-bit builds ...'
    rmdir /s /q _Libs\_sources\_build_cache-w64
)

IF EXIST .\_Libs\_sources\_build_cache_msvc\NUL (
    echo 'removing Dependencies build cache for MSVC ...'
    rmdir /s /q _Libs\_sources\_build_cache_msvc
)

echo ==== Clear! ====
exit /B 0
goto quit;


rem ------------------------------------------------------------
rem ------------------------------------------------------------
rem ------------------------------------------------------------

:updateSubModules
git submodule foreach git checkout master
git submodule foreach git pull origin master
exit /B 0
goto quit;


rem ------------------------------------------------------------
rem ------------------------------------------------------------
rem ------------------------------------------------------------

:repairSubModules
rem !!FIXME!! Implement parsing of submodules list and fill this array automatically
rem NOTE: Don't use "git submodule foreach" because broken submodule will not shown in it's list!
set SUBMODULES=_Libs\FreeImage
set SUBMODULES=%SUBMODULES% _Libs\QtPropertyBrowser
set SUBMODULES=%SUBMODULES% _Libs\sqlite3
set SUBMODULES=%SUBMODULES% _Libs\AudioCodecs
set SUBMODULES=%SUBMODULES% _Libs\SDL_Mixer_X
set SUBMODULES=%SUBMODULES% _common\PGE_File_Formats
set SUBMODULES=%SUBMODULES% _common\PgeGameSave\submodule
set SUBMODULES=%SUBMODULES% Content\help
rem \===============================================================================
for %%s in (%SUBMODULES%) do (
echo Remove folder %%s ...
if exist %%s\nul rd /Q /S %%s
)
echo Fetching new submodules...
git submodule init
git submodule update
echo.
git submodule foreach git checkout master
git submodule foreach git pull origin master
echo.
echo ==== Fixed! ====
exit /B 0
goto quit;

rem ------------------------------------------------------------
rem ------------------------------------------------------------
rem ------------------------------------------------------------

:run
cd %CD%\Editor
%QtDir%\lrelease.exe *.pro
cd ..\Engine
%QtDir%\lrelease.exe *.pro
cd ..

rem build all components
%QtDir%\qmake.exe %CONFIG_QMAKE% %BuildArgs%
if ERRORLEVEL 1 goto error

%MinGW%\mingw32-make %MAKE_EXTRA_ARGS% release
if ERRORLEVEL 1 goto error

rem copy data and configs into the build directory

%MinGW%\mingw32-make install
if ERRORLEVEL 1 goto error

echo.
echo =========BUILT!!===========
echo.

cd "%SRCDIR%"
goto quit
:error
echo.
echo =========ERROR!!===========
echo.
PATH=%OldPATH%
cd "%SRCDIR%"
if "%NoPause%"=="0" pause
exit /B 1
:quit
PATH=%OldPATH%
if "%NoPause%"=="0" pause

