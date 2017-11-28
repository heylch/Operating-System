#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "blkdev.h"


void mirror_test(){
	struct blkdev *image = image_create("test.img");
}