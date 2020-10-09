import subprocess
import time

f = open("Ring_ProcessTime.csv", "w")

# for different number of processors (0 to 5)
for i in range(5):
    # Command args and path
    cmd = "mpiexec -n " + str(2 ** i) + " .\\x64\\Debug\\GravitationalPhysics.exe"
    print(cmd)
    for bat in range(30):
        # starting timer and running simulation
        start = time.time()
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE, creationflags=0x08000000)
        process.wait()
        end = time.time()

        # displaying 
        f.write(str(i**2) + "," + str(end-start) + "\n")
        print(end - start)