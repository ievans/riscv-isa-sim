#!/usr/bin/python

import argparse, shutil, subprocess, threading, os

CC = "riscv64-unknown-linux-gnu-gcc"
MAKE_TEST = CC + " {} -o {}/{} > /dev/null 2> {}/{}.stderr"
MAKE_RUNNER = CC + " {} -o {} > /dev/null 2> /dev/null"

NUM_PROCS = 1
STDERR_POSTPATH = "stderr"
RUNNER_POSTPATH = "runner"
SET_DELIM = "set{}"
TEST_DELIM = "test"
TEST_EXTENSION = ".c"
DEFAULT_OUTPUT = "testfiles_compiled/"
TIMEOUT = 120 # timeout for each compilation

# Class to (in a new thread)
# execute a shell command in a separate thread
# and wait for it to terminate (up to a timeout).
class CommandWaiter(object):
    def __init__(self, cmd, name="unknown"):
        self.cmd = cmd
        self.process = None
        self.name = name

    # Body of thread.
    def doThread(self, maxTime):
        # Body of interior thread.
        def target():
            self.process = subprocess.Popen(self.cmd, shell=True)
            self.process.communicate()

        # Start our shell command in a subprocess, and wait for it to finish.
        thread = threading.Thread(target=target)
        thread.start()
        thread.join(maxTime)

        # Check if the program is still running. If so, kill it.
        if thread.is_alive():
            print "Terminating build for", self.name
            self.process.terminate()
            thread.join()
        if self.process.returncode > 0:
          print "Build for", self.name, "returned:", self.process.returncode

    # Function to run body of thread in a new thread. Returns immediately.
    def run(self, maxTime):
        thread = threading.Thread(target=self.doThread, args=[maxTime])
        thread.start()

def compile_test_runner(runner_path):
  runner_compile_cmd = MAKE_RUNNER.format(runner_path, RUNNER_POSTPATH)
  with open(os.devnull, "w") as fnull:
    retcode = subprocess.call(runner_compile_cmd, shell=True, stderr=fnull)
  if retcode != 0:
    raise Exception("failed to compile C test runner!")

# make output folder if it doesn't exist
def make_folder(path):
  try:
    os.makedirs(path)
  except OSError:
    if not os.path.isdir(path):
      raise Exception # only raise unrelated OSErrors

# delete everyting inside given folder path
# http://stackoverflow.com/questions/185936/delete-folder-contents-in-python
def clean_folder(path):
  for the_file in os.listdir(path):
    file_path = os.path.join(path, the_file)
    try:
      if os.path.isfile(file_path):
        os.unlink(file_path)
      elif os.path.isdir(file_path):
        shutil.rmtree(file_path)
    except Exception, e:
      print e

# create output folder and set up structure
def init_output_folder(output_path):
  # make output folder if it doesn't exist
  make_folder(output_path)

  # make folder for compile stderr's
  stderr_path = os.path.join(output_path, STDERR_POSTPATH)
  make_folder(stderr_path)

  # partition tests into set of output folders
  for i in xrange(NUM_PROCS):
    new_folder_path = os.path.join(output_path, SET_DELIM.format(i))
    make_folder(new_folder_path)
    # delete old files in case folder exists already
    clean_folder(new_folder_path)

    # copy built test runner into this folder
    new_runner_path = os.path.join(new_folder_path, RUNNER_POSTPATH)
    shutil.copyfile(RUNNER_POSTPATH, new_runner_path)
    shutil.copymode(RUNNER_POSTPATH, new_runner_path) # file needs +x

    # make folder inside of new folder for tests
    test_folder_path = os.path.join(new_folder_path, TEST_DELIM)
    make_folder(test_folder_path)


# generator over all files with given extension in path
# yields absolute paths
def get_all_files(path, extension, recursive):
  for the_file in os.listdir(path):
    file_path = os.path.join(path, the_file)
    try:
      if os.path.isfile(file_path) and file_path.endswith(TEST_EXTENSION):
        yield file_path
      elif recursive and os.path.isdir(file_path):
        for x in get_all_files(file_path, extension, recursive):
          yield x
    except Exception, e:
      print e

def main():
  global NUM_PROCS
  # parse arguments
  parser = argparse.ArgumentParser()
  parser.add_argument('--path', help='path to the tests source folder',
    required=True)
  parser.add_argument('--output', help='desired output folder name',
    default=DEFAULT_OUTPUT)
  parser.add_argument('--runner', help='path to C test runner source',
    required=True)
  parser.add_argument('-n', help='number of cores to partition into',
    required=False)
  parser.add_argument('-r', help='Recursively search path for c sources',
    action='store_true')
  args = parser.parse_args()

  # fixup arguments
  input_path = os.path.abspath(args.path)
  output_path = os.path.abspath(args.output)
  runner_path = os.path.abspath(args.runner)
  if args.n:
    NUM_PROCS = int(args.n)
  recursive = args.r

  # compile program which runs the tests inside spike linux
  compile_test_runner(runner_path)

  # set up output folder
  init_output_folder(output_path)

  # delete compiled test runner (assume still exists)
  os.remove(RUNNER_POSTPATH)

  # read input folder and compile all the srcs
  n_tests = 0
  stderr_path = os.path.join(output_path, STDERR_POSTPATH)
  for file_path in get_all_files(input_path, TEST_EXTENSION, recursive):
    proc = n_tests % NUM_PROCS
    # create cmd to run on this file
    filename = os.path.basename(file_path)
    no_ext = os.path.splitext(filename)[0]
    output_dir = os.path.join(output_path,
      "{}/{}".format(SET_DELIM, TEST_DELIM).format(proc))
    test_compile_cmd = MAKE_TEST.format(file_path, output_dir, no_ext,
      stderr_path, no_ext)

    # run the cmd in a subprocess
    test_compiler = CommandWaiter(test_compile_cmd, file)
    test_compiler.run(TIMEOUT)

    # update the number of tests compiled so far
    n_tests += 1

if __name__ == "__main__":
  main()
