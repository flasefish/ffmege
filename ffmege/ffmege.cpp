// ffmpegtest.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"


#include "stdafx.h"
#include <stdio.h>
#include "dump_info.h"
#include "log.h"
#include "decode_video.h"
#include "czyplayer.h"

#define __STDC_CONSTANT_MACROS
#define SDL_MAIN_HANDLED

extern "C"//��������˻���һ�Ѵ��󣬷������ǹ���ȥ
{

#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

void Ffmpeglog(int l, char* t) {
	if (l <= AV_LOG_INFO)
		fprintf(stdout, "%s\n", t);
}


int readMp3Info() {
	AVFormatContext *fmt_ctx = NULL;
	AVDictionaryEntry *tag = NULL;
	int ret;
	//ɾ�����жϷ�֧ IF��Ҫ��Ȼ������ȥ���������ˣ�����
	av_register_all();//ע�����е����ʲô��  ��װffmpeg��һ�䶼�������
	if ((ret = avformat_open_input(&fmt_ctx, "On a Slow Boat to China.mp3", NULL, NULL)))
		//�ϰ汾�Ļ�avformat_open_input��&fmt_ctx,file,Null,0,Null�����β�һ������Ҫ���ϰ汾���ӵ�
		//ע���ļ����õ�λ�ã���Ҫ����debug�ļ��У������ڹ�����
		//�Ŵ�ط�����avformat_open_inputû�򿪵Ļ�������ֵ-2  �ϵ�֮���ܿ���ret=-2����
		//��ȷ�Ļ� ret=0
		return ret;
	while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
		printf("%s=%s\n", tag->key, tag->value);
	avformat_close_input(&fmt_ctx);
	return 0;
}


#define INBUF_SIZE (10<<20)

int main()//ɾ��������
{
	//readMp3Info();
	/*char *filename = "F:\\video.MP4";
	char *outfilename;
	FILE *f;
	uint8_t* data;
	size_t   data_size;

	set_log_callback();

	f = fopen(filename, "rb");
	if (!f) {
		fprintf(stderr, "Could not open %s\n", filename);
		exit(1);
	}

	data = (uint8_t*)malloc(INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE);
	// set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) 
	memset(data + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
	// read raw data from the input file 
	data_size = fread(data, 1, INBUF_SIZE, f); // read 1 byte every time, ret = how many time, INBUF_SIZE = max time
	if (!data_size)
		exit(1);

	dump_info(data, data_size);

	free(data);
	fclose(f);
	*/
	//decode_main("F:\\video.MP4", "F:\\video2.MP4");

	play_main();
}
