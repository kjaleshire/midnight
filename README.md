####Midnight - A simple & fast HTTP daemon

Currently only supports GET requests, handled by dispatch blocks.

Alpha until otherwise indicated.

Build (inside 'midnight' directory):

    $ make
    $ ./midnight

####OPTIONS
-h, --help          help information  
-e, --error         error-only logging  
-p, --port          port to listen on (default: 8080)  
-a, --address       IP address to bind to (default: all)  
-d, --docroot       HTML document root (default: ./docroot)  
-q, --quiet         run silently

####CREDITS
Includes Zed Shaw's famous, fast & secure HTTP request parser.
Copyright (c) 2005 Zed Shaw. You can redistribute it and/or modify it under the same terms as Ruby.

####LICENSE
Copyright (c) 2013, Kyle J Aleshire
All rights reserved. Permission is given to redistribute or modify under the same terms as FreeBSD.