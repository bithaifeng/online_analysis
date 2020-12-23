
//########################################################
//##            Receiver Related Information            ##
//########################################################

// HMTT DMA Buffer Registers
//#define DMA_BUF_ADDR			0x0000000300100000ULL
//#define DMA_BUF_ADDR			0x0000001E00100000ULL //120G
#define DMA_BUF_ADDR			0x0000001400100000ULL // 80G
//#define DMA_BUF_SIZE			0x0000000080000000ULL
#define DMA_BUF_SIZE			0x0000000200000000ULL
//#define DMA_REG_ADDR			0x0000001E00000000ULL
#define DMA_REG_ADDR			0x0000001400000000ULL
//#define DMA_REG_ADDR			0x0000000300000000ULL
#define DMA_SEG_SIZE			0x0000000004000000ULL

//#########################################################
//##             Sender Related Information              ##
//#########################################################

// HMTT Config Space Range Registers
#define CFG_START			0x0000000ff0000000ULL  
#define	CFG_END				0x0000000ff0100000ULL

// HMTT Valid Trace Range Registers
//#define PA_RANGE_START			0x00000000c0000000ULL
//#define PA_RANGE_END			0x00000000ffffffffULL

#define PA_RANGE_START			0x0000000000000000ULL
//#define PA_RANGE_START			0x0000000800000000ULL
#define PA_RANGE_END			0x000000ffffffffffULL
// HMTT Physical Address Map Registers
#define	PA_MAP_1_2			0x000001030000d41eULL
#define	PA_MAP_3_4			0x0000010500000104ULL
#define	PA_MAP_5_6			0x0000010700000106ULL
#define	PA_MAP_7_8			0x0002332100000108ULL
#define	PA_MAP_9_10			0x0000010900023322ULL
#define	PA_MAP_11_12			0x0000010b0000010aULL
#define	PA_MAP_13_14			0x0000f41f0000010cULL
#define	PA_MAP_15_16			0x0001141d0001041cULL
#define	PA_MAP_17_18			0x0000010f0000010dULL
#define	PA_MAP_19_20			0x0000011100000110ULL
#define	PA_MAP_21_22			0x0000011300000112ULL
#define	PA_MAP_23_24			0x0000011500000114ULL
#define	PA_MAP_25_26			0x000001160000010eULL
#define PA_MAP_27_28			0x0000011800000117ULL
#define	PA_MAP_29_30			0x0000011a00000119ULL
#define	PA_MAP_31_32			0x0000000000000000ULL
#define	PA_MAP_33_34			0x0000000000000000ULL
#define	PA_MAP_35_36			0x0000000000000000ULL

// HMTT Physical Address Trace Config Registers
#define	PA_TRACE_CFG			0x000000000001000fULL
