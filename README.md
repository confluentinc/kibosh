# Kibosh

Kibosh is a fault-injecting filesystem for Linux.

Kibosh acts as a pass-through layer on top of existing filesystems.  Usually,
it simply forwards each request on to the underlying filesystems, but it can be
configured to inject arbitrary faults.

# Environment

Kibosh runs on Linux.  We do not currently support Mac or Windows.

If running in a container environment, the container needs to be run with 
additional options giving Kibosh access to FUSE.

    --cap-add SYS_ADMIN     # this allows Kibosh to mount a FUSE filesystem
    --device /dev/fuse      # this exposes Kibosh to the FUSE device
    
Alternatively, the container can be run with --privileged flag which grants
Kibosh all the permissions it needs.

    $ docker run --privileged -ti ubuntu /bin/bash

# Build Requirements

In order to build Kibosh, you must have the CMake build system installed, and a
C compiler.  We also depend on the development libraries for FUSE.  Pkg-config is
used by configure script to gather paths of required libraries.

Dependencies can be installed with the following commands.  Run configure before
make to verify all required dependencies are installed.

    # install cmake
    $ apt-get install cmake
    
    # install libfuse
    $ apt-get install libfuse-dev
    
    # install fuse (for fusermount)
    $ apt-get install fuse
    
    # install pkg-config
    $ apt-get install pkg-config

# Building

It is recommended to build in a separate directory from the source directory.

    $ mkdir build
    $ cd build
    $ ../configure
    $ make

To run the tests for Kibosh, type:

    $ make test

# Running

To run Kibosh, you supply a mirror as the mount point as the first argument, 
and a target directory which you would like to mirror as the second argument.

    $ ./kibosh <mirror_dir> --target <target_dir>
    
Here is an example to run Kibosh with more options:

    $ ./kibosh --target /mnt/kibosh/target --pidfile /mnt/kibosh/log/pidfile --log /mnt/kibosh/log/kibosh.log /mnt/kibosh/mirror

For more usage information, try:

    $ ./kibosh -h

# Injecting Faults

Faults are injected by writing JSON to the control file.  The control file is a
virtual file which does not appear in directory listings, but which is used to
control the filesystem behavior.

Example:

    # Mount the filesystem.
    $ ./kibosh /kibosh_mnt --target /mnt

    # Verify that there are no faults set, this shows current fault JSON.
    $ cat /kibosh_mnt/kibosh_control
    {"faults":[]}

    # Inject new faults:
    # add unreadable fault
    $ echo '{"faults":[{"type":"unreadable", "prefix":"", "suffix":"", "code":5}]}' > /kibosh_mnt/kibosh_control
    
    # add unwritable fault
    $ echo '{"faults":[{"type":"unwritable", "prefix":"", "suffix":"", "code":5}]}' > /kibosh_mnt/kibosh_control
    
    # add read_delay fault
    $ echo '{"faults":[{"type":"read_delay", "prefix":"", "suffix":"", "delay_ms":1000, "fraction":1.0}]}' > /kibosh_mnt/kibosh_control
    
    # add write_delay fault
    $ echo '{"faults":[{"type":"write_delay", "prefix":"", "suffix":"", "delay_ms":1000, "fraction":1.0}]}' > /kibosh_mnt/kibosh_control

    # add read_corrupt fault
    $ echo '{"faults":[{"type":"read_corrupt", "prefix":"", "suffix":"", "mode":1000, "fraction":0.5, "count":-1}]}' > /kibosh_mnt/kibosh_control
    
    # add write_corrupt fault
    $ echo '{"faults":[{"type":"write_corrupt", "prefix":"", "suffix":"", "mode":1000, "fraction":0.5, "count":-1}]}' > /kibosh_mnt/kibosh_control
    
    # Remove all faults.
    $ echo '{"faults":[]}' > /kibosh_mnt/kibosh_control

# Unmount Kibosh

    # fuse needs to be installed, use sudo if necessary.
    $ fusermount -u <mirror_dir>

# License

Kibosh is licensed under the Apache 2.0 license.  See LICENSE for details.

# Contact

Colin McCabe <colin@cmccabe.xyz>
