@echo off
del .\bin\*.exe
del .\bin\*.map
rd /s /q .\bin_tests
rd /s /q .\build
rd /s /q .\build_tests
del .\*.ppx
del .\*.tag

