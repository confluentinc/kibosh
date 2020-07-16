# Kibosh

Kibosh is a fault-injecting filesystem for Linux.

Kibosh acts as a pass-through layer on top of existing filesystems.  Usually,
it simply forwards each request on to the underlying filesystems, but it can be
configured to inject arbitrary faults.

# Build Requirements

Kibosh runs on Linux.  We do not currently support Mac or Windows.

In order to build Kibosh, you must have the CMake build system installed, and a
C compiler.  We also depend on the development libraries for FUSE.

# Building

It is recommended to build in a separate directory from the source directory.

    $ mkdir build
    $ cd build
    $ ../configure
    $ make

To run the tests for Kibosh, type:

    $ make test

# Running

To run Kibosh, you supply the mount point as the first argument, and the
directory which you would like to mirror as the second argument.

    $ ./kibosh /kibosh_mnt --target /mnt

For more usage information, try:

    $ ./kibosh -h

# Injecting Faults

Faults are injected by writing JSON to the control file.  The control file is a
virtual file which does not appear in directory listings, but which is used to
control the filesystem behavior.

Example:

    # Mount the filesystem.
    $ ./kibosh /kibosh_mnt --target /mnt

    # Verify that there are no faults set
    $ cat /kibosh_mnt/kibosh_control
    {"faults":[]}

    # Configure a new fault.
    # add unreadable fault
    $ echo '{"faults":[{"type":"unreadable", "prefix":"", "file_type":"", "code":5}]}' > /kibosh_mnt/kibosh_control
    # add unwritable fault
    $ echo '{"faults":[{"type":"unwritable", "prefix":"", "file_type":"", "code":5}]}' > /kibosh_mnt/kibosh_control
    # add read_delay fault
    $ echo '{"faults":[{"type":"read_delay", "prefix":"", "file_type":"", "delay_ms":1000, "fraction":1.0}]}' > /kibosh_mnt/kibosh_control
    # add read_corrupt fault
    $ echo '{"faults":[{"type":"read_corrupt", "prefix":"", "file_type":"", "mode":1000, "fraction":0.5, "count":-1}]}' > /kibosh_mnt/kibosh_control
    # add write_corrupt fault
    $ echo '{"faults":[{"type":"write_corrupt", "prefix":"", "file_type":"", "mode":1000, "fraction":0.5, "count":-1}]}' > /kibosh_mnt/kibosh_control

# License

Kibosh is licensed under the Apache 2.0 license.  See LICENSE for details.

# Contact

Colin McCabe <colin@cmccabe.xyz>
