echo "------ SAT ------"

for f in ./all_satisfiable/*; do
	../build/fousaty $f
done

echo "------ UNSAT ------"

for f in ./all_unsat/*; do
	../build/fousaty $f
done