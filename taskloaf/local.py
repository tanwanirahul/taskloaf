import os
import time
import multiprocessing
import asyncio

import taskloaf.worker
from taskloaf.serialize import loads, dumps

class LocalComm:
    def __init__(self, local_queues, addr):
        self.local_queues = local_queues
        self.addr = addr
        assert(0 <= addr < len(self.local_queues))

    def send(self, to_addr, data):
        # start = time.time()
        D = dumps(data)
        # T = time.time() - start
        self.local_queues[to_addr].put(D)
        # print('addr: ', self.addr, 'to: ', to_addr, ' MB: ', len(D) / 1e6, T)

    def recv(self, w):
        if self.local_queues[self.addr].empty():
            return None
        # import time
        # start = time.time()
        out = loads(w, self.local_queues[self.addr].get())
        # T = time.time() - start
        # print('recv: ', T)
        return out

def localrun(n_workers, f, pin = True):
    try:
        p = multiprocessing.Pool(n_workers)
        manager = multiprocessing.Manager()
        qs = [manager.Queue() for i in range(n_workers)]
        args = [(dumps(f), i, qs, pin) for i in range(n_workers)]
        fut = p.starmap_async(localstart, args)
        return fut.get()[0]
    finally:
        p.close()
        p.join()

def localstart(f, i, qs, pin):
    try:
        if pin:
            os.system("taskset -p -c %d %d" % ((i % os.cpu_count()), os.getpid()))
        c = LocalComm(qs, i)
        return loads(None, f)(c)
    except Exception as e:
        import traceback
        traceback.print_exc()
        raise e
