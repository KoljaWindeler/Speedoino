#!/bin/bash
VERSION=`git --git-dir=$1../.git/ describe --tags 2>/dev/null |sed  's/Speedoino_//'`;
echo "#ifndef VERSION_H_
#define VERSION_H_
    #define GIT_REV \"$VERSION\"
#endif" > $1/version.h
