#include "lewin.h"
#include <netinet/in.h>
#include "lora.h"

extern  uint8_t lora_transmit_frame(uint8_t *buf, uint32_t len);

////////////////////////////////////
unsigned char auchCRCHi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

unsigned char auchCRCLo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
    0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
    0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
    0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
    0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
    0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
    0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
    0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
    0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
    0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
    0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
    0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
    0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
    0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
    0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
    0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

//uint8_t ParaSet.Devid[6] = {0x11,0x22};
extern struct parameter_set  ParaSet; 


uint8_t g_sn = 0;
lora_msg_retran_hdr_t g_lora_msg_retran;
lora_msg_retran_hdr_t *g_plora_msg_retran = &g_lora_msg_retran;

unsigned int modbus_crc16(unsigned char *puchMsg, unsigned int usDataLen)
{
    unsigned char uchCRCHi = 0xFF; /* 高CRC字节初始化 */
    unsigned char uchCRCLo = 0xFF; /* 低CRC 字节初始化 */
    unsigned long uIndex; /* CRC循环中的索引 */
    while (usDataLen--)
    {
        uIndex = uchCRCHi^*puchMsg++;
        uchCRCHi = uchCRCLo^auchCRCHi[uIndex];
        uchCRCLo = auchCRCLo[uIndex];
    }
    return (uchCRCHi << 8 | uchCRCLo);
}


int check_input_msg(lora_msg_hdr_t *pmsg_hdr)
{
    uint16_t crc;
    uint16_t crc_dst;

    crc = modbus_crc16((unsigned char *)pmsg_hdr, (unsigned int)(pmsg_hdr->len + 16 - 2));

    //crc_dst = *(uint16_t *)((char *)pmsg_hdr + pmsg_hdr->len + 16  - 2 + 1 );
	memcpy((void *)(&crc_dst), (void *)((char *)pmsg_hdr + pmsg_hdr->len + 16  - 2  ),  2);
    #if 1
    if (crc_dst != crc )
    {
        int index;

        printf("crc 0x%x",crc);
        printf("crc_dst 0x%x",crc_dst);
        printf("msg_len 0x%x\n",pmsg_hdr->len);
        for(index = 0; index < pmsg_hdr->len + 16 ; index++)
        {
            printf("0x%x,",*((char *)pmsg_hdr + index) );
        }
        printf("\n");
    }

    #endif
    return crc_dst == crc;
}

uint8_t send_lora_ack_msg(lora_addr_t *pdest_addr, lora_addr_t *psrc_addr, uint8_t sn)
{
    uint8_t status; 
    char send_msg_buf[256];
    lora_msg_hdr_t *pmsg_hdr = (lora_msg_hdr_t *)send_msg_buf;
    uint16_t crc;

    pmsg_hdr->sync_code = 0xeeff;
    pmsg_hdr->node_type = 1;
    memcpy(pmsg_hdr->dest_addr.addr, (void *)pdest_addr->addr, sizeof(lora_addr_t));
    memcpy(pmsg_hdr->src_addr.addr, ParaSet.Devid, sizeof(lora_addr_t));
    
    pmsg_hdr->sn = 0;
    pmsg_hdr->cmd = LORA_MSG_TYPE_ACK;
    
    *(uint8_t *)(send_msg_buf + sizeof(lora_msg_hdr_t)) = sn;
    pmsg_hdr->len = 4 + 1; //固定长度4,负载长度1
    
    crc = modbus_crc16((unsigned char *)pmsg_hdr, (unsigned int)(pmsg_hdr->len + 16 - 2));
    *(uint16_t *)(send_msg_buf + (pmsg_hdr->len + 16 - 2)) = crc;
    
    *(uint8_t *)(send_msg_buf + (pmsg_hdr->len + 16 - 2 + 2)) = 0x16; //包尾

    if(g_plora_msg_retran->retran_flag != FALSE)
    {
        printf("g_retran_flag is invalid\n");
        return -1;
    }
    g_plora_msg_retran->retran_flag = TRUE;
    g_plora_msg_retran->retran_buf_len = (pmsg_hdr->len + 16 - 2 + 2 + 1);
    g_plora_msg_retran->retran_time = abs(time(NULL));
    memcpy(g_plora_msg_retran->retran_buf, send_msg_buf, g_plora_msg_retran->retran_buf_len);
    
    status = lora_transmit_frame(send_msg_buf, (pmsg_hdr->len + 16 - 2 + 2 + 1));
    
    return status;
}

uint8_t send_lora_binding_msg(lora_addr_t *pdest_addr, uint8_t *ppayload, uint8_t len)
{
    uint8_t status; 
    char send_msg_buf[256];
    lora_msg_hdr_t *pmsg_hdr = (lora_msg_hdr_t *)send_msg_buf;
    uint16_t crc;

    pmsg_hdr->sync_code = 0xeeff;
    pmsg_hdr->node_type = 1;
    memcpy(pmsg_hdr->dest_addr.addr, pdest_addr->addr, sizeof(lora_addr_t));
    memcpy(pmsg_hdr->src_addr.addr, ParaSet.Devid, sizeof(lora_addr_t));
    
    pmsg_hdr->sn = g_sn++;
    pmsg_hdr->cmd = LORA_MSG_TYPE_BINDING;
    
    memcpy((uint8_t *)(send_msg_buf + sizeof(lora_msg_hdr_t)), ppayload, len);
    pmsg_hdr->len = 4 + len; //固定长度4,负载长度len
    
    crc = modbus_crc16((unsigned char *)pmsg_hdr, (unsigned int)(pmsg_hdr->len + 16 - 2));
    memcpy((send_msg_buf + (pmsg_hdr->len + 16 - 2)),  &crc, sizeof(uint16_t));
    *(uint8_t *)(send_msg_buf + (pmsg_hdr->len + 16 - 2 + 2)) = 0x16; //包尾

    if(g_plora_msg_retran->retran_flag != FALSE)
    {
        printf("g_retran_flag is invalid\n");
        return -1;
    }
    g_plora_msg_retran->retran_flag = TRUE;
    g_plora_msg_retran->retran_buf_len = (pmsg_hdr->len + 16 - 2 + 2 + 1);
    g_plora_msg_retran->retran_flag = TRUE;
    g_plora_msg_retran->retran_time = abs(time(NULL));
    memcpy(g_plora_msg_retran->retran_buf, send_msg_buf, g_plora_msg_retran->retran_buf_len);

    
    status = lora_transmit_frame(send_msg_buf, (pmsg_hdr->len + 16 - 2 + 2 + 1));
    
    return status;
}

uint8_t send_lora_req_para(lora_addr_t *pdest_addr)
{
    uint8_t status; 
    char send_msg_buf[256];
    lora_msg_hdr_t *pmsg_hdr = (lora_msg_hdr_t *)send_msg_buf;
    uint16_t crc;

    pmsg_hdr->sync_code = 0xeeff;
    pmsg_hdr->node_type = 1;
    memcpy(pmsg_hdr->dest_addr.addr, pdest_addr->addr, sizeof(lora_addr_t));
    memcpy(pmsg_hdr->src_addr.addr, ParaSet.Devid, sizeof(lora_addr_t));
    
    pmsg_hdr->sn = g_sn++;
    pmsg_hdr->cmd = LORA_MSG_TYPE_REQ_PARA;
    
    pmsg_hdr->len = 4 + 0; //固定长度4,负载长度0
    
    crc = modbus_crc16((unsigned char *)pmsg_hdr, (unsigned int)(pmsg_hdr->len + 16 - 2));
    *(uint16_t *)(send_msg_buf + (pmsg_hdr->len + 16 - 2)) = crc;
    
    *(uint8_t *)(send_msg_buf + (pmsg_hdr->len + 16 - 2 + 2)) = 0x16; //包尾
    
    if(g_plora_msg_retran->retran_flag != FALSE)
    {
        printf("g_retran_flag is invalid\n");
        return -1;
    }
    g_plora_msg_retran->retran_flag = TRUE;
    g_plora_msg_retran->retran_buf_len = (pmsg_hdr->len + 16 - 2 + 2 + 1);
    g_plora_msg_retran->retran_time = abs(time(NULL));
    memcpy(g_plora_msg_retran->retran_buf, send_msg_buf, g_plora_msg_retran->retran_buf_len);

    
    status = lora_transmit_frame(send_msg_buf, (pmsg_hdr->len + 16 - 2 + 2 + 1));
    
    return status;
}

uint8_t send_lora_set_para(lora_addr_t *pdest_addr, uint8_t *ppayload, uint8_t len)
{
    uint8_t status; 
    char send_msg_buf[256];
    lora_msg_hdr_t *pmsg_hdr = (lora_msg_hdr_t *)send_msg_buf;
    uint16_t crc;

    pmsg_hdr->sync_code = 0xeeff;
    pmsg_hdr->node_type = 1;
    memcpy(pmsg_hdr->dest_addr.addr, pdest_addr->addr, sizeof(lora_addr_t));
    memcpy(pmsg_hdr->src_addr.addr, ParaSet.Devid, sizeof(lora_addr_t));
    
    pmsg_hdr->sn = g_sn++;
    pmsg_hdr->cmd = LORA_MSG_TYPE_SET_PARA;
    
    memcpy((uint8_t *)(send_msg_buf + sizeof(lora_msg_hdr_t)), ppayload, len);
    pmsg_hdr->len = 4 + len; //固定长度4,负载长度len
    
    crc = modbus_crc16((unsigned char *)pmsg_hdr, (unsigned int)(pmsg_hdr->len + 16 - 2));

    memcpy((send_msg_buf + (pmsg_hdr->len + 16 - 2)),  &crc, sizeof(uint16_t));
    *(uint8_t *)(send_msg_buf + (pmsg_hdr->len + 16 - 2 + 2)) = 0x16; //包尾
    
    if(g_plora_msg_retran->retran_flag != FALSE)
    {
        printf("g_retran_flag is invalid\n");
        return -1;
    }
    g_plora_msg_retran->retran_flag = TRUE;
    g_plora_msg_retran->retran_buf_len = (pmsg_hdr->len + 16 - 2 + 2 + 1);
    g_plora_msg_retran->retran_time = abs(time(NULL));
    memcpy(g_plora_msg_retran->retran_buf, send_msg_buf, g_plora_msg_retran->retran_buf_len);

    status = lora_transmit_frame(send_msg_buf, (pmsg_hdr->len + 16 - 2 + 2 + 1));
    
    return status;
}
uint8_t send_lora_set_state(lora_addr_t *pdest_addr, uint8_t *ppayload, uint8_t len)
{
    uint8_t status; 
    char send_msg_buf[256];
    lora_msg_hdr_t *pmsg_hdr = (lora_msg_hdr_t *)send_msg_buf;
    uint16_t crc;

    pmsg_hdr->sync_code = 0xeeff;
    pmsg_hdr->node_type = 1;
    memcpy(pmsg_hdr->dest_addr.addr, pdest_addr->addr, sizeof(lora_addr_t));
    memcpy(pmsg_hdr->src_addr.addr, ParaSet.Devid, sizeof(lora_addr_t));
    
    pmsg_hdr->sn = g_sn++;
    pmsg_hdr->cmd = LORA_MSG_TYPE_SET_STATE;
    
    memcpy((uint8_t *)(send_msg_buf + sizeof(lora_msg_hdr_t)), ppayload, len);
    pmsg_hdr->len = 4 + len; //固定长度4,负载长度len
    
    crc = modbus_crc16((unsigned char *)pmsg_hdr, (unsigned int)(pmsg_hdr->len + 16 - 2));
    *(uint16_t *)(send_msg_buf + (pmsg_hdr->len + 16 - 2)) = crc;
    
    *(uint8_t *)(send_msg_buf + (pmsg_hdr->len + 16 - 2 + 2)) = 0x16; //包尾
    
    if(g_plora_msg_retran->retran_flag != FALSE)
    {
        printf("g_retran_flag is invalid\n");
        return -1;
    }
    g_plora_msg_retran->retran_flag = TRUE;
    g_plora_msg_retran->retran_buf_len = (pmsg_hdr->len + 16 - 2 + 2 + 1);
    g_plora_msg_retran->retran_time = abs(time(NULL));
    memcpy(g_plora_msg_retran->retran_buf, send_msg_buf, g_plora_msg_retran->retran_buf_len);
    
    status = lora_transmit_frame(send_msg_buf, (pmsg_hdr->len + 16 - 2 + 2 + 1));
    
    return status;
}
uint8_t send_lora_broadcast_time(lora_addr_t *pdest_addr, uint8_t *ppayload, uint8_t len)
{
    uint8_t status; 
    char send_msg_buf[256];
    lora_msg_hdr_t *pmsg_hdr = (lora_msg_hdr_t *)send_msg_buf;
    uint16_t crc;

    pmsg_hdr->sync_code = 0xeeff;
    pmsg_hdr->node_type = 1;
    memcpy(pmsg_hdr->dest_addr.addr, pdest_addr->addr, sizeof(lora_addr_t));
    memcpy(pmsg_hdr->src_addr.addr, ParaSet.Devid, sizeof(lora_addr_t));
    
    pmsg_hdr->sn = g_sn++;
    pmsg_hdr->cmd = LORA_MSG_TYPE_BROADCAST_TIME;
    
    memcpy((uint8_t *)(send_msg_buf + sizeof(lora_msg_hdr_t)), ppayload, len);
    pmsg_hdr->len = 4 + len; //固定长度4,负载长度len
    
    crc = modbus_crc16((unsigned char *)pmsg_hdr, (unsigned int)(pmsg_hdr->len + 16 - 2));
    memcpy((send_msg_buf + (pmsg_hdr->len + 16 - 2)),  &crc, sizeof(uint16_t));
    *(uint8_t *)(send_msg_buf + (pmsg_hdr->len + 16 - 2 + 2)) = 0x16; //包尾

    if(g_plora_msg_retran->retran_flag != FALSE)
    {
        printf("g_retran_flag is invalid\n");
        return -1;
    }
    g_plora_msg_retran->retran_flag = TRUE;
    g_plora_msg_retran->retran_buf_len = (pmsg_hdr->len + 16 - 2 + 2 + 1);
    g_plora_msg_retran->retran_time = abs(time(NULL));
    memcpy(g_plora_msg_retran->retran_buf, send_msg_buf, g_plora_msg_retran->retran_buf_len);

    status = lora_transmit_frame(send_msg_buf, (pmsg_hdr->len + 16 - 2 + 2 + 1));
    
    return status;
}
uint8_t send_lora_req_data(lora_addr_t *pdest_addr)
{
    uint8_t status; 
    char send_msg_buf[256];
    lora_msg_hdr_t *pmsg_hdr = (lora_msg_hdr_t *)send_msg_buf;
    uint16_t crc;
    //printf("func : %s\n",__FUNCTION__);
    pmsg_hdr->sync_code = 0xeeff;
    pmsg_hdr->node_type = 1;
    memcpy(pmsg_hdr->dest_addr.addr, pdest_addr->addr, sizeof(lora_addr_t));
    memcpy(pmsg_hdr->src_addr.addr, ParaSet.Devid, sizeof(lora_addr_t));
    
    pmsg_hdr->sn = g_sn++;
    pmsg_hdr->cmd = LORA_MSG_TYPE_REQ_DATA;
    
    pmsg_hdr->len = 4 + 0; //固定长度4,负载长度0
    
    crc = modbus_crc16((unsigned char *)pmsg_hdr, (unsigned int)(pmsg_hdr->len + 16 - 2));
    *(uint16_t *)(send_msg_buf + (pmsg_hdr->len + 16 - 2)) = crc;
    
    *(uint8_t *)(send_msg_buf + (pmsg_hdr->len + 16 - 2 + 2)) = 0x16; //包尾
    
    if(g_plora_msg_retran->retran_flag != FALSE)
    {
        printf("g_retran_flag is invalid\n");
        return -1;
    }
    g_plora_msg_retran->retran_flag = TRUE;
    g_plora_msg_retran->retran_buf_len = (pmsg_hdr->len + 16 - 2 + 2 + 1);
    g_plora_msg_retran->retran_time = abs(time(NULL));
    memcpy(g_plora_msg_retran->retran_buf, send_msg_buf, g_plora_msg_retran->retran_buf_len);

    
    status = lora_transmit_frame(send_msg_buf, (pmsg_hdr->len + 16 - 2 + 2 + 1));
    
    return status;
}

uint8_t get_retran_flag(void)
{
    return g_plora_msg_retran->retran_flag;    
}
uint8_t set_retran_flag(uint8_t flag)
{
	g_plora_msg_retran->retran_flag = flag;
    return 0;    
}

time_t get_tran_time(void)
{
    return g_plora_msg_retran->retran_time;    
}

uint8_t init_retran_cb(void)
{
    memset(g_plora_msg_retran, 0, sizeof(lora_msg_retran_hdr_t));
    return 0;
}

uint8_t retran_lora_msg(void)
{
    uint8_t status; 

    if(g_plora_msg_retran->retran_times >= LORA_MSG_MAX_RETRAN_TIMES)
    {
        memset(g_plora_msg_retran, 0, sizeof(lora_msg_retran_hdr_t));
        return 0;
    }
    if((abs(time(NULL)) - g_plora_msg_retran->retran_time) < LORA_MSG_RETRAN_INTERVAL)
    {
        return 0;
    }
    
    g_plora_msg_retran->retran_time = abs(time(NULL));
    g_plora_msg_retran->retran_times++;
    status = lora_transmit_frame(g_plora_msg_retran->retran_buf, g_plora_msg_retran->retran_buf_len);
    return status;
}
int handle_lora_rx_msg( void *buffer, uint16_t size )
{
    lora_msg_hdr_t *pmsg_hdr;
    
    printf("handle_lora_rx_msg\n");

    pmsg_hdr = (lora_msg_hdr_t *)((char *)buffer); 
    if(size < sizeof(lora_msg_hdr_t))
    {
        printf("msg size if invalid\n");
        return -1;
    }
    
    if(size < (pmsg_hdr->len + 17)) 
    {
        printf("msg size if invalid\n");
        return -1;
    }
    
    if(!check_input_msg(pmsg_hdr))
    {
        printf("check_input_msg failed\n");
        return -1;
    }

    switch(pmsg_hdr->cmd)
    {
        case LORA_MSG_TYPE_ACK:
            
        case LORA_MSG_TYPE_BINDING_ACK:

        case LORA_MSG_TYPE_REQ_PARA_ACK:

        case LORA_MSG_TYPE_SET_PARA_ACK:

        case LORA_MSG_TYPE_SET_STATE_ACK:

        default:
            break;

    }

}
