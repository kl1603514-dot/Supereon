#ifndef SELF_UPDATE_H
#define SELF_UPDATE_H
/* 1 = an update was started, caller must exit now.  0 = keep running. */
int self_update_check(int argc, char **argv);
#endif