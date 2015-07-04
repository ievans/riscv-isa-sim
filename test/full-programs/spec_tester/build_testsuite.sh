#!/bin/bash
# Note: assumes that you have already created config file spike.cfg in spec config directory

set -e
set -v

function usage {
    echo "usage: build_testsuite.sh <spec path>"
    exit 0
}

if [[ "$#" -ne 1 ]] ; then
    usage
fi

# delete old run directory and make new one
rm -rf run/
mkdir run
outpath=$(pwd)/run

cd $1
source shrc

cd benchspec/CPU2006
# loop over all tests
for testpath in $(pwd)/* ; do
    [ -d "${testpath}" ] || continue # if not a directory, skip
    dirname="$(basename "${testpath}")"
    testname=${dirname/*\./}
    number=${dirname/\.*/}
    cd $testpath

    # some tests are weird
    exename=$testname
    if [[ "$exename" == "sphinx3" ]] ; then
        exename="sphinx_livepretend"
    fi
    if [[ "$exename" == "xalancbmk" ]] ; then
        exename="Xalan"
    fi
    output_exename="${exename}_base.gcc43-64bit"

    # clean out old outputs
    rm -rf build/
    mkdir build
    rm -rf run/
    mkdir run

    # setup output directories
    runspec --fake --loose --size test --tune base --config spike $number > /dev/null

    # build executable
    cd build
    builddir=""
    # there should be only one directory here because we cleaned it out earlier
    for path in $(pwd)/*; do
        [ -d "${path}" ] || continue # if not a directory, skip
        builddir="$(basename "${path}")"
    done
    cd $builddir
    specmake clean
    specmake -j > /dev/null

    # get the output binary
    cd $testpath
    cd run
    rundir=""
    for path in $(pwd)/*; do
        [ -d "${path}" ] || continue # if not a directory, skip
        rundir="$(basename "${path}")"
    done
    cd $testpath
    cp "build/${builddir}/${exename}" "run/${rundir}/${output_exename}"

    # create script to run the test
    cd run/$rundir
    specinvoke -n > run_test.sh
    printf "#!/bin/bash\ncd \$(dirname \$0)\n" | cat - run_test.sh > /tmp/out && mv /tmp/out run_test.sh
    chmod +x run_test.sh

    # copy run directory to the output folder
    cd ..
    mkdir $outpath/$number
    cp -r $rundir $outpath/$number/

    # cleanup build files
    cd $testpath
    rm -rf build/
    mkdir build
    rm -rf run/
    mkdir run
done
