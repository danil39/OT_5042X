/*
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <libgen.h>
#include <sys/vfs.h>

#define DEBUGFS_TYPE	(0x64626720)
#define OPTIONS		"d:o:"

struct profile_args {
	char *device;
	char *ofile;
};

// blktrace.c
extern char *debugfs_path;
extern int start_blktrace(char *device, char *ofile);
extern void exit_tracing(void);

// blkparse.c
extern int start_blkparse(char *file);

// bt_timeline.c
extern int start_btt(char *file);

static void *thread_blktrace(void *arg)
{
	int ret = 0;
	struct profile_args *args = arg;

	ret = start_blktrace(args->device, args->ofile);

	return NULL;
}

int main(int argc, char *argv[])
{
	int opt = 0, ret = 0;
	pthread_t thread;
	struct profile_args args;
	struct statfs st;
	char stop;

	while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
		switch (opt) {
		case 'd':
			args.device = strdup(optarg);
			break;
		case 'o':
			args.ofile = strdup(optarg);
			break;
		default:
			printf("Invalid option: %c", opt);
			goto cleanup;
		}
	}

	if (statfs(debugfs_path, &st) < 0 || st.f_type != (long)DEBUGFS_TYPE) {
		fprintf(stderr, "Invalid debug path %s: %d/%s\n",
			debugfs_path, errno, strerror(errno));
		return 1;
	}

	if (pthread_create(&thread, NULL, thread_blktrace, &args)) {
		fprintf(stderr, "Failed to start thread\n");
		ret = errno;
		goto cleanup;
	}

	printf("Collecting block trace...\n");
	while(1) {
		printf("Press 's' to stop collecting blktrace: ");
		scanf("%c", &stop);
		if (stop == 's') break;
	};
	exit_tracing();

	printf("Parsing trace logs...\n");
	ret = start_blkparse(args.ofile);
	if (ret) {
		printf("block parse failed: %d", ret);
		goto cleanup;
	}

	printf("Generating metrics...\n");
	ret = start_btt(args.ofile);
	if (ret) {
		printf("btt failed: %d", ret);
		goto cleanup;
	}

	printf("Checkout out the logs at %s/\n", dirname(args.ofile));

cleanup:
	if (args.device) free(args.device);
	if (args.ofile) free(args.ofile);

	return ret;
}
