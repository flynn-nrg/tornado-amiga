cd utils
lz.exe -o song.prt.lz song.prt
if %errorlevel% neq 0 exit /b %errorlevel%
cranker_windows.exe -f stub -o song.exe -a song.prt.lz
move song.exe ..\Amiga_PretrackerSong.exe