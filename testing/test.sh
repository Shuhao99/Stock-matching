xml=$(cat test1.xml)
size=$(echo -n "$xml" | wc -c)
content="$size\n$xml"
echo -e "$content" | nc -w 1 vcm-30745.vm.duke.edu 12345 >> response.xml

xml=$(cat test1.xml)
size=$(echo -n "$xml" | wc -c)
content="$size\n$xml"
echo -e "$content" | nc -w 1 vcm-30745.vm.duke.edu 12345 >> response.xml