#!/usr/bin/python

# derived from the 6.035 Test Suite written by Isaac Evans, Spring 2013

# ~ a pure python replacement ~

import collections
import sqlite3
import tempfile
import subprocess
import sys
import os

verbose_shell = False

# We assume we have compiled a set of executable statements (very generically!
# could be compiler invocations, program invocations, etc), and we want to
# exercise them and discover whether they match the previous outputs marked as
# correct ("golden" outputs)

# For convenience, the exeuctable statments may be exeuctable files, or simply
# files. If they are files, the CC environment variable will be used to compile
# them. Golden outputs have the same name as the executable file or file they
# are derived from with the addition of a suffix:
expected_output_suffix = '.output'

# We assume that executables that are supposed to segfault, return non-zero
# values, etc., will be handled by a wrapper script that outputs that value in
# such a way that it can be compared to the golden output

# Modes are: 
# (a) compare output to golden output
# (b) track size of output 
# (c) track execution time to produce output 

make_gold = False
if len(sys.argv) >= 2:
    make_gold = (sys.argv[1] == 'gold')

ccompiler = os.environ.get('CC')
cflags = os.environ.get('CFLAGS')
cflags = cflags if cflags != None else ''

pk_environments = [
    'spike pk',
    'spike-no-return-copy pk',
]

linux_environments = [
    'spike +disk=root.bin vmlinux'
]

tests = {
          'single-file-tests': [('single-file-tests/bin/riscv', 'single-file-tests', pk_environments)],
          'need_linux': [('need_linux/bin/riscv', 'need_linux', pk_environments)],
        }

def shell(command):
    if verbose_shell: print '[shell]', command
    p = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output = [x for x in p.stdout.readlines()]
    retval = p.wait()
    if verbose_shell: print '[shell] returned %d, output:\n%s' % (retval, output)
    return output, retval

def getSHA(): 
    return shell('git rev-parse --short HEAD')[0][0].strip()

db = None
conn = sqlite3.connect('tests.sqlite')
db = conn.cursor()
cols = ['id', 'name', 'suite', 'githash', 'policies', 'time_taken', 'cycles', 'filesize', 'output', 'golden', 'result']
table_spec = '''%s INTEGER PRIMARY KEY, %s TEXT, %s TEXT, %s TEXT,  %s TEXT, %s INTEGER, %s INTEGER, %s INTEGER, %s TEXT, %s TEXT, %s INT''' % tuple(cols)

db.execute('CREATE TABLE IF NOT EXISTS test_results (%s)' % table_spec)
rows = db.execute('SELECT COUNT(*) FROM test_results').fetchone()

print getSHA()
if rows > 0:
    print '[info] loaded previous result data from tests.sqlite: %d rows' % rows
    thishash = db.execute('SELECT COUNT(*) FROM test_results WHERE githash = ?', (getSHA(),)).fetchone()
    if thishash[0] != 0:
        print '[info] tests have already been run for rev ' + getSHA() + '; rerun tests? [y/n] ',
        ans = raw_input()
        if ans != 'y' and ans != 'Y':
            sys.exit(0)
        db.execute('DELETE FROM test_results WHERE githash = ?', [getSHA()])

def logInstrumentedSuccess(test_data, assemblyFile, cycles):
    # count the number of lines in the output file
    lines = shell('wc -l < ' + assemblyFile)[0][0].strip()
    test_data['cycles'] = cycles
    test_data['filesize'] = lines
    logSuccess(test_data)

def logSuccess(test_data):
    test_data['result'] = 1
    log(test_data)

def logFailure(test_data, message):
    test_data['result'] = 0
    test_data['output'] = message
    log(test_data)

def log(test_data):
    assert(test_data['result'] == 0 or test_data['result'] == 1)
    entries = [str(test_data[col]) for col in cols if col != 'id']
    columns = [str(col) for col in cols if col != 'id']
    cmd = 'INSERT INTO test_results(%s) VALUES(%s)' % (str(columns)[1:-1], ('?,'*len(entries))[:-1])
    db.execute(cmd, tuple(entries))
    if len(test_data['output']) > 0 and type(test_data['output']) == type(''):
        print '[' + test_data['name'] + '] ' + test_data['output']

root_dir = os.getcwd()

def make_expected_path(outputDir, inputDir, file, suffix, policy_number):
    return os.path.join(outputDir + directory.replace(inputDir, ''), file + suffix) + '.' + str(policy_number) + '.txt'

for test_name, options in tests.items():
    inputDir, outputDir = map(lambda x: os.path.join(root_dir, x), list(options[0])[0:2])
    policies = list(options[0])[2]
    print '[info] running ' + test_name + ' tests (folder: ' + inputDir + ')'

    for directory, subdirectories, files in os.walk(inputDir):
        for file in files:
            input =  os.path.join(directory, file)
            binary = tempfile.NamedTemporaryFile()
            output = tempfile.NamedTemporaryFile()

            for policy_number, policy in enumerate(policies):
                expected = make_expected_path(outputDir,inputDir, file, expected_output_suffix, policy_number)

                test_data = collections.defaultdict(lambda: 'NULL', {'suite': test_name, 'name': input, 'githash': getSHA(), 'policies': policy, 'golden': expected })

                is_executable = os.access(input, os.X_OK)
                if ccompiler == None and not is_executable:
                    logFailure(test_data, 'no compiler is defined, but the input file is not executable')
                    continue

                if ccompiler != None:
                    compiler_call = '%s %s -o %s %s' % (ccompiler, cflags, binary.name, input)
                    error_output, retval = shell(compiler_call)
                    if retval != 0:
                        logFailure(test_data, 'compiler gave nonzero return code ' + str(retval) + ' for command ' + compiler_call)
                        continue
                    if len(error_output) > 0:
                        logFailure(test_data, 'unexpected output ' + ''.join(error_output))
                        continue

                # TODO(inevans) support multiple policy types 
                execution_prefix = policy

                # also redirect stderr to stdout (2>&1)
                exe_call = '%s %s > %s 2>&1' % (execution_prefix, input if is_executable else binary.name, output.name)
                exe_error, exe_ret_val = shell(exe_call)

                if exe_ret_val != 0:
                    logFailure(test_data, 'program failed to run (command "%s" returned %d): %s' %
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
                        logFailure(test_data, 'program output did not match expected output ' + ''.join(diffOutput))
                        continue

                cycles = '0'
                logInstrumentedSuccess(test_data, output.name, cycles)

            binary.close()
            output.close()

print '*'*80
print 'test suite concluded, run suite-report to see output'

# done! let's write out the output 
conn.commit()
conn.close()
