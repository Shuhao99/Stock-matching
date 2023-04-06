xml=$(cat test_create.xml)
size=$(echo -n "$xml" | wc -c)
content="$size\n$xml"
echo -e "$content" | nc -w 1 vcm-30745.vm.duke.edu 12345 >> response.xml