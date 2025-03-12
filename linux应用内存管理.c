#include <stdio.h>      // 用户空间打印
#include <stdlib.h>     // malloc/free
#include <string.h>     // strncpy
#include <pthread.h>    // pthread_mutex

/* 定义客户端结构体 */
struct client_data {
    int id;
    char name[32];
    unsigned long status;
    void *private_data;
};

/* 静态全局指针，用于保存客户端数据 */
static struct client_data *global_client = NULL;

/* 定义互斥锁保护全局指针 */
static pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 用户空间的打印宏，类似内核的pr_xxx */
#define pr_info(fmt, ...)  printf("INFO: " fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...)   fprintf(stderr, "ERROR: " fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...)  printf("WARNING: " fmt, ##__VA_ARGS__)

/* 错误码定义，模仿内核 */
#define EINVAL  22
#define ENOMEM  12

/**
 * set_client - 设置全局客户端数据
 * @client: 要设置的客户端结构体指针
 * 
 * 返回值: 0 表示成功，负值表示错误
 */
int set_client(struct client_data *client)
{
    if (!client) {
        pr_err("%s: Invalid client pointer\n", __func__);
        return -EINVAL;
    }

    pthread_mutex_lock(&client_mutex);
    if (global_client) {
        pr_warn("%s: Overwriting existing client\n", __func__);
        free(global_client);
    }
    global_client = client;
    pthread_mutex_unlock(&client_mutex);

    pr_info("%s: Client set successfully\n", __func__);
    return 0;
}

/**
 * get_client - 获取全局客户端数据
 * 
 * 返回值: 指向客户端结构体的指针，如果未设置则返回NULL
 */
struct client_data *get_client(void)
{
    struct client_data *client;

    pthread_mutex_lock(&client_mutex);
    client = global_client;
    pthread_mutex_unlock(&client_mutex);

    return client;
}

/**
 * client_init - 初始化客户端数据
 * 
 * 返回值: 0 表示成功，负值表示错误
 */
int client_init(void)
{
    struct client_data *client;
    int ret;

    /* 分配内存，使用用户空间的malloc */
    client = malloc(sizeof(struct client_data));
    if (!client) {
        pr_err("%s: Failed to allocate memory for client\n", __func__);
        return -ENOMEM;
    }

    /* 初始化结构体成员 */
    client->id = 1;
    strncpy(client->name, "default_client", sizeof(client->name) - 1);
    client->name[sizeof(client->name) - 1] = '\0';
    client->status = 0;
    client->private_data = NULL;

    /* 设置全局客户端 */
    ret = set_client(client);
    if (ret) {
        pr_err("%s: Failed to set client\n", __func__);
        free(client);
        return ret;
    }

    pr_info("%s: Client initialized successfully\n", __func__);
    return 0;
}

/**
 * client_cleanup - 清理客户端数据
 */
void client_cleanup(void)
{
    pthread_mutex_lock(&client_mutex);
    if (global_client) {
        free(global_client);
        global_client = NULL;
        pr_info("%s: Client data freed\n", __func__);
    }
    pthread_mutex_unlock(&client_mutex);
}

/* 示例主函数 */
int main(void)
{
    int ret;
    struct client_data *client;

    /* 初始化 */
    ret = client_init();
    if (ret) {
        return ret;
    }

    /* 获取并使用 */
    client = get_client();
    if (client) {
        pr_info("Client ID: %d, Name: %s\n", client->id, client->name);
    }

    /* 清理 */
    client_cleanup();
    return 0;
}