#ifndef SYS_APP_MSG_H
#define SYS_APP_MSG_H





int app_task_put_usr_msg(int msg, int arg_num, ...);

void app_task_get_msg(int *msg, int msg_size, int block);

int app_task_post_usr_msg(int msg, int arg_num, ...);

int app_task_put_key_msg(int msg , int value);


#endif

