id: E-DEPLOY-0025
title: A log with no limit
match: a day of logging at 100 KB a second fills a 4 GB disk in eleven hours with no limit, and stays inside the budget with one
match: records fill a file, the next one closes it, and the oldest files go first so that what is left is always the newest, with no holes
platforms: linux, windows
teaches: 20-09-a-log-that-cannot-fill-the-disk
---

## Symptom

The robot stops being able to save its configuration, take an update or dump the black box, after eleven hours, and the cause is a log file.

## Cause

Logging 100 KB a second wrote 8.6 GB in a day: a 4 GB disk was full after 11.1 hours with no limit. With 10 MB files, 20 files and 200 MB the peak was 199 MB in at most 20 files and 8.44 GB was deleted. Nine records of 40 bytes in files of 100 with a total of 300 left files 1 to 4, 280 bytes, with 80 deleted.

## Fix

Rotate under a byte budget, oldest first.

```cpp
LogStore store(policy);
store.append(bytes);
```
