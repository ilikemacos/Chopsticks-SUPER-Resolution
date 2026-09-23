@echo off
rem Launcher for the Universal FrameFX PowerShell edition.
rem Runs the app without changing your system's PowerShell execution policy.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0UniversalFrameFX.ps1" %*
