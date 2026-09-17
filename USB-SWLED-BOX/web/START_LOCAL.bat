@echo off
cd /d "%~dp0"
where python >nul 2>nul
if %errorlevel% neq 0 (
  echo Python が見つかりません。
  echo Chrome または Edge から HTTPS サーバー上で開くか、Python をインストールしてください。
  pause
  exit /b 1
)
start "" http://localhost:8000
python -m http.server 8000
