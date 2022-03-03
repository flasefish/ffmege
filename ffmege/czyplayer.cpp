#include "czyplayer.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

static double r2d(AVRational r)
{
	return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

int play_main() {
	//1.注册组件
	av_register_all();
	char *path = "F:\\video.MP4";
	//封装格式上下文(存放视屏，或者流信息)
	AVFormatContext *ic = NULL;

	//2.打开输入视频文件(0 成功)
	int re = avformat_open_input(&ic, path, 0, 0); //lib avformat
	if (re != 0) {
		char buf[1024] = { 0 };             //存放错误信息  (文件不存在,格式不支持)
		av_strerror(re, buf, sizeof(buf));  // lib: avutil
		printf("open %s failed: %s\n", path, buf);
		return -1;
	}

	//将视频时间戳换算为毫秒
	int totalSec = ic->duration / AV_TIME_BASE;
	printf("file total Sec is %d : %d \n", totalSec / 60, totalSec % 60);

	//视频解码，需要找到对应的AVStream所在的pFormatCtx->streams的索引位置
	//nb_streams: 包含流的数量,视频流、音频流、字幕流
	int videoStream = 0;
	AVCodecContext *videoCtx = NULL;
	for (int i = 0; i < ic->nb_streams; i++)
	{
		AVCodecContext *enc = ic->streams[i]->codec;
		//3.根据类型判断是否是视频流
		if (enc->codec_type == AVMEDIA_TYPE_VIDEO)
		{

			videoCtx = enc; //根据索引拿到对应的流,根据流拿到解码器上下文
			videoStream = i;
			//4.获取解码器. 根据上下文拿到编解码id, 通过该id拿到解码器
			AVCodec *codec = avcodec_find_decoder(enc->codec_id);
			if (!codec)
			{
				printf("video code not find! \n");
			}

			//5.打开解码器
			int err = avcodec_open2(enc, codec, NULL);
			if (err != 0)
			{
				char buf[1024] = { 0 };
				av_strerror(err, buf, sizeof(buf));
				printf(buf);
				return -2;
			}
			printf("open codec success!\n");
		}
	}

	//像素数据(解码数据)
	AVFrame *yuv = av_frame_alloc();

	int outwidth = 640;
	int outheight = 480;
	SwsContext *cCtx = NULL;

	char *rgb = new char[outwidth * outheight * 4];

	for (;;)
	{
		AVPacket pkt;
		//6.一帧一帧读取压缩的视频数据AVPacket
		re = av_read_frame(ic, &pkt);
		if (re != 0)    break;
		if (pkt.stream_index != videoStream)
		{
			av_packet_unref(&pkt);
			continue;
		}
		int pts = pkt.pts * r2d(ic->streams[pkt.stream_index]->time_base) * 1000;

		//7.1ffmpeg新版本方法: 解码
		int re = avcodec_send_packet(videoCtx, &pkt);
		if (re != 0)
		{
			av_packet_unref(&pkt);
			continue;
		}
		//7.2
		re = avcodec_receive_frame(videoCtx, yuv);
		if (re != 0)
		{
			av_packet_unref(&pkt);
			continue;
		}

		printf("[decode success!] ");           //解码成功

		//解码出来的是yuv, 但是显示需要rgb    yuv->rgb              
		//解码出来的视频大小, 不一定是显示的大小, 窗口会放大、缩小(这个缩放虽然可以在客户端转换, 但消耗资源, 可以一举两得, 在这里就做缩放处理)

		//8.获取转码器           lib: swscale
		cCtx = sws_getCachedContext(cCtx, videoCtx->width,
			videoCtx->height,
			videoCtx->pix_fmt,
			outwidth,
			outheight,
			AV_PIX_FMT_BGRA,
			SWS_BICUBIC,            //转码用什么算法
			NULL, NULL, NULL
		);
		if (!cCtx)
		{
			printf("sws_gegCachedContext failed!\n");
			break;
		}

		uint8_t *data[AV_NUM_DATA_POINTERS] = { 0 };
		data[0] = (uint8_t *)rgb;
		int linesize[AV_NUM_DATA_POINTERS] = { 0 };
		linesize[0] = outwidth * 4;     //一行的大小, 宽 * 4
										//9.转码YUV->RGB8888 (h: 转码后的高度)
		int h = sws_scale(cCtx, yuv->data, yuv->linesize, 0, videoCtx->height,
			data,
			linesize);

		if (h > 0)
		{
			printf("(h:%d) ", h);
		}

		printf("pts = %d \n", pts);         //利用pts来控制进度    (pts如果不断往上加, 表示读取视频正常了)
		av_packet_unref(&pkt);              //清理空间      lib: avcodec

	}

	if (cCtx)   //释放转码器
	{
		sws_freeContext(cCtx);
		cCtx = NULL;
	}

	//写ffmpeg的时候,结对编程,有申请空间,就要释放空间
	avformat_close_input(&ic);              //在读视频后,关闭
	ic = NULL;


}