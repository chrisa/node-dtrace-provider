logman stop mytrace1 -ets
logman stop mytrace2 -ets
logman stop mytrace3 -ets
logman stop mytrace4 -ets

del "multi_summary1.txt"
del "multi_dump1.xml"
del "multi_summary2.txt"
del "multi_dump2.xml"
del "multi_summary3.txt"
del "multi_dump3.xml"
del "multi_summary4.txt"
del "multi_dump4.xml"

tracerpt mytrace1.etl -o multi_dump1.xml -summary multi_summary1.txt
tracerpt mytrace1.etl -o multi_dump2.xml -summary multi_summary2.txt
tracerpt mytrace3.etl -o multi_dump3.xml -summary multi_summary3.txt
tracerpt mytrace4.etl -o multi_dump4.xml -summary multi_summary4.txt