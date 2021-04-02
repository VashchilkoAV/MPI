import time
import os
import subprocess


COMPILER = "mpicc"
ARGS = ""
CNAME = "main.c"
RNAME = "task1"

def compile(source, executable):
    output = subprocess.run("{COMPILER} {source} -lm -o {executable}".format(COMPILER=COMPILER, source=source, executable=executable), shell=True, text=True, capture_output=True)
    print(output.stdout.strip())
    print(output.stderr.strip())


JOB_TEMPLATE = "#!/bin/bash\n" \
               "#PBS -l walltime=00:05:00,nodes={nodes}:ppn={ppn}\n" \
               "#PBS -N job\n" \
               "#PBS -q batch\n" \
               "#cd $PBS_O_WORKDIR\n" \
               "mpirun --hostfile $PBS_NODEFILE -np {np} ./{executable} {args}"

RESULT = []


def create_job(nodes=7, ppn=4, np= 1, executable=RNAME, count=10000000000):
    with open("job.sh", "w") as job:
        job.write(JOB_TEMPLATE.format(nodes=nodes, ppn=ppn, executable=RNAME, np=np, args=str(count)))


def create_and_run_job(nodes=7, ppn=4, np=1, count=10000000000, executable = RNAME):
    create_job(nodes, ppn, np, count=count)
    output = subprocess.run(f"qsub job.sh", shell=True, text=True, capture_output=True)
    job_id = output.stdout.replace(".head.vdi.mipt.ru\n", "")
    #print(job_id)

    while True:
        time.sleep(5)
        output = subprocess.run(f"ls | grep job.o" + job_id, shell=True, text=True, capture_output=True)
        if str(output.stdout.__len__()):
            break


    output = subprocess.run(f"cat job.o" + job_id, shell=True, text=True, capture_output=True)

    return output.stdout


if __name__ == '__main__':
    compile(CNAME, RNAME)

    for np in range(1, 29):
        r = (np, create_and_run_job(7,4, np, 10000000000))
        print(r)
        RESULT.append(r)
    for i in RESULT:
        print(i[0], i[1])

    client.close()