// ffmpegtest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


#include "stdafx.h"
#include <stdio.h>
#include "dump_info.h"
#include "log.h"

#define __STDC_CONSTANT_MACROS
#define SDL_MAIN_HANDLED

extern "C"//这个忘记了会有一堆错误，反正就是过不去
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
	//删掉了判断分支 IF（要不然就跳出去结束程序了！！）
	av_register_all();//注册所有的组件什么的  安装ffmpeg第一句都是这个！
	if ((ret = avformat_open_input(&fmt_ctx, "On a Slow Boat to China.mp3", NULL, NULL)))
		//老版本的话avformat_open_input（&fmt_ctx,file,Null,0,Null）传参不一样，不要被老版本给坑到
		//注意文件放置的位置，不要放在debug文件夹，而是在工程内
		//放错地方导致avformat_open_input没打开的话，返回值-2  断点之后能看到ret=-2……
		//正确的话 ret=0
		return ret;
	while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
		printf("%s=%s\n", tag->key, tag->value);
	avformat_close_input(&fmt_ctx);
	return 0;
}


#define INBUF_SIZE (10<<20)

int main()//删掉了输入
{
	//readMp3Info();
	char *filename = "F:\\video.MP4";
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
	/* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
	memset(data + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
	/* read raw data from the input file */
	data_size = fread(data, 1, INBUF_SIZE, f); // read 1 byte every time, ret = how many time, INBUF_SIZE = max time
	if (!data_size)
		exit(1);

	dump_info(data, data_size);

	free(data);
	fclose(f);
}
