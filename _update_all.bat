@cd "%~dp0"
@echo Pulling project
@git pull
@if not exist "%~dp0.gitmodules" @goto SUBMODULES_UPDATE_DONE
@echo Updating submodules
@git submodule update --init --recursive --remote --merge
@if "%1"=="no-pull" @goto NO_PULL
@if "%1"=="no_pull" @goto NO_PULL
@if "%1"=="NO-PULL" @goto NO_PULL
@if "%1"=="NO_PULL" @goto NO_PULL
@if "%1"=="no-pull-submodules" @goto NO_PULL
@if "%1"=="no_pull_submodules" @goto NO_PULL
@if "%1"=="NO-PULL-SUBMODULES" @goto NO_PULL
@if "%1"=="NO_PULL_SUBMODULES" @goto NO_PULL
@if not exist "%~dp0.bat" @goto SUBMODULE_BAT_DONE
@cd "%~dp0.bat"
@echo Pulling .bat submodule
@git pull
@cd "%~dp0"
:SUBMODULE_BAT_DONE
@if not exist "%~dp0.cmake" @goto SUBMODULE_CMAKE_DONE
@cd "%~dp0.cmake"
@echo Pulling .cmake submodule
@git pull
@cd "%~dp0"
:SUBMODULE_CMAKE_DONE
:NO_PULL
@if not exist "%~dp0_libs\update_libs.bat" @goto LIBS_DONE
@cd "%~dp0_libs"
@echo Pulling libs
@call update_libs.bat
@cd "%~dp0"
:LIBS_DONE
@exit /B 0
