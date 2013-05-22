Things still to work out towards a 2.0 release:

* implement command line flags/config file reading so we can pass in options (docroot, port/address, logging level, worker thread #, etc.)
* add daemonizing capability
* keep-alive using separate event queues for each thread using timer technique in libev documentation