
# WHAT IS THIS?
This script is a Linux-specific command-line utility to manage POSIX queues.

# BUILT AND USE IT
You can compile this program with the command:
```sh
$ gcc pq.c -o pq -Wall -Wextra -lrt
```
which will generate the `pq` executable. You can call this command in various
ways. Here are some:

```sh
$ ./pq ls # List al queues
$ ./pq unlink /my_queue # Delete a queue named "my_queue"
$ ./pq stat /my_queue # Get information about the queue "my_queue"
```
if you want to know more on how to use it, just run it without arguments:
```sh
$ ./pq
```

# HOW IT WORKS

From the [manual](https://linux.die.net/man/7/mq_overview):

> # Mounting the message queue filesystem
> On Linux, message queues are created in a virtual filesystem.
> (Other implementations may also provide such a feature, but the
> details are likely to differ.)  This filesystem can be mounted
> (by the superuser) using the following commands:
> ```sh
> mkdir /dev/mqueue
> mount -t mqueue none /dev/mqueue
> ```
> The sticky bit is automatically enabled on the mount directory.
>
> After the filesystem has been mounted, the message queues on the
> system can be viewed and manipulated using the commands usually
> used for files (e.g., ls(1) and rm(1)).

This script automatizes this process by creating the folder and
mounting the POSIX FS by doing system calls directly.
