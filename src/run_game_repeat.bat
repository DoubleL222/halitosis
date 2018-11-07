call make.bat
FOR /L %%A IN (0,1,5) DO (halite.exe --replay-directory replays/ -vvv --width 32 --height 32 --results-as-json "MyBot.exe "%A%" "%A% "MyBot.exe 1 2")
PAUSE