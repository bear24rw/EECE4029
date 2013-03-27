Encrypted FUSE Filesystem
=========================

This project implements an 'encrypted' FUSE file system. The 'encryption' is
achived by simpily adding 1 to each byte if the user has permission to do
so.

Example
-------

```
% mkdir test_dir
% mkdir mount_dir
% id --user
1000
```

Mount the `test_dir` into `mount_dir` and set the user to our current uid.

```
% ./bbfs test_dir mount_dir 1000
```

Writing some text into a file in the `mount_dir` and reading it back shows that
it is read and written as expected.

```
% echo "12345\nThis is a test" > mount_dir/test.txt
% cat mount_dir/test.txt
12345
This is a test
```

By looking at the same file in the `test_dir` we can see that the file is
actually encrypted.

```
% cat test_dir/test.txt
23456Uijt!jt!b!uftu%
```

If we mount the directory with a user id other than our own and try to print
out `test.txt` we can see that we were not allowed to decrypt it (since our uid
does not match the uid that was specified when mounting)

```
% fusermount -u mount_dir
% ./bbfs test_dir mount_dir 1001
% cat mount_dir/test.txt
23456Uijt!jt!b!uftu%
```

If we try to write a file we will find that it does not get encrypted (since
again our uid does not the one specified when mounting).

```
% echo "11111\nThis is not encrypted" > mount_dir/not.txt
% cat test_dir/not.txt
11111
This is not encrypted
```

Additionally, if we try to change the permissions of a file we find that we are
unable to so and the attempt is recorded in the log file

```
% ll mount_dir/test.txt
-rw-r--r-- 1 user user 21 Mar 27 10:10 mount_dir/test.txt
% chmod +x mount_dir/test.txt
% ll mount_dir/test.txt
-rw-r--r-- 1 user user 21 Mar 27 10:10 mount_dir/test.txt
```
```
Illegal op by user 1000 on file /test.txt Wed Mar 27 10:19:51 2013
```
