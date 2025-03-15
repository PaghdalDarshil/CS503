# Linux User Namespace Lab Assignment (No Root Required)

## Overview
In this lab, you will explore Linux user namespaces, which are a fundamental building block of container technology. User namespaces are the only namespace type that can be manipulated without root privileges, making them perfect for student environments. You'll learn how user namespaces provide isolation and capabilities to unprivileged users.

## Learning Objectives
- Understand the concept of Linux user namespaces
- Learn how to create and manage user namespaces without root privileges
- Experience how UID/GID mapping works in practice
- Observe how capabilities function inside user namespaces

## Prerequisites
- Access to a Linux machine (Ubuntu 20.04+ or other modern distribution)
- Unprivileged user namespace support must be enabled
- Basic command-line familiarity

## Checking System Compatibility
Before starting the lab, check if your system supports unprivileged user namespaces:

```
cat /proc/sys/kernel/unprivileged_userns_clone
```

If this returns `1`, your system supports unprivileged user namespaces. If it returns `0`, you'll need to request your system administrator to enable this feature (or use a different system).

<img width="1082" alt="Screenshot 2025-03-14 at 11 10 30 PM" src="https://github.com/user-attachments/assets/856fe6b1-aa4c-49f7-bef4-a01ec4591dc7" />

## Part 1: Exploring User Namespaces Basics

1. Examine your current user ID and groups:

```
id
```
Record this information for comparison later.

<img width="1161" alt="Screenshot 2025-03-14 at 11 13 06 PM" src="https://github.com/user-attachments/assets/e48e5109-2add-48df-95ff-49cb20880cbe" />


2. List your current namespaces:

```
ls -la /proc/self/ns/
```
Take note of the user namespace identifier.

<img width="1140" alt="Screenshot 2025-03-14 at 11 13 26 PM" src="https://github.com/user-attachments/assets/79b87641-1419-45df-8d93-28bec838f202" />

3. Create a new user namespace:

```
unshare --user bash
```
<img width="730" alt="Screenshot 2025-03-14 at 11 13 51 PM" src="https://github.com/user-attachments/assets/686a4527-7318-4ba0-bddf-07012c772325" />


4. Inside this new bash shell (in the new user namespace), check your user and group IDs:

```
id
```

Notice how your UID and GID appear as 65534 (nobody).

<img width="899" alt="Screenshot 2025-03-14 at 11 14 10 PM" src="https://github.com/user-attachments/assets/1f17b4c2-411e-4ef9-af28-80c03eb392cc" />


5. Examine the namespace identifier to confirm you're in a different user namespace:

```
ls -la /proc/self/ns/user
```

Compare it with the value you noted earlier.

You might also try `whoami` and observe that it prints "nobody".

<img width="1104" alt="Screenshot 2025-03-14 at 11 14 44 PM" src="https://github.com/user-attachments/assets/0997971b-61a1-4dbe-ba35-de80ba1179b9" />


6. Check your new namespace's UID mapping:

```
cat /proc/self/uid_map
cat /proc/self/gid_map
```

<img width="1014" alt="Screenshot 2025-03-14 at 11 15 12 PM" src="https://github.com/user-attachments/assets/0ee6e9db-a8df-43e0-818c-2972fed5e63d" />

Note that these files are likely empty, indicating no mappings are established.

7. Exit the namespace:

```
exit
```

<img width="489" alt="Screenshot 2025-03-14 at 11 15 27 PM" src="https://github.com/user-attachments/assets/7c0dbe96-9e4c-4895-a23b-a079c960e99a" />


Whole PART 1 
<img width="1173" alt="Screenshot 2025-03-14 at 11 15 46 PM" src="https://github.com/user-attachments/assets/2b8f7895-c9ab-406c-8001-66860f63d832" />


## Part 2: Creating User Namespaces with UID/GID Mappings

For this part, we'll use a simple C program to create a user namespace with custom mappings. The C program is `userns_child.c` in the starter folder

Compile and run this program:

```
make
./userns_child
```
<img width="1193" alt="Screenshot 2025-03-14 at 11 18 43 PM" src="https://github.com/user-attachments/assets/fca2a60c-7a10-4ced-996d-0eae684b70a7" />

Observe the output and answer these questions:

1. What is your UID in the new user namespace?
    - Ans: - Inside the user namespace, the UID appears as:
    - uid=0(root) gid=65534(nogroup)
    - This means the process is assigned UID 0 (root) within the namespace but remains unprivileged.
2. What capabilities do you have in the user namespace?
    - Ans: - The capabilities inside the namespace are:
    - CapInh:  0000000000000000
    - CapPrm:  0000000000000000
    - CapEff:  0000000000000000
    - CapBnd:  000001ffffffffff
    - CapAmb:  0000000000000000
    - 
    - CapEff (Effective capabilities): 0000000000000000 → No real privileges.
    - CapPrm (Permitted capabilities): 0000000000000000 → Cannot execute privileged operations.
    - CapBnd (Bounding capabilities): 000001ffffffffff → Defines the max set of capabilities available.
    - 
    - Even though the UID appears as 0, the process has no effective privileges, preventing it from performing root-level actions like mounting or modifying the system settings.

3. How do the UID mappings work?
   - Ans:- UID mappings allow an unprivileged user on the host to appear as UID 0 (root) inside the namespace.
   - 0    1000    1
   - 
   - This means:
   - UID 0 inside the namespace corresponds to UID 1000 on the host.
   - The process inside the namespace sees itself as root (UID 0) but is still restricted to its original host UID permissions.

## Part 3: Exploring Isolation with User Namespaces

Run `check_isolation.sh` in the starter folder:

```bash
chmod +x check_isolation.sh
./check_isolation.sh
```

Note the differences in the output before and after entering the user namespace.

<img width="1180" alt="Screenshot 2025-03-14 at 11 26 52 PM" src="https://github.com/user-attachments/assets/9c824642-eb4d-4591-ac3b-105b9d50559f" />
<img width="1181" alt="Screenshot 2025-03-14 at 11 27 09 PM" src="https://github.com/user-attachments/assets/2b59d7db-263c-4e03-9f02-f584b6821784" />
<img width="982" alt="Screenshot 2025-03-14 at 11 27 24 PM" src="https://github.com/user-attachments/assets/f1deaf85-843c-4307-b64f-95c78923fe31" />


## Part 4: Practical Applications - Running a Program with "Root" Privileges

1. Compile and use it to run commands as "root" (in the namespace):

```
make
./fake_root id
./fake_root bash -c "id && whoami && ls -la /proc/self/ns/"
```

Notice that inside the user namespace, your UID is 0 (root), but this is only within the namespace!

<img width="1008" alt="Screenshot 2025-03-14 at 11 28 38 PM" src="https://github.com/user-attachments/assets/d146a6e5-e9c2-4c97-a4e4-f7c594b3311e" />


2. Try to create a file as "root" in your home directory, using your program:

```
./fake_root touch ~/root_test_file
ls -la ~/root_test_file
```

The file is created, but notice who owns it on the real system!

<img width="1066" alt="Screenshot 2025-03-14 at 11 29 10 PM" src="https://github.com/user-attachments/assets/7a06fb5a-c13a-4691-8a82-cd6147648942" />


## Part 5: Exploring the Limitations

1. Try to perform privileged operations in your user namespace:

```
mkdir -p /tmp/test_mount
./fake_root bash -c "mount -t tmpfs none /tmp/test_mount"
```

This will likely fail with "Operation not permitted" because of missing capabilities or namespace configurations.

<img width="944" alt="Screenshot 2025-03-14 at 11 29 49 PM" src="https://github.com/user-attachments/assets/d5f9f585-f12c-407b-93dd-a8df80998fa0" />

2. Try to access network namespaces (which usually require real root):

```
./fake_root ip link add dummy0 type dummy
```

Note and document what errors you encounter.

<img width="1044" alt="Screenshot 2025-03-14 at 11 30 28 PM" src="https://github.com/user-attachments/assets/7c1b102c-2313-48f2-b782-6ac13f4acffd" />


## Deliverables

1. A brief introduction to Linux user namespaces (in your own words)
  - Ans: - Linux user namespaces allow processes to run with different user and group IDs than they have on the host system. This means that a user can appear to have root privileges within a namespace while still being restricted as an unprivileged user outside of it. This is an important feature for containerization and security, as it enables process isolation without giving full root access. 
2. Answers to the following questions:
   1. How do user namespaces provide the illusion of having root privileges?
   - Ans: - User namespaces allow an unprivileged user to be mapped as UID 0 (root) inside a namespace, which makes it look like they have root access. However, they still lack real root privileges outside of the namespace, so they can’t make system-wide changes.
     
   2. What is the purpose of UID/GID mapping in user namespaces?
   - Ans: - UID/GID mapping allows a process inside a namespace to appear as a different user than it is on the host system. This enables unprivileged users to run processes as root inside the namespace, while they are still restricted to their original permissions on the host.
     
   3. What limitations did you encounter when working with user namespaces?
   - Ans: -Failed UID/GID mapping: write: Operation not permitted when trying to modify /proc/self/uid_map.
     - No real privileges: The process inside the namespace had CapEff = 0000000000000000, meaning it couldn't perform privileged operations.
     - Mounting failed: Attempting to mount a filesystem resulted in Operation not permitted.
     - Network changes failed: The ip link add command was denied.
       
   4. How might user namespaces be used in container technology?
      - Ans: - User namespaces are used in containers to allow processes to run as root inside the container while keeping them unprivileged on the host. This is a key security feature in container runtimes like Docker and Podman, preventing processes inside the container from affecting the host system.

   5. What security implications do user namespaces have?
      - Ans: - Potential privilege escalation: If misconfigured, a user could gain access to sensitive files.
          -  Increased attack surface: More kernel interactions mean a greater risk of security vulnerabilities.
          -  Filesystem risks: Improper mappings could allow namespace processes to modify host files.

   6. Why are other namespace types typically not available to unprivileged users?
      - Ans: - PID Namespace: Prevents access to host process IDs.
          - Network Namespace: Could allow an unprivileged user to manipulate network settings.
          - Mount Namespace: Stops unauthorized filesystem modifications.
          - IPC Namespace: Blocks inter-process communication between namespaces and the host.


5. A conclusion section with your insights and any challenges you faced.
   - Ans: - This lab helped me understand how user namespaces work by allowing a user to run processes as root inside a namespace while still being unprivileged outside. UID/GID mapping is a key feature that enables this without compromising security. However, the limitations, like lack of real privileges and restricted operations, make sure that the namespace remains isolated. These concepts are widely used in container technologies, which rely on namespaces for process isolation.
   - One challenge I faced was the "Operation not permitted" error when writing to /proc/self/uid_map, which prevented the user mapping from being set up properly. Despite this, I was still able to observe the key behaviors of user namespaces, including isolation and restricted capabilities.


