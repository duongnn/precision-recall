#!/bin/bash

classpath=../lib/protobuf-java-3.6.1.jar:.
outputdir=./classes

mkdir -p ./classes

# generate .java file from .proto file
echo "compiling protocol buffer wcpprocess.proto ..."
rm -rf ./src/wcpprocess/wcpprocess/protobuf/wcpprocess
protoc -I=./ --java_out=./src/wcpprocess/wcpprocess/protobuf/ ./src/wcpprocess/wcpprocess/protobuf/wcpmessage.proto

# compile .java file
echo "compiling protocol buffer .java file ..."
rm -rf ./classes/*
javac -d ./classes/ ./src/wcpprocess/wcpprocess/protobuf/wcpprocess/protobuf/WcpMessageProtos.java -cp $classpath

# export into jar
echo "creating .jar file for protocol buffer ..."
cd ./classes
jar cvf ../../lib/wcpprotobuf.jar ./wcpprocess/*

# clean content 
rm -rf ./wcpprocess
