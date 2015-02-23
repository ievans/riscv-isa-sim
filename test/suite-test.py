#!/usr/bin/python

# derived from the 6.035 Test Suite written by Isaac Evans, Spring 2013

# ~ a pure python replacement ~

import collections
import pickle
import tempfile
import subprocess
import sys
import os

# We assume we have compiled a set of executable statements (very generically!
# could be compiler invocations, program invocations, etc), and we want to
# exercise them and discover whether they match the previous outputs marked as
# correct ("golden" outputs)

# For convenience, the exeuctable statments may be exeuctable files, or simply
# files. If they are files, the CC environment variable will be used to compile
# them. Golden outputs have the same name as the executable file or file they
# are derived from with the addition of a suffix:
expected_output_suffix = '.out'

# pre-execution action is also possible
execution_prefix = 'spike pk'

# We assume that executables that are supposed to segfault, return non-zero
# values, etc., will be handled by a wrapper script that outputs that value in
# such a way that it can be compared to the golden output

# Modes are: 
# (a) compare output to golden output
# (b) track size of output 
# (c) track execution time to produce output 

make_gold = False
if len(sys.argv) >= 2:
    make_gold = sys.argv[1] == 'gold'

ccompiler = os.environ.get('CC')
cflags = os.environ.get('CFLAGS')
cflags = cflags if cflags != None else ''

tests = {
          'single-file-tests': [('single-file-tests/bin/riscv', 'single-file-tests')],
          'need_linux': [('need_linux/bin/riscv', 'need_linux')],
        }

def shell(command):
    print '[shell]', command
    p = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output = [x for x in p.stdout.readlines()]
    retval = p.wait()
    return output, retval

def getSHA(): 
    return shell('git rev-parse --short HEAD')[0][0].strip()

resultsBySHA = collections.OrderedDict()
sha = getSHA()

if os.path.exists('tests.dat'):
    f = open('tests.dat')
    resultsBySHA = pickle.load(f)
    f.close()
    print '[info] loaded previous result data from tests.dat'
    if sha in resultsBySHA:
        print '[info] tests have already been run for rev ' + sha + '; rerun tests? [y/n] ',
        ans = raw_input()
        if ans != 'y' and ans != 'Y':
            sys.exit(0)
    else: # initialize
        resultsBySHA[sha] = collections.OrderedDict()
else:
    # clear everything out - or not; since the tests are individually done
    resultsBySHA[sha] = collections.OrderedDict()

def logInstrumentedSuccess(testName, fileName, assemblyFile, cycles):
    # count the number of lines in the output file
    lines = shell('wc -l < ' + assemblyFile)[0][0].strip()
    # number of cycles it took to execute
    instruments = [lines, cycles]
    logFailure(testName, fileName, instruments, True)

def logSuccess(testName, fileName):
    logFailure(testName, fileName, ['', ''], True)

def logFailure(testName, fileName, message, status = False):
    if len(message) > 0 and type(message) == type(''):
        print '[' + fileName + '] ' + message
    if fileName not in resultsBySHA[sha][testName]:
        resultsBySHA[sha][testName][fileName] = [(status, message)]
    else:
        resultsBySHA[sha][testName][fileName].extend([(status, message)])

root_dir = os.getcwd()
for test_name, options in tests.items():
    inputDir, outputDir = map(lambda x: os.path.join(root_dir, x), list(options[0]))
    resultsBySHA[sha][test_name] = {}
    print '[info] running ' + test_name + ' tests (folder: ' + inputDir + ')'

    for directory, subdirectories, files in os.walk(inputDir):
        for file in files:
            expected = os.path.join(outputDir, file + expected_output_suffix)
            input =  os.path.join(directory, file)
            binary = tempfile.NamedTemporaryFile()
            output = tempfile.NamedTemporaryFile()

            is_executable = os.access(input, os.X_OK)
            if ccompiler == None and not is_executable:
                logFailure(test_name, input, 'no compiler is defined, but the input file is not executable')
                continue

            if ccompiler != None:
                compiler_call = '%s %s -o %s %s' % (ccompiler, cflags, binary.name, input)
                error_output, retval = shell(compiler_call)
                if retval != 0:
                    logFailure(test_name, input, 'compiler gave nonzero return code ' + str(retval) + ' for command ' + compiler_call)
                    continue
                if len(error_output) > 0:
                    logFailure(test_name, input, 'unexpected output ' + ''.join(error_output))
                    continue

            # also redirect stderr to stdout (2>&1)
            exe_call = '%s %s > %s 2>&1' % (execution_prefix, input if is_executable else binary.name, output.name)
            exe_error, exe_ret_val = shell(exe_call)

            if exe_ret_val != 0:
                logFailure(test_name, input, 
                           'program failed to run (command "%s" returned %d): %s' %
                           (exe_call, exe_ret_val, ''.join(exe_error)))
                continue

            if make_gold:
                # copy the output to the golden reference
                diffOutput, diffRetVal = shell('cp %s %s' % (output.name, expected))
            else:
                # run diff on the output
                diffCall = 'diff -u %s %s' % (output.name, expected)
                diffOutput, diffRetVal = shell(diffCall)
                if diffRetVal != 0:
                    logFailure(test_name, input, 'program output did not match expected output ' + ''.join(diffOutput))
                    continue

            cycles = '0'
            logInstrumentedSuccess(test_name, input, output.name, cycles)

            binary.close()
            output.close()

# done! let's write out the output 
pickle.dump(resultsBySHA, open('tests.dat', 'w'))
