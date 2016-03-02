typedef struct
{
    uint8_t reqStatus;       
    uint8_t retryCount;
    uint8_t command;         
    uint8_t offsetData;      
    uint8_t length;          
    union
    {
    uint8_t msgData[ 16 ];   
	unsigned char	*pdatabytes;			
    }payload_u;
} cbus_req_t;
bool_t 	ITE6811MhlTxChipInitialize( void );
void 	ITE6811MhlTxDeviceIsr( void );
bool_t	ITE6811MhlTxDrvSendCbusCommand ( cbus_req_t *pReq  );
bool_t ITE6811MhlTxDrvCBusBusy(void);
void	ITE6811MhlTxDrvTmdsControl( bool_t enable );
void	ITE6811MhlTxDrvPowBitChange( bool_t powerOn );
void	ITE6811MhlTxDrvNotifyEdidChange ( void );
bool_t ITE6811MhlTxReadDevcap( uint8_t offset );
void ITE6811MhlTxDrvGetScratchPad(uint8_t startReg,uint8_t *pData,uint8_t length);
void ITEMhlTxDrvSetClkMode(uint8_t clkMode);
extern	void	ITE6811MhlTxNotifyDsHpdChange( uint8_t dsHpdStatus );
extern	void	ITE6811MhlTxNotifyConnection( bool_t mhlConnected );
extern	void	ITE6811MhlTxMscCommandDone( uint8_t data1 );
extern	void	ITE6811MhlTxMscWriteBurstDone( uint8_t data1 );
extern	void	ITE6811MhlTxGotMhlIntr( uint8_t intr_0, uint8_t intr_1 );
extern	void	ITE6811MhlTxGotMhlStatus( uint8_t status_0, uint8_t status_1 );
extern	void	ITE6811MhlTxGotMhlMscMsg( uint8_t subCommand, uint8_t cmdData );
extern	void	ITE6811MhlTxGotMhlWriteBurst( uint8_t *spadArray );
