
for f in ./all_satisfiable/*; do
	../build/fousaty $f;
	python3 check_model.py "model_file.txt" $f 
done

echo "------ UNSAT ------"

for f in ./all_unsat/*; do
	../build/fousaty $f
done
