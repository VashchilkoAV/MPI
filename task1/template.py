import paramiko
import time
import os

HOST = "head.vdi.mipt.ru"
USER = "b0587218"
PASSWORD = ""

REMOTE_PATH = "/home/" + USER

SCR_FILE = "/home/artemiy/CLionProjects/Draft/main.cpp"
COMPILER = "mpicxx"
ARGS = ""
COMPILED_FILE_NAME = "task"


def file_cpy(src: str,  new_name=None, dst=REMOTE_PATH):
    transport = paramiko.Transport((HOST, 22))
    transport.connect(username=USER, password=PASSWORD)
    sftp = paramiko.SFTPClient.from_transport(transport)

    remote_path_full = dst + "/" + (src.split("/")[-1] if new_name is None else new_name)

    sftp.put(src, remote_path_full)
    sftp.close()
    transport.close()


def compile(source, executable):
    stdin, stdout, stderr = client.exec_command(f"{COMPILER} {source} -std=c++11 -o {executable}")
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


def create_job(nodes=1, ppn=1):
    with open("job.sh", "w") as job:
        job.write(JOB_TEMPLATE.format(nodes=nodes, ppn=ppn, executable=COMPILED_FILE_NAME, np=nodes * ppn, args=ARGS))
    file_cpy(os.getcwd() + "/" + "job.sh")
    os.remove("job.sh")



def create_and_run_job(nodes=1, ppn=1):
    create_job(nodes, ppn)
    stdin, stdout, stderr = client.exec_command("qsub job.sh")
    stdin.close()
    job_id = stdout.read().decode().replace(".head.vdi.mipt.ru\n", "")

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
    client.connect(hostname=HOST, port=22, username=USER, password=PASSWORD)

    stdin: paramiko.ChannelStdinFile;
    stdout: paramiko.channel.ChannelFile
    stderr: paramiko.channel.ChannelStderrFile;

    file_cpy(SCR_FILE, new_name="task.cpp")
    compile("task.cpp", COMPILED_FILE_NAME)

    for ppn in range(1, 5):
        if ppn == 1:
            for nodes in range(1, 8):
                RESULT.append((ppn * nodes, create_and_run_job(nodes, ppn)))
                print(RESULT)
        elif ppn == 2:
            for nodes in range(4, 8):
                RESULT.append((ppn * nodes, create_and_run_job(nodes, ppn)))
                print(RESULT)
        elif ppn == 3:
            for nodes in range(5, 8):
                RESULT.append((ppn * nodes, create_and_run_job(nodes, ppn)))
                print(RESULT)
        else:
            for nodes in range(6, 8):
                RESULT.append((ppn * nodes, create_and_run_job(nodes, ppn)))
                print(RESULT)

    print("FINALLY")
    print(RESULT)

    client.close()
