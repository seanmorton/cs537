# cs537
This repository contains projects from my operating systems course - CS537 Fall 2014. It is a good representation of my C programming skills. 

Each project was split into two parts: Part A and Part B. Part A usually involved creating programs that were similar to standard UNIX programs, while Part B always involved extending the functionality of [xv6](http://pdos.csail.mit.edu/6.828/2014/xv6.html), a simple UNIX-like operating system designed for educational purposes.

## p1
* Part A: Simple program that demonstrates the birthday paradox. This was a warm up project intended to refresh students' C programming skills.
* Part B: Added a new system call to xv6 that incremented a counter. This project served as an introduction to the xv6 environment.

## p2
* Part A: Simple UNIX shell that supports output redirection and pipes.
* Part B: Changed xv6's CPU scheduler to use a lottery scheduling policy.

## p3
* Part A: Dynamic memory allocator similar to malloc, with three different versions that are each optimized for different workloads.
* Part B: Rearranged the xv6 memory layout so that the stack grew from the bottom towards the top, with the heap growing from top to bottom.

## p4
* Part A: Web server that demonstrated the producer-consumer problem in concurrency.
* Part B: Implement threads in xv6, however I did not do this project because we were allowed to drop one project from our grade and it was Thanksgiving break :)

## p5
* Part A: Fully functional network file server, using UDP and RPC. This was by far my favorite project in the course. It taught me a great deal about file systems.
* Part B: Implemented a new file type in xv6 that supported mirroring, similar to RAID. A file of this type was gauranteed to always have two copies on disk.
