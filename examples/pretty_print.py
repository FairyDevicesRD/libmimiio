#!/usr/bin/python
# encoding: utf-8
import json,sys

try:
    while True:
        pj = json.loads(raw_input())
        for obj in pj["response"]:
            sys.stdout.write(obj["result"])
        print ""
except KeyboardInterrupt:
    pj = json.loads(raw_input())
    for obj in pj["response"]:
        sys.stdout.write(obj["result"])
    print ""
    
