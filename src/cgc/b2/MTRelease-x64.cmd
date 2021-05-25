@echo off

if "%BOOST_ROOT%"=="" (
  echo "Please set environment variable BOOST_ROOT to the location of Boost."
  goto EXIT
)
cd /d "%BOOST_ROOT%"
.\b2.exe address-model=64 link=static runtime-link=static variant=release "%~dp0"
pause
