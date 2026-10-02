@echo off

if not exist builds\Debug mkdir builds\Debug

cmake -S . -B builds\Debug -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 exit /b %errorlevel%

copy /Y builds\Debug\compile_commands.json compile_commands.json
if errorlevel 1 exit /b %errorlevel%

cmake --build builds\Debug --parallel
exit /b %errorlevel%