#include <linux/module.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/jiffies.h>
#include <linux/time.h>


#define NODE_NAME "start_system"
#define LEN_MSG 400

#define LOG(...) printk(KERN_INFO "INFO: " __VA_ARGS__)
#define ERR(...) printk(KERN_ERR "Error: " __VA_ARGS__)
#define DBG(...) if (deb > 0) printk(KERN_DEBUG "DBG: " __VA_ARGS__)



static int deb = 0;
module_param(deb, int, 0);

static inline u64 read_tsc(void)
{
	union sc
	{
		struct {u32 hi, lo;} r;
		u64 res;
	} sc;
	
	__asm__ __volatile__ ("rdtsc" : "=a"(sc.r.lo), "=d"(sc.r.hi));
	return sc.res;
}

static u64 calibr(void)
{
	int rep = 10000;
	u64 temp1 = 0, temp2 = 0, res = 0;
	for (int i = 0; i < rep; ++i)
	{
		temp1 = read_tsc();
		temp2 = read_tsc();
		res += temp2 - temp1;
	}
	
	return (u64)res / rep;
}

static u64 proc_hz(void)
{
	long long temp1, temp2;
	time64_t t1, t2;
	t1 = jiffies;
	while(jiffies != (t1 + HZ)) temp1 = read_tsc();
	t2 = jiffies;
	while(jiffies != (t2 + HZ)) temp2 = read_tsc();
	
	return temp1 - temp2 - calibr();
}

static void get_time(u64* all_sec, u64* all_min, u64* all_hours, char* out)
{
    *all_sec = jiffies_to_msecs(jiffies) / HZ;
    *all_min = *all_sec / 60;
    *all_hours = *all_min / 60;

    DBG("first time is completed\n");

    u64 tmp_sec = *all_sec;
    u64 h = tmp_sec / 3600; 
    tmp_sec %= 3600; 
    u64 m = tmp_sec / 60; 
    tmp_sec %= 60; 

    sprintf(out, "time: %llu:%02llu:%02llu", h, m, tmp_sec); 

    DBG("second time is completed\n");
}
		

static int open_node_proc(struct inode* inode, struct file* file)
{
	int ret = 0;
	DBG("Open node\n");
	return ret;
}


static ssize_t read_node_proc(struct file* file, char __user* buffer, size_t length, loff_t* offset)
{
	u64 sec, min, hours;	
	char output[30];
	DBG("ptr to file: %p\nptr to usr buffer: %p\nlenght: 	%ld\noffset: %p\n", file, buffer, length, offset);
	char buff_msg[LEN_MSG];
	get_time(&sec, &min, &hours, output);
	int msg_len = snprintf(buff_msg, LEN_MSG, "Time from system start\nsec : %llu\nmin: %llu\nhours: %llu\nprocessor HZ per sec: %llu\n\n%s\n", sec, min, hours, proc_hz(), output);
	if (*offset >= msg_len)
	{
		return 0;
	}
	
	DBG("1");
	
	int to_copy = msg_len - *offset;
	if (to_copy > length)
	{
		to_copy = length;
	}
	
	if (copy_to_user(buffer, buff_msg + *offset, to_copy))
	{
		DBG("Error");
		return -EFAULT;
	}
	DBG("2");
	DBG("read: %lld (buffer=%p, off=%ld)", length, buffer, *offset); 
	
	*offset += to_copy;
	return to_copy;
}


static const struct proc_ops pops = 
{
	.proc_open = open_node_proc,
	.proc_read = read_node_proc,
	.proc_write = NULL,
};



static int __init start(void)
{
	struct proc_dir_entry* entry = proc_create(NODE_NAME, 0, NULL, &pops);
	if (!entry)
	{
		ERR("Cannot create /proc/%s\n", NODE_NAME);
		return -ENOMEM;
	}
	
	LOG("/proc/%s installed!\n", NODE_NAME);
	
	return 0;
}


static void __exit completion(void)
{
	remove_proc_entry(NODE_NAME, NULL);
	LOG("/proc/%s removed!\n", NODE_NAME);
}


MODULE_LICENSE("GPL");
module_init(start);
module_exit(completion);
