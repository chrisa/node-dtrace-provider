set folderName=%time:~0,2%_%time:~3,2%__%date:~4,2%_%date:~7,2%_%date:~10,4%
set testsDir=tests

call auto_testing.bat>output.txt

xcopy output.txt ..\test_results\%folderName%\
del output.txt
del mytrace.etl
del summary.txt
del dumpfile.xml