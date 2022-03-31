grep "^r" out.tr > clean.out
grep -v "rtProtoDV" clean.out > temp.out
cat temp.out > clean.out

sed -i -e 's/ /,/g' clean.out

sort -n -t, -k12 clean.out > temp.out
cat temp.out > clean.out

sed -i -e 's/,/\t/g' clean.out



