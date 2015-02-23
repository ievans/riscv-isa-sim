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

def shell(command): 
    p = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output = [x for x in p.stdout.readlines()]
    retval = p.wait()
    return output, retval

gitMessages = {}
def gitInfo():
    output, retval = shell('git log --pretty=oneline --abbrev-commit')
    if retval == 0:
        for line in output:
            h, msg = (line[:line.index(' ')], line[line.index(' '):].strip())
            gitMessages[h] = msg

gitInfo()

project_name = 'riscv-isa-sim'
project_org = 'riscv-mit'

baseURL = 'https://github.com/%s/%s/commit/' % (project_org, project_name)
baseFileURL = 'https://github.com/%s/%s/blob/master/' % (project_org, project_name)

resultsBySHA = collections.OrderedDict()
resultsBySHA = pickle.load(open('tests.dat'))

# reverse the order of the keys so that the HEAD commit is on the left
# resultsBySHA = collections.OrderedDict(reversed(resultsBySHA.items()))

resultsByTest = {}
for h in resultsBySHA.keys():
    for testName in resultsBySHA[h].keys():
        if testName not in resultsByTest:
            resultsByTest[testName] = {}
        for fileName in resultsBySHA[h][testName].keys():
            if fileName in resultsByTest[testName]:
                resultsByTest[testName][fileName].extend([(h, resultsBySHA[h][testName][fileName])])
            else:
                resultsByTest[testName][fileName] = [(h, resultsBySHA[h][testName][fileName])]

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
<title>{{project_name}} Test Suite Output</title></head><body><h3>{{project_name}} Test Suite Output</h3>
<select id="matrix"><option>correctness</option><option># instructions</option><option># instructions delta</option><option># microseconds</option><option># microseconds delta</option></select>'''
f.write(s.replace('{{project_name}}', project_name))
f.write('<table>')

isCorrect = lambda x: x[1][0][0]

def writeSHAHeaderRow(f, headerText):
    f.write('<tr class="category"><td>' + headerText + '</td>')
    for i, h in enumerate(resultsBySHA.keys()):
        f.write('<td>' + ('HEAD<br/>' if i == len(resultsBySHA.keys()) - 1 else '') + ' <a class="category" href="' + baseURL + h + '" title="' + (gitMessages[h] if h in gitMessages else '') + '">' + h + '</a>' + '</td>')
    f.write('</tr>')

writeSHAHeaderRow(f, 'overall')

# overall
for testName in sorted(resultsByTest.keys()):
    f.write('<tr>')
    f.write('<td>' + testName + '</td>')
    for h in resultsBySHA.keys():
        if not testName in resultsBySHA[h]:
            f.write('<td>-</td>')
            continue
        allFiles = resultsBySHA[h][testName].values()
        total = len(allFiles)
        correct = len(filter(lambda x: x[0][0], allFiles))
        state = 'ok'
        if correct != total:
            state = 'fail'
        f.write('<td class="' + state + '">' + str(correct) + '/' + str(total)  + '</td>')
    f.write('</tr>')

def project_basepath(filename):
    project_dir, ok = shell("git rev-parse --show-toplevel")
    print project_dir, filename
    return filename.replace(project_dir[0].strip() + '/', '')

# drilldown
for testName in sorted(resultsByTest.keys()):
    writeSHAHeaderRow(f, testName)

    for fileName in sorted(resultsByTest[testName].keys()):
        f.write('<tr><td><a href="' + baseFileURL + project_basepath(fileName.replace(',/', '')) + \
                '">' + os.path.basename(fileName) + '</a></td>')
        prevCycles = 0
        prevLines = 0
        for h in resultsBySHA.keys():
            found = None
            linesDelta, cyclesDelta = '', ''
            for item in resultsByTest[testName][fileName]:
                if item[0] == h:
                    found = item
            if found == None: # if there is no data for a SHA, don't output anything
                f.write('<td>-</td>')
                continue
            lines = found[1][0][1][0] if isCorrect(found) else 'fail'
            cycles = found[1][0][1][1] if isCorrect(found) else 'fail'
            if isCorrect(found) and lines != '' and cycles != '':
                linesDelta = ("%+d" % (int(lines) - int(prevLines))).replace('+0', '')
                cyclesDelta = ("%+d" % (int(cycles) - int(prevCycles))).replace('+0', '')
                prevCycles = cycles
                prevLines = lines
            state = 'ok' if isCorrect(found) else 'fail'
            divWrap = lambda text, cssclass: '<div class="' + cssclass + '">' + text + '</div>'
            divWrap2 = lambda text, cssclass, cssclass2: '<div class="' + cssclass + ' ' + cssclass2 + '">' + text + '</div>'
            def classForNumber(x):
                if x == '': return ''
                if x[0] == '+': return 'positive'
                if x[0] == '-': return 'negative'

            f.write('<td class="' + state + '">' + \
                        divWrap(state, "state") +  \
                        divWrap(lines, "lines") + \
                        divWrap2(linesDelta, "linesDelta", classForNumber(linesDelta)) + \
                        divWrap(cycles, "cycles") + \
                        divWrap2(cyclesDelta, "cyclesDelta", classForNumber(cyclesDelta)) + \
                        '</td>')
        f.write('</tr>')

f.write('</table></body></html>')
f.close()
