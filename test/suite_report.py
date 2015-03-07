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

def getSHA(): 
    return shell('git rev-parse --short HEAD')[0][0].strip()

project_name = 'riscv-isa-sim'
project_org = 'riscv-mit'

baseURL = 'https://github.com/%s/%s/commit/' % (project_org, project_name)
base_file_url = 'https://github.com/%s/%s/blob/master/' % (project_org, project_name)

conn = sqlite3.connect('tests.sqlite')
db = conn.cursor()

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
.divider { background-color: black; color: white; vertical-align: bottom }
.state { display: block; }
.lines { display: none; }
.cycles { display: none; }
.linesDelta { display: none; }
.cyclesDelta { display: none; }
a .category { color: white}
a .ok { color: white }
a .fail { color: white }
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


def write_row_for_git_commit(git_hash, git_message, is_head):
    f.write('<table>')
    f.write(write_git_header_row(git_hash, git_message, is_head))
    f.write(get_overall_rows_for_hash(git_hash))
    f.write(show_all_tests(git_hash))
    f.write('</table>')

def write_git_header_row(git_hash, git_message, is_head):
    s = ''
    s += '<tr class="category">'
    s += '<td>' + ('HEAD<br/>' if is_head else '') + ' <a class="category" href="' + baseURL + git_hash + '" title="' + (git_message) + '">' + git_hash + '</a>' + '</td>'
    s += '</tr>'
    return s

def project_basepath(filename):
    project_dir, ok = shell("git rev-parse --show-toplevel")
    return filename.replace(project_dir[0].strip() + '/', '')

def current_dir_basepath(filename):
    return filename.replace(os.getcwd(), '')

def project_url(filename):
    return base_file_url + project_basepath(filename)

def get_overall_rows_for_hash(sha_hash):
    overall_query = '''SELECT suite, policies, SUM(result), COUNT(result) FROM test_results WHERE githash = ? GROUP BY suite, policies'''
    s = ''
    s += make_tr(['suite', 'policies', 'results'], row_class = 'divider')
    for row in db.execute(overall_query, [sha_hash]):
        suite, policies, success, total = row
        print(row)
        s += '<tr>'
        s += '<td>%s</td><td>%s</td>' % (suite, policies)
        state = 'ok' if success == total else 'fail'
        s += '<td class="' + state + '">' + str(success) + '/' + str(total)  + '</td>'
        s += '</tr>'
    return s
    
divWrap = lambda text, cssclass: '<div class="' + cssclass + '">' + text + '</div>'

def make_tr(items, row_class = ''):
    return ('<tr class="%s">' % row_class) + ''.join(['<td>%s</td>' % i for i in items]) + '</tr>'

# test by all policy dimensions
def show_all_tests(sha_hash):
    get_tests = '''SELECT * FROM test_results WHERE githash = ? ORDER BY suite, name, policies DESC'''
    current_suite = None
    s = ''
    s += make_tr(['suite', 'filename', 'policies', 'result'], row_class = 'divider')
    for row in db.execute(get_tests, [sha_hash]):
        test_id, name, suite, githash, policies, time_taken, cycles, filesize, output, golden, result = row
        state = 'ok' if result else 'fail'
        s += '<tr><td>%s</td><td>%s</td><td>%s</td><td class="%s"><a class="%s" href="%s">%s</a></td>' % \
             (suite, project_basepath(name), policies, state, state, current_dir_basepath(golden), state)
        if current_suite != suite:
            current_suite = suite
            # emit row
    return s



gitInfo()
current_hash = gitMessages.keys()[0]
assert(current_hash == getSHA())

for k, v in gitMessages.items():
    write_row_for_git_commit(k, v, k == current_hash)

f.write('</body></html>')
f.close()
