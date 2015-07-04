#!/usr/bin/python

import argparse, subprocess, threading, os

RUNNER_POSTPATH = "runner"
TEST_DELIM = "test"
DEFAULT_INPUT = "testfiles_compiled/"
TIMEOUT = 3600 # timeout for each execution of spike

# Class to (in a new thread)
# execute a shell command in a separate thread
# and wait for it to terminate (up to a timeout).
class CommandWaiter(object):
    def __init__(self, cmd, num, silent=False):
        self.cmd = cmd
        self.process = None
        self.num = num
        self.silent = silent

    # Body of thread.
    def doThread(self, maxTime):
        # Body of interior thread.
        def target():
            if self.silent:
                with open(os.devnull, "w") as fnull:
                    self.process = subprocess.Popen(self.cmd, shell=True,
                        stdout=fnull, stderr=fnull)
            else:
                self.process = subprocess.Popen(self.cmd, shell=True)
            self.process.communicate()

        # Start our shell command in a subprocess, and wait for it to finish.
        thread = threading.Thread(target=target)
        thread.start()
        thread.join(maxTime)

        # Check if the program is still running. If so, kill it.
        if thread.is_alive():
            print "Terminating spike", self.num
            self.process.terminate()
            thread.join()
        if self.process.returncode != 0:
          print "Spike", self.num, "returned:", self.process.returncode

    # Function to run body of thread in a new thread. Returns immediately.
    def run(self, maxTime):
        thread = threading.Thread(target=self.doThread, args=[maxTime])
        thread.start()

# make output folder if it doesn't exist
def make_folder(path):
    try:
        os.makedirs(path)
    except OSError:
        if not os.path.isdir(path):
            raise Exception # only raise unrelated OSErrors

def main():
    # parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', default=DEFAULT_INPUT,
        help='path to the test BINARIES')
    parser.add_argument('--sd', help='path to testrunner_setup_disk.sh', required=True)
    parser.add_argument('--vml', help='path to vmlinux', required=True)
    parser.add_argument('--spike', help='spike executable name', required= True)
    args = parser.parse_args()

    # fixup arguments
    input_path = os.path.abspath(args.path)
    setupdisk_path = os.path.abspath(args.sd)
    vmlinux_path = os.path.abspath(args.vml)
    spike = args.spike # name should be on path

    # read input folder, make disks from each
    for folder in os.listdir(input_path):
        if folder.startswith("stderr"):
            continue # ignore this folder

        # run setup_disk.sh on this folder
        folder_path = os.path.join(input_path, folder)
        path_to_tester = os.path.join(folder_path, RUNNER_POSTPATH)

        # redirect stdout of setup_disk.sh to /dev/null
        with open(os.devnull, "w") as fnull:
            retcode = subprocess.call(setupdisk_path + " " + folder_path + " " + folder_path
                + " " + path_to_tester + " " + TEST_DELIM, shell=True, stdout=fnull)
        if retcode != 0:
            raise Exception("setup_disk.sh failed with code: {}".format(retcode))

    # for each disk, start spike
    disk = 0
    for folder in os.listdir(input_path):
        if folder.startswith("stderr"):
            continue # ignore this folder

        # start spike on this disk
        folder_path = os.path.join(input_path, folder)
        disk_path = os.path.join(folder_path, "root.bin")
        stdout_path = os.path.join(folder_path, "{}.stdout".format(RUNNER_POSTPATH))

        invocation = [spike, "+disk={}".format(disk_path),
            vmlinux_path, ">", stdout_path]
        cmd_to_run = " ".join(invocation)

        # run the cmd in a subprocess
        runner = CommandWaiter(cmd_to_run, disk)
        runner.run(TIMEOUT)

        disk += 1

    print "Started {} spike(s)".format(disk)

if __name__ == "__main__":
    main()
