grep "^r" out.tr > clean.out
grep -v "rtProtoDV" clean.out > temp.out
cat temp.out > clean.out

# sort -n -t, -k11 clean.out > temp.out
# cat temp.out > clean.out

sed -i -e 's/ /\t/g' clean.out 