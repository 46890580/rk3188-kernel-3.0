#ifndef _IT6811_MHL_HW_H
#define _IT6811_MHL_HW_H

int it6811_detect_device(void);
int it6811_sys_detect_hpd(void);
int it6811_sys_read_edid(int block, unsigned char *buff);
int it6811_sys_config_video(struct hdmi_video_para *vpara);
int it6811_sys_config_audio(struct hdmi_audio *audio);
void it6811_sys_enalbe_output(int enable);
int it6811_sys_insert(void);
int it6811_sys_remove(void);
BYTE hdmitxrd(BYTE regaddr);
void hdmitxwr(BYTE regaddr, BYTE data);
void hdmitxset(BYTE Reg,BYTE Mask,BYTE Value);
BYTE mhltxrd(BYTE regaddr);
void mhltxwr(BYTE regaddr, BYTE data);
void mhltxset(BYTE Reg,BYTE Mask,BYTE Value);
long int get_current_time_us(void);








#endif
