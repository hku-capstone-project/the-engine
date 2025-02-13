@echo off

@REM fetches all submodules for this project
git submodule update --init --recursive

@REM bootstrap vcpkg if not already bootstrapped
if not exist dep/vcpkg/vcpkg.exe (
    echo Bootstrapping vcpkg...
    cd dep/vcpkg
    bootstrap-vcpkg.bat
    cd ../..
    echo vcpkg bootstrapped successfully!
) else (
    echo vcpkg already bootstrapped!
)
