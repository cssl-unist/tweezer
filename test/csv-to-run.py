#!/usr/bin/env python3

import subprocess, os, datetime, time, asyncio

# Mode : 
#   'B' : Running benchmarks
#   'F' : Fill the workloads
# Fillsize : When choose 'F' mode, set size of benchmarks
#   '16_128' : 16GB with 128 value size. Num = 120000000
#   '16_1024' : 16GB with 1024 value size. Num = 15000000
#   '64_128' : 64GB with 1024 value size. Num = 480000000
#   '64_1024' : 64GB with 128 value size. NUM = 60000000
# Workloads :
#   M(mode) :
#       'F' : fillrandom
#       'E' : use_existing_db
#   'R100_K_V_M" : readrandomwriterandom, readwritepercent = 100, key_size=K, value_size=V
#   'R90_K_V_M" : readrandomwriterandom, readwritepercent = 90, key_size=K, value_size=V
#   'R80_K_V_M" : readrandomwriterandom, readwritepercent = 80, key_size=K, value_size=V
#   'RAN_K_V_M" : seekrandom, key_size=K, value_size=V
# dir : this option specify directory
#   if you using 'F' mode, this option specify generated directory.
#   if you using '_M' as 'E' in workloads, this option specify where you copied from.
# binary : Speicify path to binary that will be used to test.
# block : Specify block size for SSTable.
# cache : Specify block cache size.
# entries : Speicify entry size (num)
# threads : Specify number of threads.



configMap = ['mode','filesize', 'workload', 'dir', 'binary' , 'block', 'cache', 'entries', 'threads']


os.environ['SCONE_HEAP'] = '16G'


def line_to_config(line):
    ls = line.rstrip('\n').split(';')
    ret = {}
    if (not line or line[0] == '#'):
        print('skipping commented: {}', line)
        return {}
    for i in range(0,len(configMap)):
        ret[configMap[i]] = ls[i].strip()
    return ret

def config_to_args(config):
    args = [config['binary'].strip()]
    args += ['--disable_wal=0']
    if (config['mode'] == 'F'):
        args += ['-benchmarks=fillseq']
        #Increase compaction threads 8 for speed
        args += ['-max_background_jobs=8']
        args += ['-use_existing_db=0']
        args += ['-db={}'.format(config['dir'])]
        args += ['-key_size=16']
        if (config['filesize'] == '16_128'):
            args += ['-value_size=128']
            args += ['-num=120000000']
        elif (config['filesize'] == '16_1024'):
            args += ['-value_size=1024']
            args += ['-num=15000000']
        elif (config['filesize'] == '64_128'):
            args += ['-value_size=128']
            args += ['-num=480000000']
        elif (config['filesize'] == '64_1024'):
            args += ['-value_size=1024']
            args += ['-num=15000000']
        args += ['-block_size={}'.format(int(config['block'])*1024)]
        args += ['-cache_size={}'.format(int(config['cache'])*1024*1024)]
        args += ['-histogram']
        args += ['-statistics']
# Benchmark mode
    else:
        option = config['workload'].rstrip('\n').split('_')
        #_M is used
        print(option)
        if(len(option) == 4):
            args += ['-use_existing_db=1']
            os.system('rm -rf ./testing')
            os.system('cp -rf {} ./testing'.format(config['dir']))
            args += ['-db=./testing']
            args += ['-file_opening_threads=8']
            if (option[0] == 'R100'):
                args+= ['-benchmarks=readrandom']
            elif (option[0] == 'R90'):
                args+= ['-benchmarks=readrandomwriterandom']
                args+= ['-readwritepercent=90']
            elif (option[0] == 'R80'):
                args+= ['-benchmarks=readrandomwriterandom']
                args+= ['-readwritepercent=80']
        else:
            if (option[0] == 'R100'):
                args+= ['-benchmarks=fillrandom,readrandom']
            elif (option[0] == 'R90'):
                args+= ['-benchmarks=fillrandom,readrandomwriterandom']
                args+= ['-readwritepercent=90']
            elif (option[0] == 'R80'):
                args+= ['-benchmarks=fillrandom,readrandomwriterandom']
                args+= ['-readwritepercent=80']

        args += ['-key_size={}'.format(int(option[1]))]
        args += ['-value_size={}'.format(int(option[2]))]
        args += ['-block_size={}'.format(int(config['block'])*1024)]
        args += ['-cache_size={}'.format(int(config['cache'])*1024*1024)]
        args += ['-num={}'.format(int(config['entries']))]
        args += ['-threads={}'.format(int(config['threads']))]
        args += ['-histogram'] 
    return args


def dump(res, args, config, sgxtop):
    fname = (str(datetime.datetime.now()) + '.txt').replace(" ","-").replace(":","-")
    with open('output/' + fname, 'w') as f:
        f.write('@@start\n')
        f.write(str(args) + '\n\n')
        f.write(str(config) + '\n\n')
        f.write(res)
        f.write('@@end\n')
        f.write('@@sgxtop-start\n')
        f.write(sgxtop)
        f.write('@@sgxtop-end\n')


csv_name = "config.csv"
progress_name = "progress.csv"

with open(csv_name, 'r') as f:
    lines = f.readlines()
    print(lines)
    config = list(filter(lambda x: x != {}, map(lambda x: line_to_config(x.strip()), lines)))


def ping():
    res = subprocess.run("./ping.txt")

def cleanup():
    untrusted_mem = 'rm  -f /app/untrusted_memory/foo'.split()
    checked_run(untrusted_mem)


def checked_run(args):
    try:
        res = subprocess.check_output(args).decode('utf-8')
        return res
    except Exception as e:
        cleanup()
        ping()
        return 1

def is_sgx(config):
    return config['binary'] != './binary/vanilla'

def start_sgxtop():
    command = '../sgxtop/sgxtop stdout'.split()
    return subprocess.Popen(command, stdout = subprocess.PIPE)

def run_one_config(config):
    args = config_to_args(config)
    print(args)
    if (is_sgx(config)):
        untrusted_mem = 'fallocate -l 4G /app/untrusted_memory'.split()
        checked_run(untrusted_mem)

    sgxtop_process = start_sgxtop() 
    start = time.time()
    print('running @ {}: {}'.format(datetime.datetime.now(), args))
    res = checked_run(args)
    if(res==1):
      return
    end = time.time()
    sgxtop_process.kill()
    sgxtop_log =sgxtop_process.communicate()[0].decode('utf-8')
    print('elapsed: {}'.format(datetime.timedelta(seconds=end-start)))
    c['sec'] = (end-start)
    c['min'] = round((end-start)/60, 2)
    dump(res, args, c, sgxtop_log)
    cleanup()




if __name__ == '__main__':
    print('csv-to-run')

    for c in config:
        run_one_config(c)

    ping()
