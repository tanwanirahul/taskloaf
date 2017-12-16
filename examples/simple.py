import taskloaf as tsk

async def submit(w):
    await tsk.task(w, lambda w: print('hI'))

if __name__ == "__main__":
    tsk.run(submit)
    tsk.cluster(1, submit)
    tsk.cluster(1, submit, runner = tsk.mpirun)
