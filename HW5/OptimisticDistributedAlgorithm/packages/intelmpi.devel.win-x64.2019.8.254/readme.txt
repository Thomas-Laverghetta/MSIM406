-----------------------------------------------------
Using NuGet* MPI packages in Microsoft Visual Studio*
-----------------------------------------------------

Local usage (one node)

1.	Link Intel MPI Library in the project's properties:
	Go to "Properties->intelmpi.devel.win-x64->Linkage" and choose the "Dynamic" value.
	
2.	Specify the path to mpiexec in Properties -> Debugging ->Command 
	Type: <path_to_Visual_Studio_project>\packages\<redist_package_folder>\runtimes\win-x64\bin\mpiexec.exe 
	
3. 	Specify the mpiexec parameters in Properties -> Debugging ->CommandArguments: 
	Type: -localonly -n <count of ranks> "$(TargetPath)"
	For example:
	    -localonly -n 2 "$(TargetPath)"


Multinode usage

General steps

	1.	Make sure to place the Visual Studio project and MPI packages on all 
		the nodes you want to use. The paths must be the same.
		
	2.	Install the hydra service on each node (run cmd as administrator): 
		<path_to_Visual_Studio_project>\packages\<redist_package_folder>\runtimes\win-x64\bin\hydra_service.exe --install
		
	3.	Register your Windows* user credentials to enable the process 
		manager to launch MPI jobs. Credentials are encrypted and stored in the 
		registry:
		<path_to_Visual_Studio_project>\packages\<redist_package_folder>\runtimes\win-x64\bin\mpiexec.exe --register
		
	4.	Link Intel MPI Library in the project's properties:
		Go to "Properties->intelmpi.devel.win-x64->Linkage" and choose the "Dynamic" value.
		
	5.	Specify the path to mpiexec in the project's properties (Properties -> Debugging ->Command): 
		<path_to_Visual_Studio_project>\packages\<redist_package_folder>\runtimes\win-x64\bin\runtimes\win-x64\native\bin\mpiexec.exe

If there is no shared path between nodes:
	1.	Specify the mpiexec parameters in Properties -> Debugging ->CommandArguments: 
			-hosts <host1>,<host2>,...,<hostN> -n <n> -ppn <ppn> "$(TargetPath)"
		Where:
			<n> 	-	number of nodes
			<ppn> 	- 	number of processes per node
		For example:	-hosts hostname1,hostname2 -n 4 -ppn 2 "$(TargetPath)"
		
	2.	Compile your application and copy it to other nodes (use the same path 
		for all nodes). 
		
	3.	Run the application.
	
If there is a shared path between nodes:
	1.	Specify the mpiexec parameters (Properties -> Debugging ->CommandArguments):
			-hosts <host1>,<host2>,...,<hostN> -n <n> -ppn <ppn> <app.exe> 
		Where: 
			<n> 	-	number of nodes
			<ppn> 	- 	number of processes per node
			<app.exe> - path to your application
		For example:
			-hosts hostname1,hostname2 -n 4 -ppn 2 \\hostname2\VS_project\x64\Release\application.exe
			
	2.	Compile the application on one node. In our case, it is hostname2
	
	3.	Run the application

