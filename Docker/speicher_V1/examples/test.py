import psutil

PROCNAME = "simple_put_loop"

for proc in psutil.process_iter():
    if proc.name() == PROCNAME:
        print(proc.pid)
