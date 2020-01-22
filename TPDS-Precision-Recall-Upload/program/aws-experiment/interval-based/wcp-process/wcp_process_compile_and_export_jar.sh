#!/bin/bash

## compiling protocol buffer stuff
bash protobuf_compile_and_export_jar.sh

## compiling wcp processes
libdir=../../../lib
#classpath=$libdir/commons-codec-1.6.jar:$libdir/guava-14.0.1.jar:$libdir/jopt-simple-4.6.jar:$libdir/wcpprotobuf.jar:$libdir/predicatedetectionlib.jar:$libdir/protobuf-java-3.6.1.jar:.
classpath=$libdir/*:.
outputdir=../../classes

mkdir -p ../../classes

# compile .java file
cd ./src/wcpprocess	# It is important that we move to this directory, so that import package statement matches with package name in the code
					# We also need to provide current directory (.) in the classpath too.
					# Otherwise, says if we stay in wcp-process directory (containing this script), we will get the error 
					#    package wcpprocess.helpers does not exist
					# when compiling the WcpProcess.java

echo "compiling wcpprocess ..."
javac -d $outputdir ./wcpprocess/helpers/*.java -cp $classpath
javac -d $outputdir ./wcpprocess/process/*.java -cp $classpath

# export into jar
echo "creating .jar file for wcpprocess ..."
cd $outputdir
jar cvf wcpprocess.jar ./wcpprocess/*
mv wcpprocess.jar ../../lib/
