#!/usr/bin/python3

import sys, glob, os, shutil, subprocess

memcheck = False

def fetch_input_impl(fin, out):
  fout = open(out, "w")
  while True:
    line = fin.readline()
    if line == "---input---\n":
      return True
    if line == "":
      return False
    print(line, file=fout, end='')

def fetch_input(fprog, fin):
    ret = fetch_input_impl(fprog, "tmp")
    if not fin == 0:
      fetch_input_impl(fin, "tmp2")
    return ret

def produce_output(fprog, fout, first, fin):
  mila = "../../../../llvm-obj/Debug+Asserts/examples/Sfe"

  if not first:
    f = open(fout, "a")
    print("---output---", file=f)
    f.close()

  if memcheck:
    valgrind = "valgrind --tool=memcheck --leak-check=yes "
    code = os.system(valgrind + mila + " " + fprog)
  else:
    code = os.system(mila + " " + fprog + " 1>/dev/null 2>&1")
  if code == 0:
    if os.path.isfile(fin):
        os.system("./a.out < " + fin + " >> " + fout)
    else:
        os.system("./a.out >> " + fout)

  f = open(fout, "a")
  print(str(code), file=f)
  f.close()

def gen_input(folder, prog):
  print("processing " + prog)
  program = prog[prog.find("/")+1:-5]
  fprog = open(prog, "r")

  finstr = "input/" + program + ".txt"
  fin = 0
  if os.path.isfile(finstr):
    fin = open(finstr, "r")

  first = True
  while fetch_input(fprog, fin):
    produce_output("tmp", folder + "/" + program + ".txt", first, "tmp2")
    first = False
  produce_output("tmp", folder + "/" + program + ".txt", first, "tmp2")

def gen(folder, input):
  if input == "--all":
    if os.path.isdir(folder):
      shutil.rmtree(folder)
    os.mkdir(folder)
  else:
    if os.path.isfile(folder + "/" + input + ".txt"):
      os.remove(folder + "/" + input + ".txt")

  if input == "--all":
    for file in glob.glob("program/*.mila"):
      gen_input(folder, file)
  else:
      gen_input(folder, "program/" + input + ".mila")

def test():
  gen("output", "--all")
  for file in glob.glob("output/*.txt"):
    code = os.system("diff golden/" + file[7:] + " " + file + " > tmp")
    if code != 0:
      print("difference in " + file + " found")
      with open("tmp", "r") as f:
        print(f.read(), end='')

if __name__ == "__main__":
  if len(sys.argv) == 3:
    if sys.argv[1] == '--gen':
      print('generating golden data')
      gen("golden", sys.argv[2])
  elif len(sys.argv) == 1 or len(sys.argv) == 2:
    if len(sys.argv) == 2 and sys.argv[1] == '--mem':
        print('memcheck enabled')
        memcheck = True
    print('testing')
    test()
  else:
    print("invalid arguments")
