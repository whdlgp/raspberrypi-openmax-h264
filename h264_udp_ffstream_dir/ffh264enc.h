#ifndef FFH264ENC_H
#define FFH264ENC_H

/* init the instance (context) */
extern int ffh264_enc_open(int w, int h, int bit_rate, int fps);

/* get extradata(SPS/PPS) */
void ffh264_get_global_header(int* header_size, unsigned char* header_data);

/* encode one using the single tone */
extern int ffh264_enc_encode(unsigned char *pYUV, unsigned char **cbf);

/* close it */ 
extern int ffh264_enc_close( );

#endif


