( find ./tmp/source -type f -name '*' -print0 | xargs -0 cat ) > tmp/all-code.txt
echo total lines `cat tmp/all-code.txt | wc -l ` | tee tmp/output.txt
echo total size `du -h "tmp/all-code.txt" | cut -f1` | tee -a tmp/output.txt
echo | tee -a tmp/output.txt
cloc tmp/source/ | tee -a tmp/output.txt