import sys
# model file is of format var : valuation \in {0, 1}

def parse_model( model_file ):
    var = dict()
    with open(model_file) as mf:
        line = mf.readline()

        words = line.split(' ')
        for litstr in words[2:-2]:
            lit = int(litstr)
            var[str(abs(lit))] = lit > 0
            
    return var

def parse_dimacs( filename ):
    with open(filename) as ff:
        lines = ff.readlines()

        clauses = [[]]

        for line in lines:
            line = line.strip()
            if line[0] == 'c' or line[0] == 'p':
                continue

            # signals end of dimacs file on satlib benches
            if line[0] == '%':
                break

            words = line.split(' ')
            for word in words:
                word = word.strip()

                if word == '0':
                    clauses.append([])
                else:
                    clauses[-1].append( word )


    clauses.pop(-1)
    return clauses

def verify_model( filename, model_file ):
    model = parse_model( model_file )
    formula = parse_dimacs( filename )

    for clause in formula:
        sat = False

        for literal in clause:
            neg = False
            if literal[0] == '-':
                neg = True

            var = literal[(neg * 1):]
            val = model[var]
            sat = sat or ( neg and not val ) or ( not neg and val )

        if not sat:
            print( clause )
            return False
    return True


if __name__ == "__main__":
    model_path = sys.argv[1]
    dimacs_path = sys.argv[2]

    if not verify_model( dimacs_path, model_path ):
        print( "Incorrect model for", dimacs_path )



