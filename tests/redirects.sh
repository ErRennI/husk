echo first > out.txt
cat out.txt
echo second >> out.txt
cat out.txt
echo overwrite > out.txt
cat < out.txt
tr a-z A-Z < out.txt > upper.txt
cat upper.txt
