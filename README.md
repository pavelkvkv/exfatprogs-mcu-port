
## About this port
The porting target is to work on microcontrollers and embedded devices running without an OS (bare-metal, FreeRTOS, etc.). Corresponding changes have been made to the code to work on 32-bit systems (sometimes in a very clumsy way).

The project is based on version 1.2.4 of exfatprogs, which already includes a mechanism for clearing lost clusters.

## Usage
The porting has been done only for fsck. Copy the fsck, include, and lib directories into your project.
The project includes examples of wrappers for memory allocation and low-level memory card operations (blkdev_wrapper.c, mem_wrapper.c) and an example of running checks on FreeRTOS (fsck_tasks.c).
These examples were written for our specific tasks, so they may have flaws. I have tried to document the function operations before publication.

## Issues
High memory consumption. Testing on a real filesystem with a couple of hundred thousand files consumes about 30 MB of dynamic RAM for storing exfat_inode structures. In its current form, the program is suitable only for small storage devices with light usage.

## Information about the original repository
Address: https://github.com/exfatprogs/exfatprogs/
