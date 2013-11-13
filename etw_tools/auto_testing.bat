for /f "tokens=*" %%i in (tests.txt) do (
call start_trace
node ..\%testsDir%\%%i.js>console_output.txt
call stop_trace
mkdir ..\test_results\%folderName%\%%i\
xcopy summary.txt ..\test_results\%folderName%\%%i\
xcopy dumpfile.xml ..\test_results\%folderName%\%%i\
xcopy mytrace.etl ..\test_results\%folderName%\%%i\
xcopy *.man ..\test_results\%folderName%\%%i\
del *.man
xcopy console_output.txt ..\test_results\%folderName%\%%i\
del console_output.txt
)

for /f "tokens=*" %%i in (gc_tests.txt) do (
call start_trace
node --expose-gc ..\%testsDir%\%%i.js>console_output.txt
call stop_trace
mkdir ..\test_results\%folderName%\%%i\
xcopy summary.txt ..\test_results\%folderName%\%%i\
xcopy dumpfile.xml ..\test_results\%folderName%\%%i\
xcopy mytrace.etl ..\test_results\%folderName%\%%i\
xcopy *.man ..\test_results\%folderName%\%%i\
del *.man
xcopy console_output.txt ..\test_results\%folderName%\%%i\
del console_output.txt
)