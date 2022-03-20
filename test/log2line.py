#!/usr/bin/env python3

import re, ast, os, argparse

# 'ready -> 'config' -> 'progress' -> 'sgxtop' -> 'ready'

configMap = ['workload', 'val_size', 'key', 'block', 'cache', 'directio',
        'entries', 'task','db']


parser = argparse.ArgumentParser()
parser.add_argument("--raw", dest='raw', action='store_true')
args = parser.parse_args()

def genOneData(linesIn, idx):
    state = 'config'
    ret = {}
    next_i = idx
    max_epc = 0
    max_va = 0
    start_ts = 0
    for i in range(idx, len(linesIn)):
        line = linesIn[i]
        next_i = i+1
        if (state == 'ready'):
            if (line == '@@start'):
                state = 'config'
            continue
        elif (state == 'config'):
            if (len(line) > 1 and line[0] == '{' and line[-1] == '}'):
                d = ast.literal_eval(line)
                ret['config'] = d
                state = 'progress'
            continue
        elif (state == 'progress'):
            sp = line.split()
            if (len(sp) > 0 and (sp[0] == 'readrandomwriterandom' or sp[0] ==
            'readrandom' or sp[0] == 'seekrandom')):
                ret['throughput'] = int(sp[4])
                ret['latency'] = float(sp[2])
            elif (len(sp) > 0 and sp[0] == 'RawSize:'):
                ret['RawSize'] = float(sp[1])
            elif (len(sp) > 0 and 'throughput' in ret and sp[0] == 'Percentiles:'):
                ret['P50'] = float(sp[2])
                ret['P99'] = float(sp[6])
                ret['P99.9'] = float(sp[8])

            elif (line == '@@end'):
                state = 'sgxtop'
                next_i = i + 1
            continue
        elif (state == 'sgxtop'):
            if (line == '@@sgxtop-start'):
                state = 'sgxtop-progress'
            continue
        elif (state == 'sgxtop-progress'):
            if (line == '@@sgxtop-end'):
                ret['max_epc'] = max_epc
                ret['max_va'] = max_va
                break
            else:
                record = list(map(lambda x: x.strip(), line.split(';')))
                if (len(record) < 7 or record[0] == 'timeStamp'):
                    continue
                if (start_ts == 0):
                    start_ts = int(record[0])
                epc = int(record[5]);
                va = int(record[6]);
                if (epc > max_epc):
                    max_epc = epc
                if (va > max_va):
                    max_va = va

                    

            continue
            

        else:
            Exception("")
    return ret, next_i 


def consumeLog(fname):
    ret = []
    with open(fname, 'r') as f:
        lines = list(map(lambda x: x.rstrip('\n'), f.readlines()))
    next_i = 0
    while True:
        print("consume: {}".format(next_i))
        newret, ni = genOneData(lines, next_i)
        if (ni <= next_i + 1):
            break
        next_i = ni
        newret['filename'] = fname
        ret += [newret]
    return ret
files = sorted(os.listdir('output'))
results = []
for f in files:
    print('pricessing: {}'.format(f))
    if (f[0] == '.'):
        continue
    
    newRes = consumeLog('output/{}'.format(f))
    if(newRes != []):
        results += newRes

pretty_format = '{:15s} | {:10s} | {:10s} | {:10s} | {:10s} | {:10s} | {:10s} | {:10s} | {:10s}| {:15s}'
def header_str():
    return pretty_format.format(
            'workload',
            'filesize',
            'entries',
            'block',
            'latency',
            'throughput',
            'min',
            'cache',
            'threads',
            'binary',
            )
def result_to_string(r):
    return pretty_format.format(
            r['config']['workload'],
            r['config']['filesize'],
            r['config']['entries'],
            r['config']['block'],
            str(r['latency']),
            str(r['throughput']),
            str(r['config']['min']),
            str(r['config']['cache']),
            str(r['config']['threads']),
            str(r['config']['binary']),
            )

if (not args.raw):
    print(header_str())
    for r in results:
        print(result_to_string(r))
else:
    for r in results:
        print(r)

    if (len(results) > 0):
        bests = {}
        for r in results:
            if (int(r['config']['entries']) != 10000000):
                continue
            if (r['config']['task'] not in bests):
                bests[r['config']['task']] = r
            elif (bests[r['config']['task']]['throughput'] < r['throughput']):
                bests[r['config']['task']] = r
    print('-' * 50)
    for b in bests:
        print(bests[b])
