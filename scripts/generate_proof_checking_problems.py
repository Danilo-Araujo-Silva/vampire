import random
import os
import subprocess

N=100
VAMPIRE='../vampire_rel_master'
TPTP='~/TPTP/TPTP-v6.1.0/'

directory='generated_proof_obligations'
if not os.path.exists(directory):
    os.makedirs(directory)

#randomly generate N problems for proof checking
all_problems=set()
with open('all_relevant_problems','r') as probs:
	for line in probs:
		all_problems.add(line.strip())
problem_set = random.sample(all_problems,N)

# For each problem generate obligations
for problem in problem_set:
	print "Dealing with ",problem

	#Solve problem using casc mode
	OUT=""
	try:
		OUT=subprocess.check_output(VAMPIRE+' --ignore_missing on --mode casc --time_limit 300 --include '+TPTP+' '+TPTP+problem, shell = True)
	except subprocess.CalledProcessError as err:
		print "Problem not solved"
		continue

	#Extract option used to solve
	found=False
	option=""
	for line in reversed(OUT.split('\n')):
		line = line.strip()
		if found:
			option=line.split(' ')[0]
			break;
		if 'Refutation found' in line:
			found=True

	if not found:
		print "There was a problem, option not found!"
		continue
	#print 'Option is ',option, 'found is ',found

	#Generate proof checking problems
	os.system('python proof_checker.py '+VAMPIRE+' --ignore_missing on --decode '+option+'0 '+TPTP+problem)

	#Move them all to a directory for that problem
	obligations = [ f for f in os.listdir('.') if f.startswith('proof_obligation')]
	if obligations:
		if os.path.exists(problem):
			print "Oh dear, we did ", problem, " before!"
		else:
			pdir = directory+'/'+problem
			os.makedirs(pdir)
			for o in obligations:
				os.rename(o,pdir+'/'+o)
