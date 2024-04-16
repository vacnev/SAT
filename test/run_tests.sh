start=`date +%s`
for f in ./all_satisfiable/*; do
	../build/fousaty $f;
  	# python3 check_model.py "model_file.txt" $f 
done

ends=`date +%s`

for f in ./all_satisfiable_100/*; do
	../build/fousaty $f
done

ends100=`date +%s`

echo "------ UNSAT ------"

for f in ./all_unsat/*; do
	../build/fousaty $f
done
endu=`date +%s`

echo "------ BIG FAT UNSAT ------"

for f in ./all_unsat_200/*; do
	../build/fousaty $f
done
endu100=`date +%s`

echo "------ BIG FAT SAT ------"

for f in ./all_satisfiable_200/*; do
	../build/fousaty $f
done
ends200=`date +%s`

#for f in ./big_fat_unsat/*; do
#	../build/fousaty $f
#done


echo SAT time was `expr $ends - $start` seconds.
echo SAT 100 time was `expr $ends100 - $ends` seconds.
echo UNSAT easy time was `expr $endu - $ends100` seconds.
echo UNSAT 200 vars time was `expr $endu100 - $endu` seconds.
echo SAT 200 vars time was `expr $ends200 - $endu100` seconds.
