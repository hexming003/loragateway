/*
 * lora.h
 *
 *  Created on: May 8, 2019
 *      Author: root
 */

#ifndef LORA_H_
#define LORA_H_

#define LORA_MSG_MAX_RETRAN_TIMES 3
#define LORA_MSG_RETRAN_INTERVAL 3 //3s
typedef struct lora_addr {

  uint8_t addr[6];

} lora_addr_t;

#pragma pack(1)
typedef struct parameter_set_payload
{
	uint8_t block_to_read;  // è¯»å–å¡æ•°æ®ç¬¬nå— é»˜è®¤61
	uint32_t ac_interval;   //é‡‡é›†é—´éš”  é»˜è®¤120

	uint8_t cip_mode;       //åŠ å¯†æ–¹å¼ é»˜è®¤0
	uint8_t cipherbuf[6];   // å¡å¯†ç  é»˜è®¤å…¨FF
	uint8_t running_mode;   //è¿è¡Œæ¨¡å¼ é»˜è®¤0
	uint8_t hotel_id[5];    //é…’åº—ä»£ç  é»˜è®¤ï¼šHPWIN
	uint8_t roomcode[3];    //é»˜è®¤ 0ï¼Œ0ï¼Œ0
	uint8_t room_storage_num; //æ»¡å¤šå°‘ä¸ªæ•°æ®ä¸ŠæŠ¥ é»˜è®¤ï¼š30
	uint8_t reset_relay_flag;  //ç»§ç”µå™¨å®šæ—¶å¤ä½æ ‡å¿—
}parameter_set_payload_t;

typedef struct lora_msg_hdr {
  uint16_t  sync_code;
  uint8_t node_type;
  lora_addr_t src_addr;
                      /* ´«Êä²ã×ª·¢ÏûÏ¢Ê±£¬²»ĞŞ¸ÄÔ´task type */
  lora_addr_t dest_addr; /* Ä¿µÄtask£¬ĞèÒª´«Êä²ã
                      ÖĞ×ªµÄ²»ĞèÒªÌî´«Êä²ãtask type£¬Ö±½ÓÌîÄ¿µÄtask type*/
  uint8_t len; // sn+ÃüÁîÂë+¸ºÔØÊı¾İ+Ğ£ÑéÂë
  uint8_t sn; // seq# if type is a reliable message
  uint8_t cmd;
} lora_msg_hdr_t;
#pragma pack () 


typedef enum LORA_MSG_TYPE
{
    LORA_MSG_TYPE_ACK = 0,
    LORA_MSG_TYPE_BINDING,
    LORA_MSG_TYPE_REQ_PARA,
    LORA_MSG_TYPE_SET_PARA,
    LORA_MSG_TYPE_SET_STATE,
    LORA_MSG_TYPE_BROADCAST_TIME,
    LORA_MSG_TYPE_REQ_DATA,
    
    LORA_MSG_TYPE_BINDING_ACK = 0x81,
    LORA_MSG_TYPE_REQ_PARA_ACK,
    LORA_MSG_TYPE_SET_PARA_ACK,
    LORA_MSG_TYPE_SET_STATE_ACK,
}SERIAL_PKT_TYPE_E;


typedef struct lora_msg_retran_hdr {
    uint8_t retran_flag;
    uint8_t retran_buf[256];
    uint8_t retran_buf_len;
    uint8_t retran_times;
    time_t  retran_time;
} lora_msg_retran_hdr_t;

extern int handle_lora_rx_msg( void *buffer, uint16_t size );
extern uint8_t retran_lora_msg(void);
extern uint8_t get_retran_flag(void);
extern uint8_t init_retran_cb(void);
extern time_t get_tran_time(void);
extern uint8_t set_retran_flag(uint8_t flag);

#endif /* GPIO_H_ */
