# Networking
Layer 2 protocol: ECHO_REQ and ECHO_REPLY

1. "test" LKM creates the character device "test_dev".
And provides IOCTL implementation for READ and WRITE command.

2. Adds a protocol handler to the networking stack. The passed &packet_type
is linked into kernel lists and may not be freed until it has been
removed from the kernel lists.

3. From user space an application can send custom ECHO_REQ packets
via character device's IOCTL implementation to another machine, 
and receives ECHO_REPLY from target system.

4. test kernel module receives packet data via IOCTL, and at layer 2 sends
the packet to target system. For now target system MAC is hard coded, just
for testing purpose.
