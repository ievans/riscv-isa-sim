#!/usr/bin/python

# 6.035 Test Suite
# Isaac Evans, Spring 2013
# ~ a pure python replacement ~

# sorry this is such a hackjob mixing html with py; I wrote it on a plane so I
# didn't have access to any static templating libraries. oh well, it works

import os
import pickle
import collections
import subprocess
import sqlite3

def shell(command): 
    p = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output = [x for x in p.stdout.readlines()]
    retval = p.wait()
    return output, retval

gitMessages = collections.OrderedDict()
def gitInfo():
    output, retval = shell('git log --pretty=oneline --abbrev-commit')
    if retval == 0:
        for line in output:
            h, msg = (line[:line.index(' ')], line[line.index(' '):].strip())
            gitMessages[h] = msg

gitInfo()
current_hash = gitMessages.keys()[0]

def getSHA(): 
    return shell('git rev-parse --short HEAD')[0][0].strip()

project_name = 'riscv-isa-sim'
project_org = 'riscv-mit'

baseURL = 'https://github.com/%s/%s/commit/' % (project_org, project_name)
baseFileURL = 'https://github.com/%s/%s/blob/master/' % (project_org, project_name)

resultsBySHA = collections.OrderedDict()
resultsBySHA = pickle.load(open('tests.dat'))

# reverse the order of the keys so that the HEAD commit is on the left
# resultsBySHA = collections.OrderedDict(reversed(resultsBySHA.items()))

conn = sqlite3.connect('tests.sqlite')
db = conn.cursor()

def get_overall_rows_for_hash(sha_hash):
    overall_query = '''SELECT suite, SUM(result), COUNT(result) FROM test_results WHERE githash = ? GROUP BY suite'''
    s = ''
    for row in db.execute(overall_query, [sha_hash]):
        suite, success, total = row
        print(row)
        s += '<tr>'
        s += '<td>' + suite + '</td>'
        state = 'ok' if success == total else 'fail'
        s += '<td class="' + state + '">' + str(success) + '/' + str(total)  + '</td>'
        s += '</tr>'
    return s

# and now let's convert that output to HTML!
f = open('output.html', 'w')
s = '''<html><head><script src="//ajax.googleapis.com/ajax/libs/jquery/2.0.0/jquery.min.js"></script>
<style>
body { font-family: monospace; font-size: 10pt; }
.ok { background-color: #55FF33; }
.positive { background-color: #55FF33; }
.negative { background-color: red; color: white }
.fail { background-color: red; color: white; }
.category { background-color: black; color: white; vertical-align: bottom }
.state { display: block; }
.lines { display: none; }
.cycles { display: none; }
.linesDelta { display: none; }
.cyclesDelta { display: none; }
a .category { color: white}
table { border-color: #600; border-width: 1px 1px 1px 1px; border-style: solid; }
td { border-color: lightgray; border-width: 1px 1px 1px 1px; border-style: solid; }
</style>
<script type="text/javascript">                                         
$(document).ready(function() {
   $("#matrix").change(function() {
     i = $(this).prop("selectedIndex");
     $(".state").css("display", "none");
     $(".lines").css("display", "none");
     $(".linesDelta").css("display", "none");
     $(".cycles").css("display", "none");
     $(".cyclesDelta").css("display", "none");
     names = ["state", "lines", "linesDelta", "cycles", "cyclesDelta"];
     $("." + names[i]).css("display", "block");
   });
 });
</script>
<title>{{project_name}} Test Suite Output</title></head><body><h3>{{project_name}} Test Suite Output</h3>'''
f.write(s.replace('{{project_name}}', project_name))
f.write('<table>')

def writeSHAHeaderRow(f, headerText):
    f.write('<tr class="category"><td>' + headerText + '</td>')
    for i, h in enumerate(resultsBySHA.keys()):
        f.write('<td>' + ('HEAD<br/>' if i == len(resultsBySHA.keys()) - 1 else '') + ' <a class="category" href="' + baseURL + h + '" title="' + (gitMessages[h] if h in gitMessages else '') + '">' + h + '</a>' + '</td>')
    f.write('</tr>')

#writeSHAHeaderRow(f, 'overall')


assert(current_hash == getSHA())
f.write(get_overall_rows_for_hash(current_hash))

def project_basepath(filename):
    project_dir, ok = shell("git rev-parse --show-toplevel")
    print project_dir, filename
    return filename.replace(project_dir[0].strip() + '/', '')


divWrap = lambda text, cssclass: '<div class="' + cssclass + '">' + text + '</div>'

# test by all policy dimensions
def show_all_tests(sha_hash):
    get_tests = '''SELECT * FROM test_results WHERE githash = ? ORDER BY suite, name, policies DESC'''
    current_suite = None
    s = ''
    for row in db.execute(get_tests, [sha_hash]):
        test_id, name, suite, githash, policies, time_taken, cycles, filesize, output, result = row
        state = 'ok' if result else 'fail'
        s += '<tr><td>%s</td><td>%s</td><td class="%s">%s</td>' % (suite, name, state, result)
        if current_suite != suite:
            current_suite = suite
            # emit row
    return s

f.write(show_all_tests(current_hash))
f.write('</table></body></html>')
f.close()
