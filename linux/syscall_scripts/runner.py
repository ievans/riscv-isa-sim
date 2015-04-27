#!/usr/bin/python

import subprocess, threading, tempfile

numSyscalls = 276
numIterations = 250
numSeconds = 360 # timeout for each syscall runner
command = ( "bash "
            "/home/jugonzalez/Documents/riscv-tools/riscv-isa-sim/linux/run_trinity.sh "
            "/home/jugonzalez/Documents/riscv-tools/riscv-isa-sim/test/need_linux/syscalls/bin/trinity"
            " --dangerous -N{0:d}"
            " -j{1:d} > stdout 2> stderr")

# Class to (in a new thread)
# execute a shell command in a separate thread
# and wait for it to terminate (up to a timeout).
class CommandWaiter(object):
    def __init__(self, cmd, num, directory):
        self.cmd = cmd
        self.process = None
        self.num = num
        self.dir = directory

    # Body of thread.
    def doThread(self, maxTime):
        # Body of interior thread.
        def target():
            print "Thread", self.num, "started"
            self.process = subprocess.Popen(self.cmd, shell=True, cwd = self.dir)
            self.process.communicate()
            print "Thread", self.num, "finished"

        # Start our shell command in a subprocess, and wait for it to finish.
        thread = threading.Thread(target=target)
        thread.start()
        thread.join(maxTime)

        # Check if the program is still running. If so, kill it.
        if thread.is_alive():
            print "Terminating thread", self.num
            self.process.terminate()
            thread.join()
        print "Thread", self.num, "returned:", self.process.returncode

    # Function to run body of thread in a new thread. Returns immediately.
    def run(self, maxTime):
        thread = threading.Thread(target=self.doThread, args=[maxTime])
        thread.start()

# Runs the command once for all numSyscalls system calls, in separate directories.
def main():
    for i in xrange(numSyscalls):
        # Make a temporary directory to store the output of our command.
        suffixStr = "{0:d}".format(i)
        tempdir = tempfile.mkdtemp(suffix=suffixStr) # make new temp folder for this test
        print "For thread {0:d}, the tempdir is located at {1}".format(i, tempdir)

        # Format the command for this syscall and start it.
        shell_cmd = command.format(numIterations, i) # fill in num iterations and syscall number
        cmd = CommandWaiter(shell_cmd, i, tempdir)
        cmd.run(numSeconds) # Start the command, which will run in a new thread.


if __name__ == "__main__":
    print "Runner thread started"
    main()
    print "Runner thread finished"