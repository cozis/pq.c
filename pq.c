
/* === WHAT IS THIS? =======================================================
**
** This script is a Linux-specific command-line utility to manage POSIX queues.
**
** === BUILT IT ============================================================
**
** You can compile this program with:
**   $ gcc pq.c -o pq -Wall -Wextra -lrt
**
** (it's assumed that this file's name is "pq.c")
**
** === HOW IT WORKS ========================================================
** 
** From the manual at https://linux.die.net/man/7/mq_overview:
**
** > Mounting the message queue filesystem
** >    On Linux, message queues are created in a virtual filesystem.
** >    (Other implementations may also provide such a feature, but the
** >    details are likely to differ.)  This filesystem can be mounted
** >    (by the superuser) using the following commands:
** >
** >        # mkdir /dev/mqueue
** >        # mount -t mqueue none /dev/mqueue
** >
** >    The sticky bit is automatically enabled on the mount directory.
** >
** >    After the filesystem has been mounted, the message queues on the
** >    system can be viewed and manipulated using the commands usually
** >    used for files (e.g., ls(1) and rm(1)).
**
** This script automatizes this process by creating the folder and
** mounting the POSIX FS by doing system calls directly.
**
** =========================================================================
*/

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <mqueue.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mount.h>

#define MOUNT_POINT "/dev/mqueue"

// Make sure "/dev/mqueue" exists and a "mqueue" (POSIX queue file system) is mounted on it.
static bool ensurePosixQueueFileSystemMounted(void)
{
    int res;

    // Create the mount point folder if it doesn't exist already.
    res = mkdir(MOUNT_POINT, 0644);
    if (res && errno != EEXIST) { // It's ok if "mkdir" fails with EEXIST because it
                                  // just means that the folder exists already.
        fprintf(stderr, "Error: Couldn't create posix filesystem mount point %s (%s)\n", MOUNT_POINT, strerror(errno));
        return false;
    }

    // Mount the POSIX queue FS if it wasn't already mounted.
    res = mount("none", MOUNT_POINT, "mqueue", 0, NULL);
    if (res && errno != EBUSY) { // If the mount call fails with EBUSY it (probably)
                                 // means that the FS was already mounted.
        fprintf(stderr, "Error: Couldn't mount the posix queue filesystem (%s)\n", strerror(errno));
        return false;
    }

    return true;
}

//unmount POSIX queue file system in "/dev/mqueue" and remove dir "mqueue"
static bool unmountPosixQueue(void)
{
    int res;

    res = umount(MOUNT_POINT);
    if (res && errno == EBUSY) { // If the umount call fails with EBUSY it 
                                 // means that the FS is busy and can't be unmounted.
        fprintf(stderr, "Error: Couldn't unmount the posix queue filesystem (%s)\n", strerror(errno));
        return false;
    }
	else{// Remove the mount point folder if it wasn't busy.
		res = rmdir(MOUNT_POINT);
		if (res && errno != EBUSY) { 
			fprintf(stderr, "Error: Couldn't remove posix filesystem mount point %s (%s)\n", MOUNT_POINT, strerror(errno));
			return false;
		}
	}

    return true;
}

// List all POSIX queues by writing their names to STDOUT.
static bool listPosixQueues(void)
{
    DIR *d = opendir(MOUNT_POINT);
    if (d == NULL) {
        fprintf(stderr, "Error: Couldn't read from the posix queue filesystem\n");
        return false;
    }

    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_type == DT_DIR ||
            ent->d_name[0] == '.')
            continue;
        fprintf(stdout, "%s\n", ent->d_name);
        count++;
    }

    if (count == 0)
        fprintf(stdout, "(No posix queues)\n");

    closedir(d);
    return true;
}

static bool printPosixQueueInfo(const char *queue_name)
{
    mqd_t queue_fd = mq_open(queue_name, O_RDONLY);
    if (queue_fd < 0) {
        fprintf(stderr, "Error: Couldn't open queue %s (%s)\n", queue_name, strerror(errno));
        return false;
    }

    struct mq_attr attr;
    if (mq_getattr(queue_fd, &attr)) {
        fprintf(stderr, "Error: Failed to query queue %s for its parameters (%s)\n", queue_name, strerror(errno));
        mq_close(queue_fd);
        return false;
    }

    fprintf(stdout, 
            "flags   %ld\n"
            "maxmsg  %ld\n"
            "msgsize %ld\n"
            "curmsgs %ld\n",
            attr.mq_flags,
            attr.mq_maxmsg,
            attr.mq_msgsize,
            attr.mq_curmsgs);

    mq_close(queue_fd);
    return true;
}

static bool unlinkPosixQueue(const char *queue_name)
{
    if (mq_unlink(queue_name)) {
        fprintf(stderr, "Error: Failed unlink queue %s (%s)\n", queue_name, strerror(errno));
        return false;
    }
    return true;
}

static void usage(FILE *stream, const char *progname)
{
    fprintf(stream, "Usage: $ sudo %s { ls | stat /<queue-name> | unlink /<queue-name> | umount }\n", progname);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        const char *progname = argc > 0 ? argv[0] : "pq";
        fprintf(stderr, "Error: Invalid usage\n");
        usage(stderr, progname);
        return -1;
    }

    if (!ensurePosixQueueFileSystemMounted())
        return -1;

    if (!strcmp(argv[1], "ls"))
        return listPosixQueues() ? 0 : -1;

    if (!strcmp(argv[1], "umount"))
        return unmountPosixQueue() ? 0 : -1;

    if (!strcmp(argv[1], "stat")) {
        if (argc < 3) {
            const char *progname = argc > 0 ? argv[0] : "pq";
            fprintf(stderr, "Error: Invalid usage\n");
            usage(stderr, progname);
            return -1;
        }
        const char *queue_name = argv[2];
        return printPosixQueueInfo(queue_name) ? 0 : -1;
    }

    if (!strcmp(argv[1], "unlink")) {
        if (argc < 3) {
            const char *progname = argc > 0 ? argv[0] : "pq";
            fprintf(stderr, "Error: Invalid usage\n");
            usage(stderr, progname);
            return -1;
        }
        const char *queue_name = argv[2];
        return unlinkPosixQueue(queue_name) ? 0 : -1;
    }

    fprintf(stderr, "Error: Invalid action \"%s\"\n", argv[1]);
    return -1;
}
