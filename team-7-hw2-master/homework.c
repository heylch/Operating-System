/*
 * file:        homework.c
 * description: skeleton code for CS 5600 Homework 2
 *
 * Peter Desnoyers, Northeastern Computer Science, 2011
 * $Id: homework.c 410 2011-11-07 18:42:45Z pjd $
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "blkdev.h"

/********** MIRRORING ***************/

/* example state for mirror device. See mirror_create for how to
 * initialize a struct blkdev with this.
 */
struct mirror_dev {
    struct blkdev *disks[2];    /* flag bad disk by setting to NULL */
    int nblks;
};
    
static int mirror_num_blocks(struct blkdev *dev)
{
    /* your code here */
    if(dev == NULL){
        return 0;
    }
    struct blkdev *mirror = dev->private;
    struct blkdev *disk0 = mirror->disks[0];
    struct blkdev *disk1 = mirror->disks[1];
    if(disk0 != NULL){
        return disk0->ops->num_blocks(disk0);
    }
    if(disk1 != NULL){
        return disk1->ops->num_blocks(disk1);
    }
    return 0;
}

/* read from one of the sides of the mirror. (if one side has failed,
 * it had better be the other one...) If both sides have failed,
 * return an error.
 * Note that a read operation may return an error to indicate that the
 * underlying device has failed, in which case you should close the
 * device and flag it (e.g. as a null pointer) so you won't try to use
 * it again. 
 */
static int mirror_read(struct blkdev * dev, int first_blk,
                       int num_blks, void *buf)
{   
    int result = 0;
    if(dev->private == NULL){
        return E_UNAVAIL;
    }
    else{
        struct blkdev *mirror = dev->private;
        if(mirror == NULL){
            return E_UNAVAIL;
        }
        struct blkdev *disk0 = mirror->disks[0];
        struct blkdev *disk1 = mirror->disks[1];
        if(disk0 != NULL){
            result = disk0->ops->read(disk0,first_blk,num_blks,buf);
            if(result != E_UNAVAIL && result != E_BADADDR){
                return result;
            }
            else{
                disk0->ops->close(disk0);
                disk0 = NULL;
            }
        }
        if(disk1 == NULL)
            return E_UNAVAIL;
        result = disk1->ops->read(disk1,first_blk,num_blks,buf);
        if(result == E_UNAVAIL || result == E_BADADDR){
            disk1->ops->close(disk1);
            disk1 = NULL;
        }
    }
    return result;
}

/* write to both sides of the mirror, or the remaining side if one has
 * failed. If both sides have failed, return an error.
 * Note that a write operation may indicate that the underlying device
 * has failed, in which case you should close the device and flag it
 * (e.g. as a null pointer) so you won't try to use it again.
 */
static int mirror_write(struct blkdev * dev, int first_blk,
                        int num_blks, void *buf)
{
    int flag = 2;
    if(dev != NULL){
        if(dev->private != NULL){
            struct blkdev *mirror = dev->private;
            if(mirror->disks[0] == NULL && mirror->disks[1] == NULL){
                return E_UNAVAIL;
            }
            if(mirror->disks[0] != NULL){
                struct blkdev *disk0 = mirror->disks[0];
                if(disk0->ops->read(disk0, first_blk, num_blks, buf) != E_UNAVAIL){
                    disk0->ops->write(disk0, first_blk, num_blks,buf);
                    flag--;
                }
                else{
                    disk0 = NULL;
                }
            }
            else{
                if(mirror->disks[1] != NULL){
                    struct blkdev *disk1 = mirror->disks[1];
                    if(disk1->ops->read(disk1，first_blk, num_blks, buf) != E_UNAVAIL){
                        disk1->ops->write(disk1, first_blk, num_blks,buf);
                        flag--;
                    }
                    else{
                        disk1 = NULL;
                    }
                }
            }
        }
    }
    if(flag == 2){
        return E_UNAVAIL;
    }
    else{
        return 0;
    }
    
}

/* clean up, including: close any open (i.e. non-failed) devices, and
 * free any data structures you allocated in mirror_create.
 */
static void mirror_close(struct blkdev *dev)
{
    /* your code here */
    if(dev != NULL && dev->private != NULL){
        struct blkddev *mirror = dev->private;
        if(mirror->disks[0] != NULL){
            mirror->disks[0]->ops->close(mirror->disks[0]);
            mirror->disks[0] = NULL;
        }
        if(mirror->disks[1] != NULL){
            mirror->disks[1]->ops->close(mirror->disks[1]);
            mirror->disks[1] = NULL;
        }
        free(dev->private);
        free(dev);
    }
}

struct blkdev_ops mirror_ops = {
    .num_blocks = mirror_num_blocks,
    .read = mirror_read,
    .write = mirror_write,
    .close = mirror_close
};

/* create a mirrored volume from two disks. Do not write to the disks
 * in this function - you should assume that they contain identical
 * contents. 
 */
struct blkdev *mirror_create(struct blkdev *disks[2])
{
    struct blkdev *dev = malloc(sizeof(*dev));
    struct mirror_dev *mdev = malloc(sizeof(*mdev));

    /* your code here */
    if (disks[0]->ops->num_blocks(disks[0]) != disks[1]->ops->num_blocks(disks[0])) {
        printf("Mirrioring creating failed: size mis-match.\n");
        return E_SIZE;
    }
    mdev->disks[0] = disks[0];
    mdev->disks[1] = disks[1];
    mdev->nblks = 2 * mirror_num_blocks(disks[0]); 

    dev->private = mdev;
    dev->ops = &mirror_ops;

    return dev;
}

/* replace failed device 'i' (0 or 1) in a mirror. Note that we assume
 * the upper layer knows which device failed. You will need to
 * replicate content from the other underlying device before returning
 * from this call.
 */
int mirror_replace(struct blkdev *volume, int i, struct blkdev *newdisk)
{   
    int good = 1 -i;
    struct blkdev *finedisk = volume->disks[good];
    if(finedisk->ops->num_blocks(finedisk) == newdisk->ops->num_blocks(newdisk)){
        volume->disks[i] = newdisk;
    }
    else{
        printf("Mirrioring replacing failed: size mis-match.\n");
        return E-SIZE;
    }
    return SUCCESS;
}

/**********  RAID0 ***************/

int raid0_num_blocks(struct blkdev *dev)
{
    return 0;
}

/* read blocks from a striped volume. 
 * Note that a read operation may return an error to indicate that the
 * underlying device has failed, in which case you should (a) close the
 * device and (b) return an error on this and all subsequent read or
 * write operations. 
 */
static int raid0_read(struct blkdev * dev, int first_blk,
                       int num_blks, void *buf)
{
    return 0;
}

/* write blocks to a striped volume.
 * Again if an underlying device fails you should close it and return
 * an error for this and all subsequent read or write operations.
 */
static int raid0_write(struct blkdev * dev, int first_blk,
                        int num_blks, void *buf)
{
    return 0;
}

/* clean up, including: close all devices and free any data structures
 * you allocated in stripe_create. 
 */
static void raid0_close(struct blkdev *dev)
{
}

/* create a striped volume across N disks, with a stripe size of
 * 'unit'. (i.e. if 'unit' is 4, then blocks 0..3 will be on disks[0],
 * 4..7 on disks[1], etc.)
 * Check the size of the disks to compute the final volume size, and
 * fail (return NULL) if they aren't all the same.
 * Do not write to the disks in this function.
 */
struct blkdev *raid0_create(int N, struct blkdev *disks[], int unit)
{
    return NULL;
}

/**********   RAID 4  ***************/

/* helper function - compute parity function across two blocks of
 * 'len' bytes and put it in a third block. Note that 'dst' can be the
 * same as either 'src1' or 'src2', so to compute parity across N
 * blocks you can do: 
 *
 *     void **block[i] - array of pointers to blocks
 *     dst = <zeros[len]>
 *     for (i = 0; i < N; i++)
 *        parity(block[i], dst, dst);
 *
 * Yes, it could be faster. Don't worry about it.
 */
void parity(int len, void *src1, void *src2, void *dst)
{
    unsigned char *s1 = src1, *s2 = src2, *d = dst;
    int i;
    for (i = 0; i < len; i++)
        d[i] = s1[i] ^ s2[i];
}

/* read blocks from a RAID 4 volume.
 * If the volume is in a degraded state you may need to reconstruct
 * data from the other stripes of the stripe set plus parity.
 * If a drive fails during a read and all other drives are
 * operational, close that drive and continue in degraded state.
 * If a drive fails and the volume is already in a degraded state,
 * close the drive and return an error.
 */
static int raid4_read(struct blkdev * dev, int first_blk,
                      int num_blks, void *buf) 
{
    return 0;
}

/* write blocks to a RAID 4 volume.
 * Note that you must handle short writes - i.e. less than a full
 * stripe set. You may either use the optimized algorithm (for N>3
 * read old data, parity, write new data, new parity) or you can read
 * the entire stripe set, modify it, and re-write it. Your code will
 * be graded on correctness, not speed.
 * If an underlying device fails you should close it and complete the
 * write in the degraded state. If a drive fails in the degraded
 * state, close it and return an error.
 * In the degraded state perform all writes to non-failed drives, and
 * forget about the failed one. (parity will handle it)
 */
static int raid4_write(struct blkdev * dev, int first_blk,
                       int num_blks, void *buf)
{
    return 0;
}

/* clean up, including: close all devices and free any data structures
 * you allocated in raid4_create. 
 */
static void raid4_close(struct blkdev *dev)
{
}

/* Initialize a RAID 4 volume with strip size 'unit', using
 * disks[N-1] as the parity drive. Do not write to the disks - assume
 * that they are properly initialized with correct parity. (warning -
 * some of the grading scripts may fail if you modify data on the
 * drives in this function)
 */
struct blkdev *raid4_create(int N, struct blkdev *disks[], int unit)
{
    return NULL;
}

/* replace failed device 'i' in a RAID 4. Note that we assume
 * the upper layer knows which device failed. You will need to
 * reconstruct content from data and parity before returning
 * from this call.
 */
int raid4_replace(struct blkdev *volume, int i, struct blkdev *newdisk)
{
    return SUCCESS;
}

