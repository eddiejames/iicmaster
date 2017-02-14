#include <sys/ioctl.h>

#define  I2C_RDRW_IOCTL_MAX_MSGS        42

typedef struct iic_rec_pol
{
	unsigned long redo_pol;
#define IIC_VAL_ADDR_NOACK	0x00010000
#define IIC_VAL_DATA_NOACK	0x00020000
#define IIC_VAL_TIMEOUT		0x00040000
#define IIC_VAL_LOST_ARB	0x00080000
#define IIC_VAL_BUS_ERR		0x00100000
#define IIC_VAL_ALL_ERRS	0xffff0000
	unsigned long rsvd;
	unsigned long redo_delay;
} iic_rec_pol_t;

#define IIC_VAL_100KHZ  100
#define IIC_VAL_400KHZ	400
typedef struct iic_xfr_opts
{
	unsigned short rsvd;
	unsigned short dev_addr;	// address of end device
	unsigned short dev_width;	// number of bytes for offset (1-4)
	unsigned long inc_addr;		// mask of address bits to increment
					// for devices that span multiple
					// addresses.
	unsigned long timeout;		// operation timeout (msec)
	unsigned short wdelay;		// delay between write xfrs (msec)
	unsigned short rdelay;		// delay between read xfrs (msec)
	unsigned short wsplit;		// splits writes into smaller chunks
	unsigned short rsplit;		// splits reads into smaller chunks
	unsigned long offset;		// offset from beginning of device
	unsigned long flags;		// flags defined below
} iic_xfr_opts_t;

enum 
{
	IIC_FORCE_DMA = 0x01,		// use dma regardless of xfr size
	IIC_NO_DMA = 0x02,		// disallow dma
	IIC_SPECIAL_RD = 0x04,		// workaround for PLL/CRC chips
	IIC_REPEATED_START = 0x08,      // repeated start
};

typedef struct iic_opts
{
	iic_xfr_opts_t xfr_opts;
	iic_rec_pol_t recovery;
} iic_opts_t;

typedef struct iic_lock
{
	unsigned short mask;
	unsigned short addr;
	unsigned long timeout;
} iic_lock_t;

/* Master IOCTL Ordinal Numbers */
#define IIC_IOC_MAGIC 		0x07
enum
{
	/* 0 bytes */
	IIC_IOC_RESET_LIGHT,
	IIC_IOC_RESET_FULL,

	IIC_IOC_0_BYTES = IIC_IOC_RESET_FULL,

	/* 4 bytes */
	IIC_IOC_SPEED,
	IIC_IOC_DEV_ADDR,
	IIC_IOC_DEV_WIDTH,
	IIC_IOC_OFFSET,
	IIC_IOC_INC_ADDR,
	IIC_IOC_TIMEOUT,
	IIC_IOC_RDELAY,
	IIC_IOC_WDELAY,
	IIC_IOC_RSPLIT,
	IIC_IOC_WSPLIT,
	IIC_IOC_REDO_POL,
	IIC_IOC_SPD_POL,
	IIC_IOC_REDO_DELAY,
	IIC_IOC_BUS_STATE,
#define IIC_VAL_BOTH_LO 0x00
#define IIC_VAL_SDA_LO  0x01
#define IIC_VAL_SCL_LO  0x02
#define IIC_VAL_BOTH_HI 0x03
	IIC_IOC_FLAGS,

	IIC_IOC_4_BYTES = IIC_IOC_FLAGS,

	/* Objects */
	IIC_IOC_LCK_ADDR,
	IIC_IOC_ULCK_ADDR,
	IIC_IOC_LCK_ENG,
	IIC_IOC_ULCK_ENG,
	IIC_IOC_ALL,
	IIC_IOC_DISPLAY_REGS,
	IIC_IOC_REPEATED_IO,
	IIC_IOC_MAXNR = IIC_IOC_REPEATED_IO,
};


#define ADAL_IIC_XFR_DEFAULT \
{\
	/*speed*/	ADAL_IIC_VAL_400KHZ,\
	/*dev_addr*/	0x0,\
	/*dev_width*/	0,\
	/*inc_addr*/	0,\
	/*timeout*/	5000,\
	/*wdelay*/	0,\
	/*rdelay*/	0,\
	/*wsplit*/	0,\
	/*rsplit*/	32768,\
	/*offset*/	0,\
	/*flags*/	0,\
} 

#define ADAL_IIC_XFR_24C02 \
{\
	/*speed*/	ADAL_IIC_VAL_400KHZ,\
	/*dev_addr*/	0x0,\
	/*dev_width*/	ADAL_IIC_VAL_1BYTE,\
	/*inc_addr*/	0,\
	/*timeout*/	5000,\
	/*wdelay*/	20,\
	/*rdelay*/	0,\
	/*wsplit*/	8,\
	/*rsplit*/	32768,\
	/*offset*/	0,\
	/*flags*/	0,\
} 

#define ADAL_IIC_XFR_24C04 \
{\
	/*speed*/	ADAL_IIC_VAL_400KHZ,\
	/*dev_addr*/	0x0,\
	/*dev_width*/	ADAL_IIC_VAL_1BYTE,\
	/*inc_addr*/	ADAL_IIC_VAL_1BIT,\
	/*timeout*/	5000,\
	/*wdelay*/	20,\
	/*rdelay*/	0,\
	/*wsplit*/	16,\
	/*rsplit*/	32768,\
	/*offset*/	0,\
	/*flags*/	0,\
} 
#define ADAL_IIC_XFR_24C08 \
{\
	/*speed*/	ADAL_IIC_VAL_400KHZ,\
	/*dev_addr*/	0x0,\
	/*dev_width*/	ADAL_IIC_VAL_1BYTE,\
	/*inc_addr*/	ADAL_IIC_VAL_2BIT,\
	/*timeout*/	5000,\
	/*wdelay*/	20,\
	/*rdelay*/	0,\
	/*wsplit*/	16,\
	/*rsplit*/	32768,\
	/*offset*/	0,\
	/*flags*/	0,\
} 
#define ADAL_IIC_XFR_24C16 \
{\
	/*speed*/	ADAL_IIC_VAL_400KHZ,\
	/*dev_addr*/	0x0,\
	/*dev_width*/	ADAL_IIC_VAL_1BYTE,\
	/*inc_addr*/	ADAL_IIC_VAL_3BIT,\
	/*timeout*/	5000,\
	/*wdelay*/	20,\
	/*rdelay*/	0,\
	/*wsplit*/	16,\
	/*rsplit*/	32768,\
	/*offset*/	0,\
	/*flags*/	0,\
} 
#define ADAL_IIC_XFR_24C64 \
{\
	/*speed*/	ADAL_IIC_VAL_400KHZ,\
	/*dev_addr*/	0x0,\
	/*dev_width*/	ADAL_IIC_VAL_2BYTE,\
	/*inc_addr*/	0,\
	/*timeout*/	10000,\
	/*wdelay*/	20,\
	/*rdelay*/	0,\
	/*wsplit*/	32,\
	/*rsplit*/	32768,\
	/*offset*/	0,\
	/*flags*/	0,\
} 
#define ADAL_IIC_XFR_24C256 \
{\
	/*speed*/	ADAL_IIC_VAL_400KHZ,\
	/*dev_addr*/	0x0,\
	/*dev_width*/	ADAL_IIC_VAL_2BYTE,\
	/*inc_addr*/	0,\
	/*timeout*/	15000,\
	/*wdelay*/	20,\
	/*rdelay*/	0,\
	/*wsplit*/	64,\
	/*rsplit*/	32768,\
	/*offset*/	0,\
	/*flags*/	0,\
} 
#define ADAL_IIC_XFR_24C512 \
{\
	/*speed*/	ADAL_IIC_VAL_400KHZ,\
	/*dev_addr*/	0x0,\
	/*dev_width*/	ADAL_IIC_VAL_2BYTE,\
	/*inc_addr*/	0,\
	/*timeout*/	15000,\
	/*wdelay*/	20,\
	/*rdelay*/	0,\
	/*wsplit*/	128,\
	/*rsplit*/	32768,\
	/*offset*/	0,\
	/*flags*/	0,\
} 

/************************** Recovery Policies *********************************/
#define ADAL_IIC_REC_DO_OR_DIE \
{\
	/*redo_pol*/	ADAL_IIC_VAL_ALL_ERRS | 5,\
	/*spd_pol*/	ADAL_IIC_VAL_ALL_ERRS | 1,\
	/*redo_delay*/	200,\
} 

#define ADAL_IIC_REC_MED \
{\
	/*redo_pol*/	ADAL_IIC_VAL_ALL_ERRS | 2,\
	/*spd_pol*/	ADAL_IIC_VAL_ADDR_NOACK | ADAL_IIC_VAL_DATA_NOACK | 1,\
	/*redo_delay*/	20,\
}

#define ADAL_IIC_REC_LITE \
{\
	/*redo_pol*/	ADAL_IIC_VAL_ALL_ERRS | 1,\
	/*spd_pol*/	0,\
	/*redo_delay*/	20,\
} 

#define ADAL_IIC_REC_NONE {0, } 


/**************************** Convenience Macros *****************************/
#define ADAL_IIC_SET_ADDR(d_opts, s_addr) (d_opts)->xfr_opts.dev_addr = s_addr 

#define ADAL_IIC_INIT_XFR(d_opts, s_xfr) \
({\
	adal_iic_xfr_opts_t t_xfr = s_xfr;\
	memcpy(&((d_opts)->xfr_opts), &t_xfr, sizeof(adal_iic_xfr_opts_t));\
})


#define ADAL_IIC_INIT_RECOVERY(d_opts, s_rec) \
({\
	adal_iic_rec_pol_t t_rec = s_rec;\
	memcpy(&((d_opts)->recovery), &t_rec, sizeof(adal_iic_rec_pol_t));\
})

#define ADAL_IIC_RETRY_ERR_MASK              0xFFFF0000
#define ADAL_IIC_RETRY_ERR_COUNT_MASK        0x0000FFFF

/* ADAL_IIC_SET_RECOVERY
 *
 * Specify all recovery details on fail of a IIC master transfer
 *
 * Parameters
 * d_opts: 	The configuration options allocated by the caller
 * retry_errs:	Errors to retry
 * 
 *		This is a bitwise mask of errors, if more than one type is required 
 *		then logical OR in these types.
 *
 *  	   	ADAL_IIC_VAL_ADDR_NOACK		Address NACK error
 *       	ADAL_IIC_VAL_DATA_NOACK 	Data NACK error
 *       	ADAL_IIC_VAL_TIMEOUT		Timeout error
 *       	ADAL_IIC_VAL_LOST_ARB 		Master lost arbitration error
 *       	ADAL_IIC_VAL_BUS_ERR		Low level bus error
 *       	ADAL_IIC_VAL_ALL_ERRS 		All errors
 *			
 * retry_count:	Number of times to retry on error(s)
 * retry_delay:	Time to wait before retry on error in milliseconds
 */
#define ADAL_IIC_SET_RECOVERY(d_opts, retry_errs, retry_count, retry_delay) \
({\
	(d_opts)->recovery.redo_pol = (retry_errs & ADAL_IIC_RETRY_ERR_MASK);\
	(d_opts)->recovery.redo_pol |= (retry_count & ADAL_IIC_RETRY_ERR_COUNT_MASK);\
	(d_opts)->recovery.redo_delay = retry_delay;\
})	

/*! @enum adal_iic_cfg
* linkage between device driver ioctl values and adal config values
*/
typedef enum {
	ADAL_IIC_CFG_SPEED = IIC_IOC_SPEED,
	ADAL_IIC_CFG_DEV_ADDR = IIC_IOC_DEV_ADDR,
	ADAL_IIC_CFG_DEV_WIDTH = IIC_IOC_DEV_WIDTH,
	ADAL_IIC_CFG_OFFSET = IIC_IOC_OFFSET,
	ADAL_IIC_CFG_INC_ADDR = IIC_IOC_INC_ADDR,
	ADAL_IIC_CFG_TIMEOUT = IIC_IOC_TIMEOUT,
	ADAL_IIC_CFG_RDELAY = IIC_IOC_RDELAY,
	ADAL_IIC_CFG_WDELAY = IIC_IOC_WDELAY,
	ADAL_IIC_CFG_RSPLIT = IIC_IOC_RSPLIT,
	ADAL_IIC_CFG_WSPLIT = IIC_IOC_WSPLIT,
	ADAL_IIC_CFG_REDO_POL = IIC_IOC_REDO_POL,
	ADAL_IIC_CFG_SPD_POL = IIC_IOC_SPD_POL,
	ADAL_IIC_CFG_REDO_DELAY = IIC_IOC_REDO_DELAY,
	ADAL_IIC_CFG_LCK_ADDR = IIC_IOC_LCK_ADDR,
	ADAL_IIC_CFG_ULCK_ADDR = IIC_IOC_ULCK_ADDR,
	ADAL_IIC_CFG_LCK_ENG = IIC_IOC_LCK_ENG,
	ADAL_IIC_CFG_ULCK_ENG = IIC_IOC_ULCK_ENG,
	ADAL_IIC_CFG_ALL = IIC_IOC_ALL,
	ADAL_IIC_CFG_BUS_STATE = IIC_IOC_BUS_STATE,
	ADAL_IIC_CFG_FLAGS = IIC_IOC_FLAGS,
} adal_iic_cfg_t;

/*! @enum adal_iic_val
* IIC device driver specific constants
*/
enum
{
	ADAL_IIC_VAL_100KHZ = 100,
	ADAL_IIC_VAL_400KHZ = 400,
	ADAL_IIC_VAL_0BYTE = 0,
	ADAL_IIC_VAL_1BYTE = 1,
	ADAL_IIC_VAL_2BYTE = 2,
	ADAL_IIC_VAL_3BYTE = 3,
	ADAL_IIC_VAL_4BYTE = 4,
	ADAL_IIC_VAL_0BIT = 0,
	ADAL_IIC_VAL_1BIT = 2,
	ADAL_IIC_VAL_2BIT = 6,
	ADAL_IIC_VAL_3BIT = 14,
	ADAL_IIC_VAL_LCK_ALL = 0x7ff,
	ADAL_IIC_VAL_LCK_ENG = 0xffff,
	ADAL_IIC_VAL_ADDR_NOACK = IIC_VAL_ADDR_NOACK,
	ADAL_IIC_VAL_DATA_NOACK = IIC_VAL_DATA_NOACK,
	ADAL_IIC_VAL_TIMEOUT = IIC_VAL_TIMEOUT,
	ADAL_IIC_VAL_LOST_ARB = IIC_VAL_LOST_ARB,
	ADAL_IIC_VAL_BUS_ERR = IIC_VAL_BUS_ERR,
	ADAL_IIC_VAL_ALL_ERRS = IIC_VAL_ALL_ERRS,
	ADAL_IIC_VAL_BOTH_LO = IIC_VAL_BOTH_LO,
	ADAL_IIC_VAL_SDA_LO = IIC_VAL_SDA_LO,
	ADAL_IIC_VAL_SCL_LO = IIC_VAL_SCL_LO,
	ADAL_IIC_VAL_BOTH_HI = IIC_VAL_BOTH_HI,
	ADAL_IIC_FLAG_NO_DMA = IIC_NO_DMA,
	ADAL_IIC_FLAG_FORCE_DMA = IIC_FORCE_DMA,
	ADAL_IIC_FLAG_SPECIAL_RD = IIC_SPECIAL_RD,
};

typedef iic_opts_t adal_iic_opts_t;
typedef iic_xfr_opts_t adal_iic_xfr_opts_t;
typedef iic_rec_pol_t adal_iic_rec_pol_t;
typedef iic_lock_t adal_iic_lock_t;

#define ADAL_IIC_DDR4_NACK_REDO_POL 0x0002DD40

