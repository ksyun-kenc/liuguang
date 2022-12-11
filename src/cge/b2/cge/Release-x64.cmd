@echo off

set argv=address-model=64 link=shared runtime-link=shared variant=release

pushd "%~dp0"

where b2
if %errorlevel%==0 (
  b2 %argv%
) else (
  if "%BOOST_ROOT%"=="" (
    echo "Please set environment variable BOOST_ROOT to the location of Boost."
    goto EXIT
  )

  if not exist "%BOOST_ROOT%\b2.exe" (
    echo "Please compile Boost first."
    goto EXIT
  )

  "%BOOST_ROOT%\b2.exe" %argv%
)

:EXIT
popd
pause
