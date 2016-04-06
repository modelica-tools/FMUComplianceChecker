File: README-FMUChecker.txt
Background information for FMI Compliance Checker (FMUChecker) application.

For build instructions see: BUILD-FMUChecker.txt
Licence information is provided in: LICENCE-FMUChecker.txt
Acknowledgements for used software: ACKNOWLEDGEMENTS-FMUChecker.txt
Instructions for how to make a release: RELEASE-NOTES-FMUChecker.txt

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
	- for model exchange FMUs test whether the FMU can be simulated with 
    explicit (forward) Euler method
		- fixed step size is used;
		- no iterations for exact location of state events;
    - for co-simulation FMUs test whether the FMU can be simulated with
    fixed communication step size;
	- log computed solution to csv result file (comma separated values,
    file format is by default compatible with RFC4180,
    http://www.ietf.org/rfc/rfc4180.txt; the header contains variable names,
    first column is time, alternative field separator may be specified as an
    option).
    - can supply numerical input data to the simulation from an CSV file. Input
    start values from FMU binary (not XML) will be used in case no input
    is provided.
    String inputs are currently not supported. The input file is expected to be 
    RFC4180 compatible with following modifications:
      - The file must be in UTF-8 encoding;
      - First line must be a header listing the variable names;
      - First column must be time;
      - Variable names shall match the names in the modelDescription.xml
      file exactly;
      - Variable names may be put in double quotes according to the RFC4180
      rules;
      - Decimal separator for numerical data is a dot;
      - Numerical data shall not be quoted;
      - Boolean data must be represented with 0 for false and 1 for true;
      - Field separator may be any printable ASCII character (detected 
      automatically);
      - End of line may be either CR-LF or just LF.
 - validation log messages are written to stderr. Can be redirected to file.
 
Usage: fmuCheck.<platform> [options] <model.fmu>

Options:

-c <separator>   Separator character to be used in CSV output. Default is ','.

-d               Print also left limit values at event points to the output
                 file to investigate event behaviour. Default is to only print
                 values after event handling.

-e <filename>    Error log file name. Default is to use standard error.

-f               Print all variables to the output file. Default is to only
                 print outputs.

-h <stepSize>    For ME simulation: Decides step size to use in forward Euler.
                 For CS simulation: Decides communication step size for the
                 stepping.
                 Observe that if a small stepSize is used the number of saved
                 outputs will still be limited by the number of output points.
                 Default is to calculated a step size from the number of output
                 points. See the -n option for how the number of outputs is
                 set.

-i <infile>      Name of the CSV file name with input data.

-l <log level>   Log level: 0 - no logging, 1 - fatal errors only, 2 - errors, 
                 3 - warnings, 4 - info, 5 - verbose, 6 - debug.

-m               Mangle variable names to avoid quoting (needed for some CSV
                 importing applications, but not according to the CrossCheck
                 rules).

-n <numSteps>    Maximum number of output points. "-n 0" means output at every
                 step and the number of outputs are decided by the -h option.
                 Observe that no interpolation is used, output points are taken
                 at the steps.
                 Default is 500.

-o <filename>    Simulation result output CSV file name. Default is to use
                 standard output.

-s <stopTime>    Simulation stop time, default is to use information from
                 'DefaultExperiment' as specified in the model description XML.

-t <tmp-dir>     Temporary dir to use for unpacking the FMU.
                 Default is to use system-wide directory, e.g., C:\Temp or /tmp.

-v               Print the checker version information.

-k xml           Check XML only.
-k me            Check XML and ME simulation.
-k cs            Check XML and CS simulation.
                 Multiple -k options add up.
                 No -k option: test XML, simulate ME and CS respectively if
                 supported.

-x               Check XML only. Same as -k xml.

-z <unzip-dir>   Do not create and remove a temp directory but instead use the
                 specified one for unpacking the FMU. The option takes 
                 precendence over -t.


Command line examples:

fmuCheck.win32 model.fmu
        The checker on win32 platform will process 'model.fmu' with default
        options.

fmuCheck.win64 -e log.txt -o result.csv -c , -s 2 -h 1e-3 -l 5 -t . model.fmu 
        The checker on win64 platform will process "model.fmu". The log 
        messages will be saved in log.txt, simulation output in 
        result.csv and comma will be used for field separation in the CSV
        file. The checker will simulate the FMU until 2 seconds with 
        time step 1e-3 seconds. Verbose messages will be generated.
        Temporary files will be created in the current directory.
