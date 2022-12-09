@(#) $Header: /cvsroot/nsnam/ns-2/README,v 1.5 2000/11/02 22:46:37 johnh Exp $

ns has been maintained by the

MASH Research Group (University of California, Berkeley,
<http://www-mash.cs.berkeley.edu/ns/>)

the VINT project (a collaboration among USC/ISI, Xerox PARC, LBNL,
and UCB, <http://www.isi.edu/nsnam/vint>)

and the CONSER project <http://www.isi.edu/conser/>.


This directory contains a source code or binary distribution of
the ns-2 Network Simulator.

Ns should configure and build on Unix systems with GNU autoconfigure.
It should also build on MS-Windows systems; see INSTALL.WIN32 for details.

Additional information:

QUICK START:  try
	./configure; make; ./validate

DOWNLOADING AND BUILDING NS:
	<http://www.isi.edu/nsnam/ns/ns-build.html>

INSTALLATION PROBLEMS AND BUG FIXES:
	<http://www.isi.edu/nsnam/ns/ns-problems.html>

DOCUMENTATION:
	ns maual: <http://www.isi.edu/nsnam/ns/doc> 

	<http://www.isi.edu/nsnam/ns>
	(includes notes about the limitations of model, debugging tips,
	and a >50 page design overview/tutorial)

LATEST INFORMATION:
	<http://www.isi.edu/nsnam/ns>


If you find ns useful, you may also wish to look at nam, the network
animator <http://www.isi.edu/nsnam/nam>.


## Changes for Qanat 

Most of the added code for simulating Qanat has been added to the my-topology and the my-queue directories. Minor changes have been made to NS2 base, but doesn't change the default functionality of the code. 

To run the experiments, the following steps should be followed: 

* Install ns-allinone-2.34 from [sourceforge](https://sourceforge.net/projects/nsnam/files/allinone/ns-allinone-2.34/).

* Remove the ns-2.34 from the allinone version and clone the current repository in its place. 

* run ./install in ns-allinone-2.34 directory. This will install NS2 and all of its dependencies on the system. 

* The experiment descriptions can be found in the [run_all_experiment.py](https://github.com/FaridZandi/ns-2.34/blob/master/ns-test/run_all_experiment.py) file. 

* Each of the configs in the main function specifies the settings that the script will sweep through. After each experiment is done, the important metrics are calculated from the logs and stored. After all the runs for a certain setting are done, the results will be aggregated in the summary.csv file. 

* When the experiments are finishe, the [plot_summary_all.sh](https://github.com/FaridZandi/ns-2.34/blob/master/ns-test/processing/plot_summary_all.sh) should be run to generate the plots. The plots can be found in the plots directory. 

* To select the desired plots that are included in the paper, the [select_paper_plots_noms.sh](https://github.com/FaridZandi/ns-2.34/blob/master/ns-test/processing/select_paper_plots_noms.sh) can be run, which will copy a handful of the plots to the `paper-plots-noms` directory. 