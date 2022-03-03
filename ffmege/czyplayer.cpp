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
	//1.ע�����
	av_register_all();
	char *path = "F:\\video.MP4";
	//��װ��ʽ������(�����������������Ϣ)
	AVFormatContext *ic = NULL;

	//2.��������Ƶ�ļ�(0 �ɹ�)
	int re = avformat_open_input(&ic, path, 0, 0); //lib avformat
	if (re != 0) {
		char buf[1024] = { 0 };             //��Ŵ�����Ϣ  (�ļ�������,��ʽ��֧��)
		av_strerror(re, buf, sizeof(buf));  // lib: avutil
		printf("open %s failed: %s\n", path, buf);
		return -1;
	}

	//����Ƶʱ�������Ϊ����
	int totalSec = ic->duration / AV_TIME_BASE;
	printf("file total Sec is %d : %d \n", totalSec / 60, totalSec % 60);

	//��Ƶ���룬��Ҫ�ҵ���Ӧ��AVStream���ڵ�pFormatCtx->streams������λ��
	//nb_streams: ������������,��Ƶ������Ƶ������Ļ��
	int videoStream = 0;
	AVCodecContext *videoCtx = NULL;
	for (int i = 0; i < ic->nb_streams; i++)
	{
		AVCodecContext *enc = ic->streams[i]->codec;
		//3.���������ж��Ƿ�����Ƶ��
		if (enc->codec_type == AVMEDIA_TYPE_VIDEO)
		{

			videoCtx = enc; //���������õ���Ӧ����,�������õ�������������
			videoStream = i;
			//4.��ȡ������. �����������õ������id, ͨ����id�õ�������
			AVCodec *codec = avcodec_find_decoder(enc->codec_id);
			if (!codec)
			{
				printf("video code not find! \n");
			}

			//5.�򿪽�����
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

	//��������(��������)
	AVFrame *yuv = av_frame_alloc();

	int outwidth = 640;
	int outheight = 480;
	SwsContext *cCtx = NULL;

	char *rgb = new char[outwidth * outheight * 4];

	for (;;)
	{
		AVPacket pkt;
		//6.һ֡һ֡��ȡѹ������Ƶ����AVPacket
		re = av_read_frame(ic, &pkt);
		if (re != 0)    break;
		if (pkt.stream_index != videoStream)
		{
			av_packet_unref(&pkt);
			continue;
		}
		int pts = pkt.pts * r2d(ic->streams[pkt.stream_index]->time_base) * 1000;

		//7.1ffmpeg�°汾����: ����
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

		printf("[decode success!] ");           //����ɹ�

		//�����������yuv, ������ʾ��Ҫrgb    yuv->rgb              
		//�����������Ƶ��С, ��һ������ʾ�Ĵ�С, ���ڻ�Ŵ���С(���������Ȼ�����ڿͻ���ת��, ��������Դ, ����һ������, ������������Ŵ���)

		//8.��ȡת����           lib: swscale
		cCtx = sws_getCachedContext(cCtx, videoCtx->width,
			videoCtx->height,
			videoCtx->pix_fmt,
			outwidth,
			outheight,
			AV_PIX_FMT_BGRA,
			SWS_BICUBIC,            //ת����ʲô�㷨
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
		linesize[0] = outwidth * 4;     //һ�еĴ�С, �� * 4
										//9.ת��YUV->RGB8888 (h: ת���ĸ߶�)
		int h = sws_scale(cCtx, yuv->data, yuv->linesize, 0, videoCtx->height,
			data,
			linesize);

		if (h > 0)
		{
			printf("(h:%d) ", h);
		}

		printf("pts = %d \n", pts);         //����pts�����ƽ���    (pts����������ϼ�, ��ʾ��ȡ��Ƶ������)
		av_packet_unref(&pkt);              //����ռ�      lib: avcodec

	}

	if (cCtx)   //�ͷ�ת����
	{
		sws_freeContext(cCtx);
		cCtx = NULL;
	}

	//дffmpeg��ʱ��,��Ա��,������ռ�,��Ҫ�ͷſռ�
	avformat_close_input(&ic);              //�ڶ���Ƶ��,�ر�
	ic = NULL;


}