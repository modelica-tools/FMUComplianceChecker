File: README-FMUChecker.txt
Background information for FMI Compliance Checker (FMUChecker) application.

For build instructions see: BUILD-FMUChecker.txt
Licence information is provided in: LICENCE-FMUChecker.txt
Acknowledgements for used software: ACKNOWLEDGEMENTS-FMUChecker.txt

The FMI Compliance Checker is intended for
validation of FMU 1.0 and 2.0 compliance to the standard specification
as published at <http://www.fmi-standard.org>

The basic features include:
 - automatic unzipping into a temporary directory;
 - checking of XML model description
	- for syntax errors;
	- for correct order of elements and correct cardinality or relations;
	- for correct cross-references;
	- for semantic consistency;
 - validation of binary FMUs compiled for "standard32/default" platform for 
	Windows (.dll), Linux (.so) and Mac OS (.dylib).
	- loading of the binary module;
	- checking whether all required functions are available
	- test whether the FMU can be simulated with explicit (forward) 
	Euler method
		- fixed step size is used;
		- no iterations for exact location of state events;
	- log computed solution to csv result file (comma separated values;
	first row variable names, first column time).
 - validation log messages are written to stderr. Can be redirected to file.
 
Usage: fmuCheck.<platform> [options] <model.fmu>

Options:

-c <separator>   Separator character to be used in CSV output. Default is ';'.

-e <filename>    Error log file name. Default is to use standard error.

-h <stepSize>    Step size to use in forward Euler. Takes preference over '-n'.

-i               Print enums and booleans as integers (default is to print item 
				names, true and false).

-l <log level>   Log level: 0 - no logging, 1 - fatal errors only,
                 2 - errors, 3 - warnings, 4 - info, 5 - verbose, 6 - debug.

-n <num_steps>   Number of steps in forward Euler until time end.
                 Default is 100 steps simulation between start and stop time.

-o <filename>    Simulation result output file name. Default is to use standard 
				output.

-s <stopTime>    Simulation stop time, default is to use information from
                'DefaultExperiment' as specified in the model description XML.

-t <tmp-dir>     Temporary dir to use for unpacking the FMU.
                 Default is to use system-wide directory, e.g., C:\Temp.

-x               Check XML and stop. Default is to load the DLL and simulate
                 after this.

-z <unzip-dir>   Do not create and remove temp directory but use the specified 
				one for unpacking the FMU. The option takes precendence over -t.
				This option is specifically intended for debugging. The unzipped
				files are not deleted and DLL is not unloaded making it possible
				for memory debugger to produce detailed repports.

Command line examples:

fmuCheck.win32 model.fmu 
        The checker on win32 platform will process "model.fmu"  with default 
		options.
                
fmuCheck.win64 -e log.txt -o result.csv -c , -s 2 -h 1e-3 -l 5 -t . model.fmu 
        The checker on win64 platform will process "model.fmu". The log 
        messages will be saved in log.txt, simulation output in 
        result.csv and comma will be used for field separation in the CSV
        file. The checker will simulate the FMU until 2 seconds with 
        time step 1e-3 seconds. Verbose messages will be generated.
        Temporary files will be created in the current directory.
