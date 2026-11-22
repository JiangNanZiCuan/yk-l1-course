// SPDX-License-Identifier: GPL-2.0
/* Linux 3.4 procfs module: the kernel keeps the secret and judges guesses. */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include "guess_logic.h"

static DEFINE_MUTEX(game_lock);
static struct guess_state game;
static unsigned int round_id;

/* Called under game_lock, except during module initialization. */
static void new_round(void)
{
    unsigned int random;
    get_random_bytes(&random, sizeof(random));
    guess_start(&game, GUESS_MIN + random % (GUESS_MAX - GUESS_MIN + 1));
    round_id++;
    pr_info("guess_game: round=%u started range=1..100\n", round_id);
}

/* Publish feedback, never the secret. A complete read holds one state lock. */
static int game_show(struct seq_file *out, void *unused)
{
    mutex_lock(&game_lock);
    seq_printf(out, "round=%u\nmin=1\nmax=100\nattempts=%u\nlast=%d\nstatus=%s\n",
               round_id, game.attempts, game.last, guess_status_name(game.status));
    mutex_unlock(&game_lock);
    return 0;
}

static int game_open(struct inode *inode, struct file *file)
{
    return single_open(file, game_show, NULL);
}

/* Protocol: write 'new' or one integer; malformed input leaves state intact. */
static ssize_t game_write(struct file *file, const char __user *input,
                          size_t count, loff_t *position)
{
    char buffer[32];
    char *command;
    int value, result;
    if (!count || count >= sizeof(buffer))
        return -EINVAL;
    if (copy_from_user(buffer, input, count))
        return -EFAULT;
    buffer[count] = '\0';
    if (memchr(buffer, '\0', count))
        return -EINVAL;
    command = strim(buffer);
    if (strcmp(command, "new") == 0) {
        mutex_lock(&game_lock);
        new_round();
        mutex_unlock(&game_lock);
        return count;
    }
    if (kstrtoint(command, 10, &value))
        return -EINVAL;
    mutex_lock(&game_lock);
    result = guess_submit(&game, value);
    if (!result)
        pr_info("guess_game: round=%u attempt=%u guess=%d result=%s\n",
                round_id, game.attempts, value, guess_status_name(game.status));
    mutex_unlock(&game_lock);
    if (result)
        return result == -2 ? -EALREADY : -EINVAL;
    return count;
}

static const struct file_operations game_operations = {
    .owner = THIS_MODULE,
    .open = game_open,
    .read = seq_read,
    .write = game_write,
    .llseek = seq_lseek,
    .release = single_release,
};

static int __init game_init(void)
{
    new_round();
    if (!proc_create("guess_game", 0600, NULL, &game_operations))
        return -ENOMEM;
    pr_info("guess_game: loaded; interface=/proc/guess_game\n");
    return 0;
}

static void __exit game_exit(void)
{
    remove_proc_entry("guess_game", NULL);
    pr_info("guess_game: unloaded\n");
}

module_init(game_init);
module_exit(game_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("YK-L1 coursework");
MODULE_DESCRIPTION("Kernel number guessing game with procfs feedback");
