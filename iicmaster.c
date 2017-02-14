/*
 * iicmaster is a low-level I2C test and debug tool for IBM internal use only
 */


#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include "iicmaster.h"
#include <getopt.h>

#define IICMASTER_MAX_CMD_FILE_SZ    2048	

#define DEBUG( fmt, args... )  if(dbg) printf("iicmaster: "fmt, ## args)

int dbg = 0;	
char* tmpstr = 0;
int buf_len = 0;
char *buf = 0;
char *cl_data = 0;
int filed = -1;
int cmdfiled = -1;	
char *bus = 0;
int scan = 0;
int scan_dbg = 0;
int display = 1;
int repeat = 0;
		
adal_iic_lock_t lock;

adal_iic_opts_t g_opts =  
{
	.xfr_opts = ADAL_IIC_XFR_DEFAULT,
	.recovery = ADAL_IIC_REC_NONE,
};

enum
{
	IIC_MIN_CMD = 0x0fff0000, /* keep adal ordinals from overalapping */
	IIC_OPEN,
	IIC_READ,
	IIC_WRITE,
	IIC_RESET,
	IIC_DUMP,
	IIC_LOCK,
	IIC_LOCK_ENG,
	IIC_CL_DATA,
	IIC_FILE,
	IIC_SET_TYPE,
	IIC_SET_OPT,
	IIC_SLOW,
	IIC_FAST,
	IIC_SPEED,
	IIC_FLAG_NO_DMA,
	IIC_FLAG_FORCE_DMA,
	IIC_DELAY,
	IIC_FLAG_SPECIAL_RD,
	IIC_CMD_FILE,	
	IIC_VERBOSE,	
	IIC_DEBUG,
	IIC_SCAN_WIDTH,
	IIC_SCAN,
	IIC_SCAN_DBG,
};

#define DEV_STR_SZ 32
struct type_conv
{
	char str[DEV_STR_SZ];
	adal_iic_opts_t dev_opts;
};

/* device type table */
struct type_conv dev_table[] = {
{
	.str	= "24C02",
	.dev_opts.xfr_opts = ADAL_IIC_XFR_24C02,
	.dev_opts.recovery = ADAL_IIC_REC_NONE,
},
{
	.str	= "24C04",
	.dev_opts.xfr_opts = ADAL_IIC_XFR_24C04,
	.dev_opts.recovery = ADAL_IIC_REC_NONE,
},
{
	.str	= "24C08",
	.dev_opts.xfr_opts = ADAL_IIC_XFR_24C08,
	.dev_opts.recovery = ADAL_IIC_REC_NONE,
},
{
	.str	= "24C16",
	.dev_opts.xfr_opts = ADAL_IIC_XFR_24C16,
	.dev_opts.recovery = ADAL_IIC_REC_NONE,
},
{
        .str    = "24C32",
        .dev_opts.xfr_opts = ADAL_IIC_XFR_24C64,
        .dev_opts.recovery = ADAL_IIC_REC_NONE,
},
{
	.str	= "24C64",
	.dev_opts.xfr_opts = ADAL_IIC_XFR_24C64,
	.dev_opts.recovery = ADAL_IIC_REC_NONE,
},
{
	.str	= "24C256",
	.dev_opts.xfr_opts = ADAL_IIC_XFR_24C256,
	.dev_opts.recovery = ADAL_IIC_REC_NONE,
},
{
	.str	= "24C512",
	.dev_opts.xfr_opts = ADAL_IIC_XFR_24C512,
	.dev_opts.recovery = ADAL_IIC_REC_NONE,
},
{
	.str = "",
	.dev_opts = {{0,},{0,}}
},
};

adal_iic_opts_t* dev_str2opts(char* str)
{
	int i = 0;
	adal_iic_opts_t* opts = 0;
	if(!str || !str[0])
	{
		return 0;
	}
	while(dev_table[i].str[0] && strncmp(str, dev_table[i].str, DEV_STR_SZ))
	{
		i++;
	}
	if(dev_table[i].str[0])
	{
		opts = &dev_table[i].dev_opts;
	}
	return opts;
}

void sighandler(int sig)
{
	printf("caught sig %d\n", sig);
}

struct sigaction act =
{
	.sa_handler = sighandler,
};

int handle_cl_data(char* i_buf)
{
	int count = strlen(i_buf);
	char a;
	int n;
	int i;

	buf_len = count/2;
	buf = (char*)realloc(buf, buf_len);
	for(i = 0; i < count; i++)
	{
		a = i_buf[i];
		if(a >= '0' && a <= '9')
			n = a - '0';
		else if(a >= 'a' && a <= 'f')
			n = a - 'a' + 10;
		else if (a >= 'A' && a <= 'F')
			n = a - 'A' + 10;
		else{
			printf("\nBad value in data string: '%c'\n", a);
			return 1;
		}
		if(!(i%2))
		{
			buf[i/2] = n << 4;
		}
		else
		{
			buf[i/2] |= n;
		}
	}
	return 0;
}

/* read up to sz bytes from opened file to buf */
void read_buf(int i_filed, char* buf, int sz)
{
	int rc;
	
	rc = lseek(i_filed, 0, SEEK_SET);
	rc = read(i_filed, buf, sz);
}

/* write sz bytes from buf to opened file */
void write_buf(char* buf, int sz)
{
	int rc;
	rc = lseek(filed, 0, SEEK_SET);
	rc = write(filed, buf, sz);
}

void display_buf(char* buf, int sz) 
{
	int n;
	for(n = 0; n < sz; n++)
	{
		printf("%02X ", buf[n]);
		if(!((n + 1) % 16))
		{
			printf("\n");
		}
	}
	printf("\n");
}

int handle_file(const char * i_file, int * i_filed, unsigned long flags)
{
	if(*i_filed >= 0)
		close(*i_filed);
	*i_filed = open(i_file, flags);
	if(*i_filed < 0)
	{
		perror("iicmaster: file open failed");
		printf("iicmaster: Could not open %s\n", i_file);
		return 1;
	}
	return 0;
}

static void show_lck_msg(int cmd, char *bus, adal_iic_lock_t *lock, int lck_time)
{
	if (cmd == IIC_LOCK_ENG) {
		char *eng = strtok(bus, "P");
		fprintf(stderr,
			"!!The IIC engine %s"
			" is %s (%ds)\n", eng, (lck_time)? "locked": "unlocked",
			lck_time);
	} else
		fprintf(stderr,
			"!!The IIC device"
			" bus:%s addr:%x(0x%x)"
			" is %s (%ds)\n",
			bus,
			lock->addr,
			(lock->mask >> 1), (lck_time)? "locked": "unlocked",
			lck_time);
}

int main(int argc, char * const *argv)
{
	int cmd = 0;
	int rc = 0;
	int adal = 0;
	int temp;
	int adal_dormant = 0;  
	int adal_temp = 0;    
	int opt_index = 0;
	char usg_str[2048];
	char active_devstr[128];   
	char dormant_devstr[128];  
	char temp_devstr[128];     
	char *cmdbuf = 0;	  
	char *const *cmd_argv;    
	int cmd_argc = 0;         
	int verbose = 0;          
	int slv_addr = 0;
	int found = 0;
	char found_list[128];
	unsigned char sample_buf = 0;
	int scan_speed = ADAL_IIC_VAL_100KHZ;
	int scan_offset = 0;
	int scan_width = 1;
	int i;
	const char *version = "2.12";          

	sprintf(usg_str, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		"[[OPTION]* [OPERATION]]*\n\n",
		"iicmaster (pronounced ick-master) is a tool for performing low level\n",
		"operations on an I2C bus.  Multiple operations may be performed on a\n",
		"single command line.  Each specified option will only affect\n",
		"operations that follow it on the command line.  Since all operations\n",
		"are done on a bus, a bus must be specified first, using the\n",
		"-b <bus path> option, where <bus_path> has the following format:\n\n",
		"    /dev/iic/<link#>/<cfam#>/<eng#>/<port#>\n\nOPERATIONS\n",
		"The special options, -r, -w, -u, -l, --reset, and --dump are\n",
		"considered operations.\n\nOPTIONS\n",
		"All other options are used to affect the behavior of the operations\n",
		"that follow and do nothing by themselves.  Options affect all\n",
		"operations that follow until a new bus is specified or the option is\n",
		"explicitly changed.\n\nEXAMPLES:\n",
		"1) Write 0xDEADBEEF (4 bytes) to the device with address 0xA0.\n\n", 
		" iicmaster -b /dev/iic/0/0/14/0 -a 0xa0 --data deadbeef -w 4\n\n",
		"2) Read 4 bytes from the device with address 0xA0.\n\n",
		" iicmaster -b /dev/iic/0/0/14/0 -a 0xa0 -r 4\n\n",
		"3) Write 0xDEADBEEF at offset 0 of device at 0xA0 and read it back.\n\n",
		" iicmaster -b /dev/iic/0/0/14/0 -a 0xa0 -W 2 -o 0 --data deadbeef ",
		"-w 4 -o 0 -r 4\n\n",
		"4) Scan a bus for devices.\n\n", 
		" iicmaster -b /dev/iic/0/0/14/0 --scan\n\n",
		"OPTIONS AND OPERATIONS\n");
	sigaction(SIGINT, &act, 0);

	struct option long_options[] = {
		{ "reset", optional_argument, 0, IIC_RESET },
		{ "bus", required_argument, 0, 'b' },
		{ "delay", required_argument, 0, 'D' },
		{ "read", required_argument, 0, 'r' },
		{ "write", required_argument, 0, 'w' },
		{ "lock", required_argument, 0, 'l' },
		{ "lock_eng", required_argument, 0, IIC_LOCK_ENG },
		{ "dev_addr", required_argument, 0, 'a' },
		{ "dev_type", required_argument, 0, 't' },
		{ "offset", required_argument, 0, 'o' },
		{ "wdelay", required_argument, 0, 'd' },
		{ "rdelay", required_argument, 0, 'y' },
		{ "dev_width", required_argument, 0, 'W' },
		{ "inc_addr", required_argument, 0, 'i' },
		{ "slow", no_argument, 0, 's' },
		{ "fast", no_argument, 0, 'f' },
		{ "speed", required_argument, 0, IIC_SPEED },
		{ "retry_pol", required_argument, 0, ADAL_IIC_CFG_REDO_POL },
		{ "retry_delay", required_argument, 0, ADAL_IIC_CFG_REDO_DELAY },
		{ "no_dma", no_argument, 0, IIC_FLAG_NO_DMA },
		{ "force_dma", no_argument, 0, IIC_FLAG_FORCE_DMA },
		{ "modified", no_argument, 0, 'M' },
		{ "time", required_argument, 0, 'm' },
		{ "rsplit", required_argument, 0, 'x' },
		{ "wsplit", required_argument, 0, 'p' },
		{ "repeat", required_argument, 0, 0 },
		{ "display", required_argument, 0, 0 },
		{ "data", required_argument, 0, IIC_CL_DATA },
		{ "file", required_argument, 0, IIC_FILE },
		{ "cmdfile", required_argument, 0, IIC_CMD_FILE },
		{ "dump", no_argument, 0, IIC_DUMP },
		{ "verbose", no_argument, 0, 'v' },
		{ "debug", no_argument, 0, IIC_DEBUG },
		{ "scan", no_argument, 0, IIC_SCAN },
		{ "scanw", required_argument, 0, IIC_SCAN_WIDTH },
		{ "scan_dbg", no_argument, 0, IIC_SCAN_DBG },
		{ 0, 0, 0, 0 }
	};

	while((cmd = getopt_long(argc, argv, "b:D:r:w:l:a:t:o:d:y:W:i:sfm:x:p:v", long_options, &opt_index)) > 0)
	{
		if(optarg)
		{
			temp = strtoul(optarg, 0, 0);
		}
		if(!adal && (cmd != 'b' && cmd != IIC_CMD_FILE && 
			     cmd != 'v' && cmd != IIC_DEBUG))
		{
			printf("bus must be specified first\n");
			exit(-1);
		}
		switch(cmd)
		{
			case 'b':
				cmd = IIC_OPEN;
				DEBUG("OPEN\n");
				if(adal)
				{
					/* does new bus already have an open adal handle ? */
					if(strcmp(active_devstr, optarg) == 0)
					{
						/* new bus is same as current adal handle so do nothing */
						DEBUG("%s already open\n", active_devstr);
						break;
					} 
					else if(strcmp(dormant_devstr, optarg) == 0) {
						/* new bus is same as dormant adal handle */
						/* swap dormant & active states of open adal handles */
					        adal_temp = adal_dormant;
						adal_dormant = adal;
						adal = adal_temp;
	
						strcpy(temp_devstr, dormant_devstr);
						strcpy(dormant_devstr, active_devstr);
						strcpy(active_devstr, temp_devstr);				

						DEBUG("%s dormant -> active\n", active_devstr);

						break;
					}
			
					/* new bus does not have a currently open adal handle */

                                        if(adal_dormant) 
					{
						/* already two handles open so close the oldest one */
                                                rc = close(adal_dormant);                                                
						DEBUG("%s closed\n", dormant_devstr);
					}

					adal_dormant = adal;  /* previous active bus is now dormant */
					strcpy(dormant_devstr, active_devstr);
					DEBUG("%s dormant\n", dormant_devstr);
				
				}
				adal = open(optarg, O_RDWR | O_SYNC);
				strcpy(active_devstr, optarg);

				DEBUG("%s active\n", active_devstr);
				if(verbose) printf("current bus: %s\n", active_devstr);
				break;
			case IIC_RESET:
				if(optarg[0] == 'l')
				{
					DEBUG("RESET light\n");
					rc = ioctl(adal, _IO(IIC_IOC_MAGIC, IIC_IOC_RESET_LIGHT), 0);	
				}
				else if(optarg[0] == 'f')
				{
					DEBUG("RESET full\n");
					rc = ioctl(adal, _IO(IIC_IOC_MAGIC, IIC_IOC_RESET_FULL), 0);	
				}
				else
				{
					printf("Invalid Reset Type\n");
					exit(-1);
				}
				if(rc < 0)
				{
					if (rc == -1)
						printf("bus is stuck.\n");
					else
						perror("reset: adal_iic_reset "
						       "failed");
					exit(-1);
				}
				if(verbose) printf("reset\n");
				break;
			case IIC_DUMP:
				rc = ioctl(adal, _IOW(IIC_IOC_MAGIC, IIC_IOC_DISPLAY_REGS, long), 0);
				if(rc)
				{
					perror("dump: adal_iic_config failed");
					exit(-1);
				}
				if(verbose) printf("dump\n");
				break;
			case IIC_CL_DATA:
				DEBUG("CL_DATA\n");
				rc = handle_cl_data(optarg);
				if(rc)
					exit(-1);
				break;
			case IIC_FILE:
				if(verbose) printf("open data file: %s\n", optarg);          
				rc = handle_file(optarg, &filed, O_RDWR | O_SYNC | O_CREAT); 
				if(rc)
					exit(-1);
				break;
			case IIC_CMD_FILE:
				if(verbose) printf("open command file: %s\n", optarg);    
				rc = handle_file(optarg, &cmdfiled, O_RDONLY | O_SYNC | O_CREAT); 
				if(rc)
					exit(-1);

				cmdbuf = (char*)realloc(cmdbuf, IICMASTER_MAX_CMD_FILE_SZ);
				read_buf(cmdfiled, cmdbuf, IICMASTER_MAX_CMD_FILE_SZ);

				DEBUG("command file string:\n%s\n", cmdbuf);

				DEBUG("cmd file arg count:%d\n", cmd_argc);

				break;
			case 'r':
				cmd = IIC_READ;
				if(temp)
				{
					buf_len = temp;
				}
				buf = (char*)realloc(buf, buf_len);
				if(buf_len && !buf)
				{
					printf("realloc failed\n");
					exit(-1);
				}
				do{
					rc = read(adal, buf, buf_len);
					if(display && (rc > 0))
					{
						if(verbose) 
						{
							printf("read :%s @%02x offset:%d = ", 
								active_devstr, g_opts.xfr_opts.dev_addr,
								(int)g_opts.xfr_opts.offset);
						}
						display_buf(buf, rc); 
					}
					if((filed >= 0) && (rc > 0))
					{
						write_buf(buf, rc);
					}
				} while(rc == buf_len && repeat--);
				repeat = 0;
				if(rc < 0)
				{
					perror("adal_iic_read failed");
					exit(-1);
				}
				/* copy data to file if specified */
				break;
			case 'w':
				cmd = IIC_WRITE;
				if(temp)
				{
					buf_len = temp;
				}
				buf = (char*)realloc(buf, buf_len);
				if(buf_len && !buf)
				{
					printf("realloc failed\n");
					exit(-1);
				}
				if(filed >= 0)
				{
					read_buf(filed, buf, buf_len);
				}
				do{
					rc = write(adal, buf, buf_len);
					if(display && (rc > 0))
					{
						if(verbose) 
						{
							printf("write data to %s @%02x offset:%d = ", 
								active_devstr, g_opts.xfr_opts.dev_addr,
								(int)g_opts.xfr_opts.offset);
						}
						display_buf(buf, rc); 
					}
				} while(rc == buf_len && repeat--);
				repeat = 0;
				if(rc < 0)
				{
					perror("adal_iic_write failed");
					exit(-1);
				}
				repeat = 0;
				break;
			case 'l':
				cmd = IIC_LOCK;
			case IIC_LOCK_ENG:
				lock.addr = g_opts.xfr_opts.dev_addr;
				lock.mask = (cmd == IIC_LOCK_ENG)
					? ADAL_IIC_VAL_LCK_ENG
					: g_opts.xfr_opts.inc_addr;
				lock.timeout = g_opts.xfr_opts.timeout;

				if(verbose)
				{
					printf("lock %s @%02x mask:%08x\n",
						active_devstr, g_opts.xfr_opts.dev_addr,
						(unsigned int)g_opts.xfr_opts.inc_addr);		
				}

				rc = ioctl(adal, _IOW(IIC_IOC_MAGIC, (cmd == IIC_LOCK_ENG) ? ADAL_IIC_CFG_LCK_ENG : ADAL_IIC_CFG_LCK_ADDR, long), &lock);
				if(rc)
				{
					perror("lock: adal_iic_config failed");
					exit(-1);
				}

				rc = daemon(0, 1);
				if (rc < 0) {
					rc = errno;
					printf("daemoizing failed\n");
					break;
				}

				show_lck_msg(cmd, bus, &lock, temp);
				while (temp--) {
					sleep(1);
					/* show lck message every 5 seconds */
					if (!(temp % 5) && temp)
						show_lck_msg(cmd, bus, &lock,
							     temp);
				}

				rc = ioctl(adal, _IOW(IIC_IOC_MAGIC, (cmd == IIC_LOCK_ENG) ? ADAL_IIC_CFG_ULCK_ENG : ADAL_IIC_CFG_ULCK_ADDR, long), &lock);
				if(rc)
				{
					perror("unlock: adal_iic_config failed");
					exit(-1);
				}
				show_lck_msg(cmd, bus, &lock, 0);
				break;
			case 't':
				cmd = IIC_SET_TYPE;
				DEBUG("SET_TYPE\n");
				{
					adal_iic_opts_t *opts;
					if(!(opts = dev_str2opts(optarg)))
					{
						printf("%s not supported\n",
								optarg);
						exit(-1);
					}
					memcpy(&g_opts, opts, sizeof(*opts));
				}
				rc = ioctl(adal, _IOW(IIC_IOC_MAGIC, ADAL_IIC_CFG_ALL, long), &g_opts);
				if(rc)
				{
					perror("type: adal_iic_config failed");
					exit(-1);
				}
				break;
			case 's':
				cmd = IIC_SLOW;
				{
					scan_speed = ADAL_IIC_VAL_100KHZ;
					rc = ioctl(adal, _IOW(IIC_IOC_MAGIC, ADAL_IIC_CFG_SPEED, long), &scan_speed);
					if(rc < 0)
					{
						perror("set slow failed");
						exit(-1);
					}
				}
				if(verbose) printf("set 100 KHz speed\n");
				break;
			case 'f':
				cmd = IIC_FAST;
				{
					scan_speed = ADAL_IIC_VAL_400KHZ;
					rc = ioctl(adal, _IOW(IIC_IOC_MAGIC, ADAL_IIC_CFG_SPEED, long), &scan_speed);
					if(rc < 0)
					{
						perror("set fast failed");
						exit(-1);
					}
				}
				if(verbose) printf("set 400 KHz speed\n");
				break;
			case IIC_SPEED:
				{
				  	if((temp > 1000) || (temp < 1))
					{
						printf("invalid speed\n");
						exit(-1);
					}
					scan_speed = temp;
					rc = ioctl(adal, _IOW(IIC_IOC_MAGIC, ADAL_IIC_CFG_SPEED, long), &scan_speed);
					if(rc < 0)
					{
						perror("set speed failed");
						exit(-1);
					}
					printf("set %dKHz speed\n", rc);
				}
				break;
			case IIC_FLAG_NO_DMA:
				if(verbose) printf("Don't use DMA\n");
				{
					long flag = ADAL_IIC_FLAG_NO_DMA;
				}
				if(rc)
				{
					perror("set no dma failed");
					exit(-1);
				}
				break;
			case IIC_FLAG_FORCE_DMA:
				if(verbose) printf("Force DMA\n");
				{
					long flag = ADAL_IIC_FLAG_FORCE_DMA;
				}
				if(rc)
				{
					perror("set force dma failed");
					exit(-1);
				}
				break;
			case 'M':
				cmd = IIC_FLAG_SPECIAL_RD;
				if(verbose) printf("special read mode\n");
				{
					long flag = ADAL_IIC_FLAG_SPECIAL_RD;
					rc = ioctl(adal, _IOW(IIC_IOC_MAGIC, ADAL_IIC_CFG_FLAGS, long), &flag);
				}
				if(rc)
				{
					perror("set special read failed");
					exit(-1);
				}
				break;
			case IIC_DELAY:
				if(verbose) printf("delay: %d\n", temp);
				{
					struct timespec ts = {
						.tv_sec = temp/1000,
						.tv_nsec = 
						    (temp % 1000) * 1000000,
					};
					nanosleep(&ts, 0);
				}
				break;
			case 'a':
				cmd = ADAL_IIC_CFG_DEV_ADDR;
				if(verbose) printf("set address: %02x\n", temp);
				g_opts.xfr_opts.dev_addr = temp;
				break;
			case 'W':
				cmd = ADAL_IIC_CFG_DEV_WIDTH;
				if(verbose) printf("set width: %08x\n", temp);
				g_opts.xfr_opts.dev_width = temp;
				break;
			case 'o':
				cmd = ADAL_IIC_CFG_OFFSET;
				if(verbose) printf("set offset: %d\n", temp);
				g_opts.xfr_opts.offset = temp;
				break;
			case 'i':
				cmd = ADAL_IIC_CFG_INC_ADDR;
				if(verbose) printf("increment address: %d\n", temp);
				g_opts.xfr_opts.inc_addr = temp;
				break;
			case 'm':
				cmd = ADAL_IIC_CFG_TIMEOUT;
				if(verbose) printf("set timeout: %d milliseconds\n", temp);
				g_opts.xfr_opts.timeout = temp;
				break;
			case 'y':
				cmd = ADAL_IIC_CFG_RDELAY;
				if(verbose) printf("set read delay: %d milliseconds\n", temp);
				g_opts.xfr_opts.rdelay = temp;
				break;
			case 'd':
				cmd = ADAL_IIC_CFG_WDELAY;
				if(verbose) printf("set write delay: %d milliseconds\n", temp);
				g_opts.xfr_opts.wdelay = temp;
				break;
			case 'x':
				cmd = ADAL_IIC_CFG_RSPLIT;
				if(verbose) printf("set rsplit: %d\n", temp);
				g_opts.xfr_opts.rsplit = temp;
				break;
			case 'p':
				cmd = ADAL_IIC_CFG_WSPLIT;
				if(verbose) printf("set wsplit: %d\n", temp);
				g_opts.xfr_opts.wsplit = temp;
				break;
			case ADAL_IIC_CFG_REDO_POL:
				if(verbose) printf("set retry policy: %d\n", temp);
				g_opts.recovery.redo_pol = temp;
				break;
			case ADAL_IIC_CFG_REDO_DELAY:
				if(verbose) printf("set retry delay: %d milliseconds\n", temp);
				g_opts.recovery.redo_delay = temp;
				break;
			case 'v':
				cmd = IIC_VERBOSE;
				DEBUG("VERBOSE\n");
				verbose = 1;
				break;
			case IIC_DEBUG:
				dbg = temp;
                                DEBUG("version %s debug:%d\n", version, temp);
				break;
			case IIC_SCAN_WIDTH:
				if (verbose) printf("set scan offset width: %d bytes\n", temp);
				scan_width = temp;
				break;
			case IIC_SCAN_DBG:
				scan_dbg = 1;
			case IIC_SCAN:
				scan = 1;
				memset(found_list, 0, 128);
				printf("Scanning for devices on bus:");
				errno = 0;
				rc = ioctl(adal, _IOW(IIC_IOC_MAGIC, ADAL_IIC_CFG_DEV_WIDTH, long), &scan_width);
				if(rc < 0)
				{
					printf("\nerror setting config width, errno=%d\n", errno);
				}
				errno = 0;
				rc = ioctl(adal, _IOW(IIC_IOC_MAGIC, ADAL_IIC_CFG_SPEED, long), &scan_speed);
				if(rc < 0)
				{
					printf("\nerror setting config speed, errno=%d\n", errno);
				}
				errno = 0;
				rc = ioctl(adal, _IOW(IIC_IOC_MAGIC, ADAL_IIC_CFG_OFFSET, long), &scan_offset);
				if(rc < 0)
				{
					printf("\nerror setting config offset, errno=%d\n", errno);
				}

				/* walk each address on the bus and look for ACK's */
				for(slv_addr = 0; slv_addr < 0x100; slv_addr += 2) {

					if((slv_addr % 0x80) == 0) {
						printf("\n");
					} else {
						printf(".");
					}

					errno = 0;
					rc = ioctl(adal, _IOW(IIC_IOC_MAGIC, ADAL_IIC_CFG_DEV_ADDR, long), &slv_addr);
					if(rc < 0)
					{
						printf("error setting config slave address, addr:0x%02x errno=%d\n", 
							slv_addr, errno);
					}

					errno = 0;
					rc = read(adal, &sample_buf, 1);
					if((errno == EALREADY) ||
					   ((scan_dbg) && (errno == ETIME)))
					{
						/* can't scan the bus, something wrong with the clock
						   and/or data line */
						scan = 0;
						exit(-1);						
					}

					if ((!scan_dbg) && (errno == ETIME)) {
						ioctl(adal, _IO(IIC_IOC_MAGIC, IIC_IOC_RESET_LIGHT), 0);
					} else if(errno != ENXIO) {
						found_list[found++] = slv_addr;
					}
				} 

				printf("\nFound %d device(s):\n[ ", found);
				for(i = 0; i < found; i++) {
					printf("%02X  ", found_list[i]);
				}
				printf("]\n");

				break;
			default:
				printf("iicmaster: Unsupported command %d\n", cmd);
				exit(-1);
		}
		if(cmd < IIC_MIN_CMD)
		{
				rc = ioctl(adal, _IOW(IIC_IOC_MAGIC, cmd, long), &temp);
				if(rc)
				{
					perror("adal_iic_config failed");
					printf("cmd=%08x, val=%08x\n",
							cmd, temp);
					exit(-1);
				}
		}
		tmpstr = 0;
	} // end while
	if(cmd < -1)
	{
		if(buf) free(buf);
		if(cmdbuf) free(cmdbuf);

		exit(-1);
	}
	else
	{
		if(buf) free(buf);
		if(cmdbuf) free(cmdbuf);

		printf("iicmaster: Done.\n");
		exit(0);
	}
}

