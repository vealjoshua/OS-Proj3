Joshua Veal-Briscoe
Project 3
_________________How to Use__________________________________
Please use the commands: 

exe = executable : oss or ./oss

---------------commands---------------

-h or -help  : shows steps on how to use the program 
-s x         : x is the maximum number of slave processes spawned (default 5) 
-l filename  : change the log file name (default test.out)
-t z         : parameter z is the time in seconds when the master will terminate itself (default 20) 

--------------examples--------------

oss -h -help -lfilename.txt -s15 -t14
./oss -h -help -l filename2.txt -s 15 -t 14
./oss -h or -help shows commands and does not run the program.

----------------Log-------------------

Everything is printed to the logfile
I do print some stuff to the screen to know that I am running 100 process
--------------------------

There are a few error's and theses occurue when your abruptly kill all process
These error are not bad beacuse they show that all memeory is dallocated and deleted so user's die and cannot access the Queue's and clock

--------------Running Program-------------------

I do count up the user's running just to see the program is running. 
This way there isn't a blank screen and if the program breaks I know where.
--------------------------------------------------------------------------

-----------------User's Time Alive-------------------------------

I user a rand function to randomzie the time users live
nano seconds are counted 1000 at a time 
Also I tried reaching 2 seconds but its hard finding that right balance
oss -s1 gets closest to getting 2 seconds I start making the wait times less beacuse nano seconds have to start back from 0 

