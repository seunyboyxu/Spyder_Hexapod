@echo off
setlocal
title Spyder Console 3.2
echo Spyder Console 3.2 - use your existing revision 3 sketch.
echo Keep servo power disconnected while connecting.
echo Open http://127.0.0.1:8033/?console=3.2 in Chrome or Edge.
echo Leave this window running. Press Ctrl+C to stop.
python -m http.server 8033 --bind 127.0.0.1 --directory "%~dp0web"
if errorlevel 1 pause
