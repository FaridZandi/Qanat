grep "^r" out.tr > clean.dat
grep -v "rtProtoDV" clean.dat > temp.out
cat temp.out > clean.dat

sed -i -e 's/ /,/g' clean.dat

sort -n -t, -k12 clean.dat > temp.out
cat temp.out > clean.dat

sed -i -e 's/,/\t/g' clean.dat



