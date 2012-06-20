File: README-FMUChecker.txt
Background information for FMI Compliance Checher (FMUChecker) application.

For build instructions see: BUILD-FMUChecker.txt
Licence information is provided in: LICENCE-FMUChecker.txt
Acknowledgements for used software: ACKNOWLEDGEMENTS-FMUChecker.txt

The FMI Compliance Checker is intended for
validation of FMU 1.0 (and later 2.0) compliance to the standard specification
as published at <http://functional-mockup-interface.org>

The basic features include:
 - automatic unzipping into a temporary directory;
 - checking of XML model description
	- for syntax errors;
	- for correct order of elements and correct cardinality or relations;
	- for correct cross-references;
	- for semantic consistency;
 - validation of binary FMUs compiled for "standard32" platform for 
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

-l <log level>   Log level: 0 - no logging, 1 - fatal errors only,
                 2 - errors, 3 - warnings, 4 - info, 5 - verbose, 6 - debug.

-n <num_steps>   Number of steps in forward Euler until time end.
                 Default is 100 steps simulation between start and stop time.

-o <filename>    Output file name. Default is to use standard output.

-s <stopTime>    Simulation stop time, default is to use information from
                'DefaultExperiment' as specified in the model description XML.

-t <tmp-dir>     Temporary dir to use for unpacking the FMU.
                 Default is to use system-wide directory, e.g., C:\Temp.

-x               Check XML and stop. Default is to load the DLL and simulate
                 after this.
