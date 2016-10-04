#!/bin/sh

# This script will submit the current branch. This will cause the tests to be
# run and a comment will be added to the commit on GitHub with the results of
# the test.
#
# You may submit as many times as you wish; however, _any_ submissions passed
# the due date will be considered late, and graded according to the course
# policy on late homework.
#
# Make sure you have committed all of your changes prior to running the
# script, otherwise they will not be included.

set -e

git branch -f submission HEAD
git push -f origin submission
repo=`git remote -v|sed -ne 's,origin\s.*\(uicsystems/[^/]\+\)\.git (push)$,\1,p'`
hash=`git rev-parse submission`
echo
echo
echo "Test results will be at https://github.com/$repo/commit/$hash"
