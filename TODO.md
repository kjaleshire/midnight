Things still to work out towards a 2.0 release:

* implement command line flags/config file reading so we can pass in options (docroot, port/address, logging level, worker thread #, etc.)
* GCD queues for logging facility (at least)
* use worker threads spawned from outset
    * really ambitious? use GCD queues.
* add method for signalling thread shutdown on QUIT
* add daemonizing capability