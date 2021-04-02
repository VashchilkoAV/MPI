import paramiko
import time
import os

HOST = "head.vdi.mipt.ru"
USER = "b0587201"
PASSWORD = "CfifDfobkrj"

SCR_FILE = "/home/alexander/computerScience/MPI/task1/task1_2.c"


def file_cpy(src: str, dst: str):
    with open(src) as file:
        string = file.read()
        string = string.replace(r'\n', r'\\n')
        string = string.replace(r'"', r'\"')
        stdin, stdout, stderr = client.exec_command(f"echo \"{string}\" > {dst}")
        stdin.close()


def compile(executable):
    stdin, stdout, stderr = client.exec_command(f"mpicc task.c -lm -o {executable}")
    stdin.close()
    print(stdout.read().decode())
    print(stderr.read().decode())


JOB_TEMPLATE = "#!/bin/bash\n" \
               "#PBS -l walltime=00:05:00,nodes={nodes}:ppn={ppn}\n" \
               "#PBS -N job\n" \
               "#PBS -q batch\n" \
               "#cd $PBS_O_WORKDIR\n" \
               "mpirun --hostfile $PBS_NODEFILE -np {np} ./{executable} {count}"

RESULT = []


def create_job(nodes=7, ppn=4, np= 1, executable="task", count=10000000000):
    # os.system(f"echo \"{JOB_TEMPLATE.format(nodes=nodes, ppn=ppn, executable=executable)}\"")
    stdin, stdout, stderr = \
        client.exec_command(f"echo \""
                            f"{JOB_TEMPLATE.format(nodes=nodes, ppn=ppn, executable=executable, np=np, count=count)}\" "
                            f"> job.sh")
    stdin.close()


def create_and_run_job(nodes=7, ppn=4, np=1, count=10000000000):
    create_job(nodes, ppn, np, count=count)
    stdin, stdout, stderr = client.exec_command("qsub job.sh")
    stdin.close()
    out_number = stdout.read().decode().replace(".head.vdi.mipt.ru\n", "")
    time.sleep(5)

    stdin, stdout, stderr = client.exec_command("cat job.o" + out_number)
    stdin.close()

    return stdout.read().decode()


if __name__ == '__main__':
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(hostname=HOST, username=USER, password=PASSWORD)

    stdin: paramiko.ChannelStdinFile;
    stdout: paramiko.channel.ChannelFile
    stderr: paramiko.channel.ChannelStderrFile;


    file_cpy(SCR_FILE, "/home/" + USER +"/task.c")
    compile("task")

    for ppn in range(1, 5):
        if ppn == 1:
            for nodes in range(1, 8):
                RESULT.append((ppn * nodes, create_and_run_job(nodes, ppn)))
                #print(RESULT)
        elif ppn == 2:
            for nodes in range(4, 8):
                RESULT.append((ppn * nodes, create_and_run_job(nodes, ppn)))
                #print(RESULT)
        elif ppn == 3:
            for nodes in range(5, 8):
                RESULT.append((ppn * nodes, create_and_run_job(nodes, ppn)))
                #print(RESULT)
        else:
            for nodes in range(6, 8):
                RESULT.append((ppn * nodes, create_and_run_job(nodes, ppn)))
                #print(RESULT)
    #print("FINALLY")
    for i in RESULT:
        print(i[0], i[1])
    #print(RESULT)

    client.close()
