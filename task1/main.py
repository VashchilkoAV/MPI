import paramiko
import time
import os

HOST = "head.vdi.mipt.ru"
USER = "b0587201"
PASSWORD = "CfifDfobkrj"

SCR_FILE = "/home/alexander/computerScience/MPI/task1/task1_2.c"
REMOTE_PATH = "/home/" + USER
COMPILER = "mpicc"
ARGS = ""
COMPILED_FILE_NAME = "task1"




def file_cpy(src: str,  new_name=None, dst=REMOTE_PATH):
    transport = paramiko.Transport((HOST, 22))
    transport.connect(username=USER, password=PASSWORD)
    sftp = paramiko.SFTPClient.from_transport(transport)

    remote_path_full = dst + "/" + (src.split("/")[-1] if new_name is None else new_name)

    sftp.put(src, remote_path_full)
    sftp.close()
    transport.close()


def compile(source, executable):
    stdin, stdout, stderr = client.exec_command(f"{COMPILER} {source} -lm -o {executable}")
    stdin.close()
    print(stdout.read().decode())
    print(stderr.read().decode())


JOB_TEMPLATE = "#!/bin/bash\n" \
               "#PBS -l walltime=00:05:00,nodes={nodes}:ppn={ppn}\n" \
               "#PBS -N job\n" \
               "#PBS -q batch\n" \
               "#cd $PBS_O_WORKDIR\n" \
               "mpirun --hostfile $PBS_NODEFILE -np {np} ./{executable} {args}"

RESULT = []


def create_job(nodes=7, ppn=4, np= 1, executable="task", count=10000000000):
    with open("job.sh", "w") as job:
        job.write(JOB_TEMPLATE.format(nodes=nodes, ppn=ppn, executable=COMPILED_FILE_NAME, np=np, args=str(count)))
    file_cpy(os.getcwd() + "/" + "job.sh")
    os.remove("job.sh")


def create_and_run_job(nodes=7, ppn=4, np=1, count=10000000000):
    create_job(nodes, ppn, np, count=count)
    stdin, stdout, stderr = client.exec_command("qsub job.sh")
    stdin.close()
    job_id = stdout.read().decode().replace(".head.vdi.mipt.ru\n", "")
    #print(job_id)

    while True:
        time.sleep(5)
        stdin, stdout, stderr = client.exec_command("ls | grep job.o" + job_id)
        stdin.close()
        if str(stdout.read().decode()).__len__():
            break


    stdin, stdout, stderr = client.exec_command("cat job.o" + job_id)
    stdin.close()

    return stdout.read().decode()


if __name__ == '__main__':
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(hostname=HOST, username=USER, password=PASSWORD)

    stdin: paramiko.ChannelStdinFile;
    stdout: paramiko.channel.ChannelFile
    stderr: paramiko.channel.ChannelStderrFile;


    file_cpy(SCR_FILE, new_name="task1.c")
    compile("task1.c", COMPILED_FILE_NAME)

    for np in range(1, 29):
        r = (np, create_and_run_job(7,4, np, 10000000000))
        #print(r)
        RESULT.append(r)
    for i in RESULT:
        print(i[0], i[1])

    client.close()
