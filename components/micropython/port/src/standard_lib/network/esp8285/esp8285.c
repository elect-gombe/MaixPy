/**
 * @file esp8285.c
 * 
 * @par Copyright:
 * Copyright (c) 2015 ITEAD Intelligent Systems Co., Ltd. \n\n
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version. \n\n
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "py/stream.h"
#include "py/runtime.h"
#include "py/misc.h"
#include "py/mphal.h"
#include "py/objstr.h"
#include "extmod/misc.h"
#include "lib/netutils/netutils.h"

#include "utils.h"
#include "modmachine.h"
#include "mphalport.h"
#include "mpconfigboard.h"
#include "machine_uart.h"
#include "modnetwork.h"
#include "plic.h"
#include "sysctl.h"
#include "atomic.h"
#include "uart.h"
#include "uarths.h"
#include "esp8285.h"
#include "sleep.h"


STATIC void kmp_get_next(const char* targe, int next[])
{  
    int targe_Len = strlen(targe);  
    next[0] = -1;  
    int k = -1;  
    int j = 0;  
    while (j < targe_Len - 1)  
    {     
        if (k == -1 || targe[j] == targe[k])  
        {  
            ++j;  
            ++k;   
            if (targe[j] != targe[k])  
                next[j] = k;    
            else   
                next[j] = next[k];  
        }  
        else  
        {  
            k = next[k];  
        }  
    }  
}  
STATIC int kmp_match(char* src,int src_len, const char* targe, int* next)  
{  
    int i = 0;  
    int j = 0;  
    int sLen = src_len;  
    int pLen = strlen(targe);  
    while (i < sLen && j < pLen)  
    {     
        if (j == -1 || src[i] == targe[j])  
        {  
            i++;  
            j++;  
        }  
        else  
        {       
            j = next[j];  
        }  
    }  
    if (j == pLen)  
        return i - j;  
    else  
        return -1;  
} 
STATIC uint32_t kmp_find(char* src,uint32_t src_len, const char* tagert)
{
	uint32_t index = 0;
	uint32_t tag_len = strlen(tagert);
	int* next = (int*)malloc(sizeof(uint32_t) * tag_len);
	kmp_get_next(tagert,next);
	index = kmp_match(src,src_len,tagert, next);
	free(next);
	return index;
}
STATIC uint32_t data_find(char* src,uint32_t src_len, const char* tagert)
{
	return kmp_find(src,src_len,tagert);
}

bool kick(esp8285_obj* nic)
{
    return eAT(nic);
}

bool reset(esp8285_obj* nic)
{
    unsigned long start;
    if (eATRST(nic)) {
        msleep(2000);
        start = mp_hal_ticks_ms();
        while (mp_hal_ticks_ms() - start < 3000) {
            if (eAT(nic)) {
                msleep(1500); /* Waiting for stable */
                return true;
            }
            msleep(100);
        }
    }
    return false;
}

char* getVersion(esp8285_obj* nic)
{
    char* version;
    eATGMR(nic,&version);
    return version;
}

bool setOprToStation(esp8285_obj* nic)
{
    char mode;
    if (!qATCWMODE(nic,&mode)) {
        return false;
    }
    if (mode == 1) {
        return true;
    } else {
        if (sATCWMODE(nic,1)) {
            return true;
        } else {
            return false;
        }
    }
}


bool joinAP(esp8285_obj* nic, const char* ssid, const char* pwd)
{
    return sATCWJAP(nic,ssid, pwd);
}

bool enableClientDHCP(esp8285_obj* nic,char mode, bool enabled)
{
    return sATCWDHCP(nic,mode, enabled);
}

bool leaveAP(esp8285_obj* nic)
{
    return eATCWQAP(nic);
}

char* getIPStatus(esp8285_obj* nic)
{
    char* list;
    eATCIPSTATUS(nic, &list);
    return list;
}

char* getLocalIP(esp8285_obj* nic)
{
    char* list;
    eATCIFSR(nic, &list);
    return list;
}

bool enableMUX(esp8285_obj* nic)
{
    return sATCIPMUX(nic,1);
}

bool disableMUX(esp8285_obj* nic)
{
    return sATCIPMUX(nic,0);
}

bool createTCP(esp8285_obj* nic,char* addr, uint32_t port)
{
    return sATCIPSTARTSingle(nic,"TCP", addr, port);
}

bool releaseTCP(esp8285_obj* nic)
{
    return eATCIPCLOSESingle(nic);
}

bool registerUDP(esp8285_obj* nic,char* addr, uint32_t port)
{
    return sATCIPSTARTSingle(nic,"UDP", addr, port);
}

bool unregisterUDP(esp8285_obj* nic)
{
    return eATCIPCLOSESingle(nic);
}

bool createTCP_mul(esp8285_obj* nic,char mux_id, char* addr, uint32_t port)
{
    return sATCIPSTARTMultiple(nic,mux_id, "TCP", addr, port);
}

bool releaseTCP_mul(esp8285_obj* nic,char mux_id)
{
    return sATCIPCLOSEMulitple(nic,mux_id);
}

bool registerUDP_mul(esp8285_obj* nic,char mux_id, char* addr, uint32_t port)
{
    return sATCIPSTARTMultiple(nic,mux_id, "UDP", addr, port);
}

bool unregisterUDP_mul(esp8285_obj* nic,char mux_id)
{
    return sATCIPCLOSEMulitple(nic,mux_id);
}

bool setTCPServerTimeout(esp8285_obj* nic,uint32_t timeout)
{
    return sATCIPSTO(nic,timeout);
}

bool startTCPServer(esp8285_obj* nic,uint32_t port)
{
    if (sATCIPSERVER(nic,1, port)) {
        return true;
    }
    return false;
}

bool stopTCPServer(esp8285_obj* nic)
{
    sATCIPSERVER(nic,0,0);
    reset(nic);
    return false;
}

bool startServer(esp8285_obj* nic,uint32_t port)
{
    return startTCPServer(nic,port);
}

bool stopServer(esp8285_obj* nic)
{
    return stopTCPServer(nic);
}

bool get_host_byname(esp8285_obj* nic, const char* host,uint32_t len,char* out_ip, uint32_t timeout_ms)
{
	int index = 0;
	if(false == sATCIPDOMAIN(nic,host, timeout_ms))
	{
		mp_printf(&mp_plat_print, "[MaixPy] %s | get_host_byname failed\n",__func__);
		return false;
	}
	char IP_buf[16]={0};
	index = data_find(nic->buffer,ESP8285_BUF_SIZE,"+CIPDOMAIN:");
	sscanf(nic->buffer + index,"+CIPDOMAIN:%s",IP_buf);
	mp_obj_t IP = mp_obj_new_str(IP_buf, strlen(IP_buf));
	nlr_buf_t nlr;
	if (nlr_push(&nlr) == 0)
	{
		netutils_parse_ipv4_addr(IP, (uint8_t*)out_ip,NETUTILS_BIG);
		nlr_pop();
	}
	return true;
}

bool get_ipconfig(esp8285_obj* nic, ipconfig_obj* ipconfig)
{

	if(0 == qATCIPSTA_CUR(nic))
		return false;
	char* cur = NULL;
	cur = strstr(nic->buffer, "ip");
	if(cur == NULL)
	{
		mp_printf(&mp_plat_print, "[MaixPy] %s | esp8285_ipconfig could'n get ip\n",__func__);
		return false;
	}
	char ip_buf[16] = {0};
	sscanf(cur,"ip:\"%[^\"]\"",ip_buf);
	ipconfig->ip = mp_obj_new_str(ip_buf,strlen(ip_buf));
	cur = strstr(nic->buffer, "gateway");
	if(cur == NULL)
	{
		mp_printf(&mp_plat_print, "[MaixPy] %s | esp8285_ipconfig could'n get gateway\n",__func__);
		return false;
	}
	char gateway_buf[16] = {0};
	sscanf(cur,"gateway:\"%[^\"]\"",gateway_buf);
	ipconfig->gateway = mp_obj_new_str(gateway_buf,strlen(gateway_buf));
	cur = strstr(nic->buffer, "netmask");
	if(cur == NULL)
	{
		mp_printf(&mp_plat_print, "[MaixPy] %s | esp8285_ipconfig could'n get netmask\n",__func__);
		return false;
	}
	char netmask_buf[16] = {0};
	sscanf(cur,"netmask:\"%[^\"]\"",netmask_buf);
	ipconfig->netmask = mp_obj_new_str(netmask_buf,strlen(netmask_buf));
	//ssid & mac
	if(false == qATCWJAP_CUR(nic))
	{
		return false;
	}
	char ssid[50] = {0};
	char MAC[17] = {0};
	cur = strstr(nic->buffer, "+CWJAP_CUR:");
	sscanf(cur, "+CWJAP_CUR:\"%[^\"]\",\"%[^\"]\"", ssid, MAC);
	ipconfig->ssid = mp_obj_new_str(ssid,strlen(ssid));
	ipconfig->MAC = mp_obj_new_str(MAC,strlen(MAC));
	return true;
}


bool esp_send(esp8285_obj* nic,const char* buffer, uint32_t len, uint32_t timeout)
{
    bool ret = false;
    uint32_t send_total_len = 0;
    uint16_t send_len = 0;

    while(send_total_len < len)
    {
        send_len = ((len-send_total_len) > ESP8285_MAX_ONCE_SEND)?ESP8285_MAX_ONCE_SEND : (len-send_total_len);
        ret = sATCIPSENDSingle(nic,buffer+send_total_len, send_len, timeout);
        if(!ret)
            return false;
        send_total_len += send_len;
    }
    return true;
}

bool esp_send_mul(esp8285_obj* nic,char mux_id, const char* buffer, uint32_t len)
{
    return sATCIPSENDMultiple(nic,mux_id, buffer, len);
}

uint32_t esp_recv(esp8285_obj* nic,char* buffer, uint32_t buffer_size, uint32_t timeout)
{
    return recvPkg(nic,buffer, buffer_size, NULL, timeout, NULL);
}

uint32_t esp_recv_mul(esp8285_obj* nic,char mux_id, char* buffer, uint32_t buffer_size, uint32_t timeout)
{
    char id;
    uint32_t ret;
    ret = recvPkg(nic,buffer, buffer_size, NULL, timeout, &id);
    if (ret > 0 && id == mux_id) {
        return ret;
    }
    return 0;
}

uint32_t esp_recv_mul_id(esp8285_obj* nic,char* coming_mux_id, char* buffer, uint32_t buffer_size, uint32_t timeout)
{
    return recvPkg(nic,buffer, buffer_size, NULL, timeout, coming_mux_id);
}

/*----------------------------------------------------------------------------*/
/* +IPD,<id>,<len>:<data> */
/* +IPD,<len>:<data> */
uint32_t recvPkg(esp8285_obj*nic,char* buffer, uint32_t buffer_size, uint32_t *data_len, uint32_t timeout, char* coming_mux_id)
{
	int errcode;
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);

	
    int index_PIPDcomma = -1;
    int index_colon = -1; /* : */
    int index_comma = -1; /* , */
    int len = -1;
    int id = -1;
    bool has_data = false;
    uint32_t ret;
    unsigned long start;
    uint32_t i;
    
    if (buffer == NULL) {
        return -1;
    }
    uint32_t iter = 0;
	memset(nic->buffer, 0, ESP8285_BUF_SIZE);
    start = mp_hal_ticks_ms();
    while ((mp_hal_ticks_ms() - start < timeout) ) {
        if(uart_rx_any(nic->uart_obj) > 0) {
			uart_stream->read(nic->uart_obj,&nic->buffer[iter++],1,&errcode);
        }
        index_PIPDcomma = data_find(nic->buffer,iter,"+IPD,");
        if (index_PIPDcomma != -1) {
            index_colon = data_find(nic->buffer+index_PIPDcomma + 5 , ESP8285_BUF_SIZE - 5 ,":");
            if (index_colon != -1) {
                index_comma = data_find(nic->buffer+index_PIPDcomma + 5 , ESP8285_BUF_SIZE - 5 ,",");
                /* +IPD,id,len:data */
                if (index_comma != -1 && index_comma < index_colon) {
					sscanf(&nic->buffer[index_PIPDcomma],"+IPD,%d,%d:",&id,&len);
                    if (id < 0 || id > 4) {
                        return -1;
                    }
                    if (len <= 0) {
                        return -1;
                    }
                } else { /* +IPD,len:data */
                	sscanf(&nic->buffer[index_PIPDcomma],"+IPD,%d:",&len);
                    if (len <= 0) {
                        return -1;
                    }
                }
                has_data = true;
                break;
            }
        }
    }
    if (has_data) {
        i = 0;
        ret = len > buffer_size ? buffer_size : len;
        start = mp_hal_ticks_ms();
        while (mp_hal_ticks_ms() - start < 3000) {
            while(uart_rx_any(nic->uart_obj) > 0 && i < ret) {
				uart_stream->read(nic->uart_obj,&buffer[i++],1,&errcode); 
            }
            if (i == ret) {
                // rx_empty(nic);
                if (data_len) {
                    *data_len = len;    
                }
                if (index_comma != -1 && coming_mux_id) {
                    *coming_mux_id = (char)id;
                }
                return ret;
            }
        }
    }
    return 0;
}

void rx_empty(esp8285_obj* nic) 
{
	int errcode;
	char data = 0;
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
    while(uart_rx_any(nic->uart_obj) > 0) {
        uart_stream->read(nic->uart_obj,&data,1,&errcode); 
    }
}

char* recvString_1(esp8285_obj* nic, const char* target1,uint32_t timeout)
{
	int errcode;
	uint32_t iter = 0;
	memset(nic->buffer,0,ESP8285_BUF_SIZE);
    unsigned long start = mp_hal_ticks_ms();
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
    while (mp_hal_ticks_ms() - start < timeout) {
        while(uart_rx_any(nic->uart_obj) > 0) {
            uart_stream->read(nic->uart_obj,&nic->buffer[iter++],1,&errcode);
        }
        if (data_find(nic->buffer,iter,target1) != -1) {
            return nic->buffer;
        } 
    }
    return NULL;
}


char* recvString_2(esp8285_obj* nic,char* target1, char* target2, uint32_t timeout, int8_t* find_index)
{
	int errcode;
	uint32_t iter = 0;
    *find_index = -1;
	memset(nic->buffer,0,ESP8285_BUF_SIZE);
    unsigned int start = mp_hal_ticks_ms();
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
    while (mp_hal_ticks_ms() - start < timeout) {
        while(uart_rx_any(nic->uart_obj) > 1 && iter < ESP8285_BUF_SIZE) {
            uart_stream->read(nic->uart_obj,&nic->buffer[iter++],1,&errcode);
        }
        if (data_find(nic->buffer,iter,target1) != -1) {
            *find_index = 0;
            return nic->buffer;
        } else if (data_find(nic->buffer,iter,target2) != -1) {
            *find_index = 1;
            return nic->buffer;
        }
    }
    return NULL;
}

char* recvString_3(esp8285_obj* nic,char* target1, char* target2,char* target3,uint32_t timeout, int8_t* find_index)
{

	int errcode;
	uint32_t iter = 0;
    *find_index = -1;
	memset(nic->buffer,0,ESP8285_BUF_SIZE);
    unsigned long start = mp_hal_ticks_ms();
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
    while (mp_hal_ticks_ms() - start < timeout) {
        while(uart_rx_any(nic->uart_obj) > 0) {
            uart_stream->read(nic->uart_obj,&nic->buffer[iter++],1,&errcode);
        }
        if (data_find(nic->buffer,iter,target1) != -1) {
            *find_index = 0;
            return nic->buffer;
        } else if (data_find(nic->buffer,iter,target2) != -1) {
            *find_index = 1;
            return nic->buffer;
        } else if (data_find(nic->buffer,iter,target3) != -1) {
            *find_index = 2;
            return nic->buffer;
        }
    }
    return NULL;
}

bool recvFind(esp8285_obj* nic, const char* target, uint32_t timeout)
{
    recvString_1(nic, target, timeout);
    if (data_find(nic->buffer,ESP8285_BUF_SIZE,target) != -1) {
        return true;
    }
    return false;
}

bool recvFindAndFilter(esp8285_obj* nic,const char* target, const char* begin, const char* end, char** data, uint32_t timeout)
{
    recvString_1(nic,target, timeout);
    if (data_find(nic->buffer,ESP8285_BUF_SIZE,target) != -1) {
        int32_t index1 = data_find(nic->buffer,ESP8285_BUF_SIZE,begin);
        int32_t index2 = data_find(nic->buffer,ESP8285_BUF_SIZE,end);
        if (index1 != -1 && index2 != -1) {
            index1 += strlen(begin);
			*data = m_new(char, index2 - index1);
			memcpy(*data,nic->buffer+index1, index2 - index1);
            return true;
        }
    }
    return false;
}

bool eAT(esp8285_obj* nic)
{	
	int errcode = 0;
	const char* cmd = "AT\r\n";
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
    rx_empty(nic);// clear rx
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
    return recvFind(nic,"OK",1000);
}

bool eATE(esp8285_obj* nic,bool enable)
{	
	int errcode = 0;
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
    rx_empty(nic);// clear rx
    if(enable)
    {
    	const char* cmd = "ATE0\r\n";
		uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
    	return recvFind(nic,"OK",1000);
    }
	else
	{
    	const char* cmd = "ATE1\r\n";
		uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
    	return recvFind(nic,"OK",1000);		
	}
}


bool eATRST(esp8285_obj* nic) 
{
	int errcode = 0;
	const char* cmd = "AT+RST\r\n";
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
    rx_empty(nic);// clear rx
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
    return recvFind(nic,"OK",1000);
}

bool eATGMR(esp8285_obj* nic,char** version)
{

	int errcode = 0;
	const char* cmd = "AT+GMR\r\n";
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
    rx_empty(nic);// clear rx
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);

    return recvFindAndFilter(nic,"OK", "\r\r\n", "\r\n\r\nOK", version, 5000); 
}

bool qATCWMODE(esp8285_obj* nic,char* mode) 
{
	int errcode = 0;
	const char* cmd = "AT+CWMODE?\r\n";
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
    char* str_mode;
    bool ret;
    if (!mode) {
        return false;
    }
    rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
    ret = recvFindAndFilter(nic,"OK", "+CWMODE:", "\r\n\r\nOK", &str_mode,1000); 
    if (ret) {
        *mode = atoi(str_mode);
        return true;
    } else {
        return false;
    }
}

bool sATCWMODE(esp8285_obj* nic,char mode)
{
	int errcode = 0;
	const char* cmd = "AT+CWMODE=";
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);	
	char mode_str[10] = {0};
    int8_t find;
	itoa(mode, mode_str, 10);
    rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,mode_str,strlen(mode_str),&errcode);
	uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);    
    if(recvString_2(nic,"OK", "no change",1000, &find) != NULL)
        return true;
    return false;
}

bool sATCWJAP(esp8285_obj* nic, const char* ssid, const char* pwd)
{
	int errcode = 0;
	const char* cmd = "AT+CWJAP=\"";
    int8_t find;
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);	
    rx_empty(nic);	
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,ssid,strlen(ssid),&errcode);
	uart_stream->write(nic->uart_obj,"\",\"",strlen("\",\""),&errcode);
	uart_stream->write(nic->uart_obj,pwd,strlen(pwd),&errcode);
	uart_stream->write(nic->uart_obj,"\"",strlen("\""),&errcode);
	uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
    if(recvString_2(nic,"OK", "FAIL", 10000, &find) != NULL && find==0 )
        return true;
    return false;
}

bool sATCWDHCP(esp8285_obj* nic,char mode, bool enabled)
{
	int errcode = 0;
	const char* cmd = "AT+CWDHCP=";
    int8_t find;
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);	
	char strEn[2] = {0};
	if (enabled) {
		strcpy(strEn, "1");
	}
	else
	{
		strcpy(strEn, "0");
	}
    rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,strEn,strlen(strEn),&errcode);
	uart_stream->write(nic->uart_obj,",",strlen(","),&errcode);
	uart_stream->write(nic->uart_obj, &mode,1,&errcode);
	uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
    if( recvString_2(nic,"OK", "FAIL", 10000, &find) != NULL && find==0)
        return true;    
    return false;
}


bool eATCWQAP(esp8285_obj* nic)
{
	int errcode = 0;
	const char* cmd = "AT+CWQAP\r\n";
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);	
    rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
    return recvFind(nic,"OK",1000);
}

bool eATCIPSTATUS(esp8285_obj* nic,char** list)
{
    int errcode = 0;
	const char* cmd = "AT+CIPSTATUS\r\n";
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);	
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
    msleep(100);
    rx_empty(nic);
    uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
    return recvFindAndFilter(nic,"OK", "\r\r\n", "\r\n\r\nOK", list,1000);
}
bool sATCIPSTARTSingle(esp8285_obj* nic,const char* type, char* addr, uint32_t port)
{
    int errcode = 0;
	const char* cmd = "AT+CIPSTART=\"";
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
	mp_obj_t IP = netutils_format_ipv4_addr((uint8_t*)addr,NETUTILS_BIG);
	const char* host = mp_obj_str_get_str(IP);
	char port_str[10] = {0};
    int8_t find_index;
	itoa(port, port_str, 10);
	rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,type,strlen(type),&errcode);
	uart_stream->write(nic->uart_obj,"\",\"",strlen("\",\""),&errcode);
	uart_stream->write(nic->uart_obj,host,strlen(host),&errcode);
	uart_stream->write(nic->uart_obj,"\",",strlen("\","),&errcode);
	uart_stream->write(nic->uart_obj,port_str,strlen(port_str),&errcode);
	uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
    if(recvString_3(nic,"OK", "ERROR", "ALREADY CONNECT", 10000, &find_index)!=NULL && (find_index==0 || find_index==2) )
        return true;
    return false;
}
bool sATCIPSTARTMultiple(esp8285_obj*nic,char mux_id, char* type, char* addr, uint32_t port)
{
    int errcode = 0;
	const char* cmd = "AT+CIPSTART=";
	char port_str[10] = {0};
    int8_t find_index;
	itoa(port,port_str ,10);
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
	rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj, &mux_id,1,&errcode);
	uart_stream->write(nic->uart_obj,",\"",strlen(",\""),&errcode);
	uart_stream->write(nic->uart_obj,type,strlen(type),&errcode);
	uart_stream->write(nic->uart_obj,"\",\"",strlen("\",\""),&errcode);
	uart_stream->write(nic->uart_obj,addr,strlen(addr),&errcode);
	uart_stream->write(nic->uart_obj,"\",",strlen("\","),&errcode);
	uart_stream->write(nic->uart_obj,port_str,strlen(port_str),&errcode);
	uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
    if(recvString_3(nic,"OK", "ERROR", "ALREADY CONNECT", 10000, &find_index) != NULL  && (find_index==0 || find_index==2) )
        return true;
    return false;
}
bool sATCIPSENDSingle(esp8285_obj*nic,const char* buffer, uint32_t len, uint32_t timeout)
{
	int errcode = 0;
	const char* cmd = "AT+CIPSEND=";
	char len_str[10] = {0};
	itoa(len,len_str ,10);
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
	rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,len_str,strlen(len_str),&errcode);
	uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
    if (recvFind(nic,">", 5000)) {
        rx_empty(nic);
		uart_stream->write(nic->uart_obj,buffer,len,&errcode);
        return recvFind(nic,"SEND OK", timeout);
    }
    return false;
}
bool sATCIPSENDMultiple(esp8285_obj* nic,char mux_id, const char* buffer, uint32_t len)
{
   	int errcode = 0;
	const char* cmd = "AT+CIPSEND=";
	char len_str[10] = {0};
	itoa(len,len_str ,10);
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
	rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj, &mux_id,1,&errcode);
	uart_stream->write(nic->uart_obj,",",strlen(","),&errcode);
	uart_stream->write(nic->uart_obj,len_str,strlen(len_str),&errcode);
    if (recvFind(nic,">", 5000)) {
        rx_empty(nic);
		uart_stream->write(nic->uart_obj,buffer,len,&errcode);
		uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
        return recvFind(nic,"SEND OK", 10000);
    }
    return false;
}
bool sATCIPCLOSEMulitple(esp8285_obj* nic,char mux_id)
{
	int errcode = 0;
	const char* cmd = "AT+CIPCLOSE=";
    int8_t find;
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);	
	rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,(const char*)&mux_id,1,&errcode);
	uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
    if(recvString_2(nic,"OK", "link is not", 5000, &find) != NULL)
        return true;
    return false;
}
bool eATCIPCLOSESingle(esp8285_obj* nic)
{

	int errcode = 0;
	const char* cmd = "AT+CIPCLOSE\r\n";
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);	
    rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
    return recvFind(nic,"OK", 5000);
}
bool eATCIFSR(esp8285_obj* nic,char** list)
{
	int errcode = 0;
	const char* cmd = "AT+CIFSR\r\n";
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
    rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
    return recvFindAndFilter(nic,"OK", "\r\r\n", "\r\n\r\nOK", list,5000);
}
bool sATCIPMUX(esp8285_obj* nic,char mode)
{
	int errcode = 0;
	const char* cmd = "AT+CIPMUX=";
	char mode_str[10] = {0};
    int8_t find;
	itoa(mode, mode_str, 10);
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);	
    rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,mode_str,strlen(mode_str),&errcode);
	uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
    if(recvString_2(nic,"OK", "Link is builded",5000, &find) != NULL && find==0)
        return true;
    return false;
}
bool sATCIPSERVER(esp8285_obj* nic,char mode, uint32_t port)
{
	int errcode = 0;
    int8_t find;
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);	
    if (mode) {
		const char* cmd = "AT+CIPSERVER=1,";
		char port_str[10] = {0};
		itoa(port, port_str, 10);
        rx_empty(nic);
		uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
		uart_stream->write(nic->uart_obj,port_str,strlen(port_str),&errcode);
		uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
        if(recvString_2(nic,"OK", "no change",1000, &find) != NULL)
            return true;
        return false;
    } else {
        rx_empty(nic);
		const char* cmd = "AT+CIPSERVER=0";
		uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
		uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
        return recvFind(nic,"\r\r\n",1000);
    }
}
bool sATCIPSTO(esp8285_obj* nic,uint32_t timeout)
{

	int errcode = 0;
	const char* cmd = "AT+CIPSTO=";
	char timeout_str[10] = {0};
	itoa(timeout, timeout_str, 10);
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);	
	rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,timeout_str,strlen(timeout_str),&errcode);
    uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
    return recvFind(nic,"OK",1000);
}

bool sATCIPMODE(esp8285_obj* nic,char mode)
{

	int errcode = 0;
	const char* cmd = "AT+CIPMODE=";
	char mode_str[10] = {0};
	itoa(mode, mode_str, 10);
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);	
	rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,mode_str,strlen(mode_str),&errcode);
    uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
    return recvFind(nic,"OK",1000);
}

bool sATCIPDOMAIN(esp8285_obj* nic, const char* domain_name, uint32_t timeout)
{
	int errcode = 0;
	const char* cmd = "AT+CIPDOMAIN=";
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
	rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,"\"",strlen("\""),&errcode);
	uart_stream->write(nic->uart_obj,domain_name,strlen(domain_name),&errcode);
	uart_stream->write(nic->uart_obj,"\"",strlen("\""),&errcode);
	uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);  
    return recvFind(nic,"OK",timeout);
}

bool qATCIPSTA_CUR(esp8285_obj* nic)
{
	int errcode = 0;
	const char* cmd = "AT+CIPSTA_CUR?";
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
	rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
	return recvFind(nic,"OK",1000);
}
bool sATCIPSTA_CUR(esp8285_obj* nic, const char* ip,char* gateway,char* netmask)
{
	int errcode = 0;
	const char* cmd = "AT+CIPSTA_CUR=";
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
	rx_empty(nic);
	if(NULL == ip)
	{
		return false;
	}
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,ip,strlen(ip),&errcode);
	if(NULL == gateway)
	{
		uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
		return recvFind(nic,"OK",1000);
	}
	uart_stream->write(nic->uart_obj,",",strlen(","),&errcode);
	uart_stream->write(nic->uart_obj,gateway,strlen(gateway),&errcode);
	if(NULL == netmask)
	{
		uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
		return recvFind(nic,"OK",1000);
	}
	uart_stream->write(nic->uart_obj,",",strlen(","),&errcode);
	uart_stream->write(nic->uart_obj,netmask,strlen(netmask),&errcode);
	uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
	return recvFind(nic,"OK",1000);
}

bool qATCWJAP_CUR(esp8285_obj* nic)
{
	int errcode = 0;
	const char* cmd = "AT+CWJAP_CUR?";
	const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
	rx_empty(nic);
	uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);
	return recvFind(nic,"OK",1000);
}

bool eINIT(esp8285_obj* nic)
{
	bool init_flag = 1;
	init_flag = init_flag && eAT(nic);
	init_flag = init_flag && eATE(nic,1);
	init_flag = init_flag && sATCIPMODE(nic,0);
	init_flag = init_flag && setOprToStation(nic);
	init_flag = init_flag && disableMUX(nic);
	init_flag = init_flag && leaveAP(nic);
	return init_flag;
}

bool eATCWLAP(esp8285_obj* nic)
{
    int errcode = 0;
    const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
    const char cmd[] = {"AT+CWLAP"};

    rx_empty(nic);
    uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);

    if (recvString_1(nic, "\r\n\r\nOK", 10000) != NULL)
        return true;
    
    return false;
}

bool eATCWLAP_Start(esp8285_obj* nic)
{
    int errcode = 0;
    const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
    const char cmd[] = {"AT+CWLAP"};

    rx_empty(nic);
    uart_stream->write(nic->uart_obj,cmd,strlen(cmd),&errcode);
	uart_stream->write(nic->uart_obj,"\r\n",strlen("\r\n"),&errcode);  
    return true;
}

bool eATCWLAP_Get(esp8285_obj* nic, bool* end)
{
    int8_t find;
    *end = false;
    if (recvString_2(nic, "\r\n+CWLAP:", "\r\n\r\nOK", 10000, &find) != NULL)
    {
        if( find == 1)
            *end = true;
        return true;
    }
    return false;
}


bool eATCWSAP(esp8285_obj* nic, const char* ssid, const char* key, int chl, int ecn)
{
    int errcode;
    const mp_stream_p_t * uart_stream = mp_get_stream(nic->uart_obj);
    char ap_cmd[128] = {0};

    if (sATCWMODE(nic, 3) == false)
    {
        return false;
    }

    rx_empty(nic);
    memset(nic->buffer,0,ESP8285_BUF_SIZE);

    sprintf(ap_cmd, "AT+CWSAP=\"%s\",\"%s\",%d,%d", ssid, key, chl, ecn);
    uart_stream->write(nic->uart_obj,ap_cmd, strlen(ap_cmd), &errcode);
    uart_stream->write(nic->uart_obj, "\r\n", strlen("\r\n"), &errcode);

    if (recvString_1(nic, "\r\nOK", 3000) == NULL)
    {
        return false;
    }

    return true;
}

//bool setOprToSoftAP(void)
//{
//    char mode;
//    if (!qATCWMODE(&mode)) {
//        return false;
//    }
//    if (mode == 2) {
//        return true;
//    } else {
//        if (sATCWMODE(2) && reset()) {
//            return true;
//        } else {
//            return false;
//        }
//    }
//}

//bool setOprToStationSoftAP(void)
//{
//    char mode;
//    if (!qATCWMODE(&mode)) {
//        return false;
//    }
//    if (mode == 3) {
//        return true;
//    } else {
//        if (sATCWMODE(3) && reset()) {
//            return true;
//        } else {
//            return false;
//        }
//    }
//}

//String getAPList(void)
//{
//    String list;
//    eATCWLAP(list);
//    return list;
//}


//bool setSoftAPParam(String ssid, String pwd, char chl, char ecn)
//{
//    return sATCWSAP(ssid, pwd, chl, ecn);
//}

//String getJoinedDeviceIP(void)
//{
//    String list;
//    eATCWLIF(list);
//    return list;
//}

//bool eATCWLAP(String &list)
//{
//    String data;
//    rx_empty();
//    m_puart->println("AT+CWLAP");
//    return recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", list, 10000);
//}


//bool sATCWSAP(String ssid, String pwd, char chl, char ecn)
//{
//    String data;
//    rx_empty();
//    m_puart->print("AT+CWSAP=\"");
//    m_puart->print(ssid);
//    m_puart->print("\",\"");
//    m_puart->print(pwd);
//    m_puart->print("\",");
//    m_puart->print(chl);
//    m_puart->print(",");
//    m_puart->println(ecn);
//    
//    data = recvString("OK", "ERROR", 5000);
//    if (data.indexOf("OK") != -1) {
//        return true;
//    }
//    return false;
//}

//bool eATCWLIF(String &list)
//{
//    String data;
//    rx_empty();
//    m_puart->println("AT+CWLIF");
//    return recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", list);
//}

