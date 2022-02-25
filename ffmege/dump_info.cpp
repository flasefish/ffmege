#include "dump_info.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

int dump_info(void* data, int data_size) {
	int ret = -1;
	AVFormatContext *fmt_ctx = NULL; // AV ��ʽ������
	AVIOContext *avio_ctx = NULL; // AV IO ������
	unsigned char *avio_ctx_buffer = NULL; // input buffer

	fmt_ctx = avformat_alloc_context(); // ��� AV format ���
	if (NULL == fmt_ctx) {
		av_log(NULL, AV_LOG_ERROR, "Could not alloc format context\n");
		goto clean1;
	}
	avio_ctx_buffer = (unsigned char *)av_malloc(data_size); // ffmpeg�����ڴ�ķ�������������仺��
	if (NULL == avio_ctx_buffer) {
		av_log(NULL, AV_LOG_ERROR, "Could not alloc indata\n");
		goto clean1;
	}
	memcpy(avio_ctx_buffer, data, data_size); // ���ƴ�������ļ�����

											  // �� av format context ���� io �����ľ������Ҫ���Σ��������ݵ�ָ�롢���ݴ�С��write_flag��������buffer����д�������������ԣ��� NULL
	avio_ctx = avio_alloc_context(avio_ctx_buffer, data_size, 0, NULL, NULL, NULL, NULL);
	if (NULL == avio_ctx) {
		av_log(NULL, AV_LOG_ERROR, "Could not alloc io context\n");
		av_free(avio_ctx_buffer);
		goto clean1;
	}
	fmt_ctx->pb = avio_ctx;

	// �����������������ȡ header �ĸ�ʽ���ݣ�ע�����ĺ������� avformat_close_input()
	if (avformat_open_input(&fmt_ctx, NULL, NULL, NULL) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Could not open input data\n");
		goto clean2;
	}

	/* retrieve stream information, Read packets of a media file to get stream information */
	if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Could not find stream information\n");
		goto clean2;
	}

	// av_dump_format(fmt_ctx, 0, NULL, 0);

	// find codec
	int video_stream_idx = -1, audio_stream_idx = -1;
	AVStream *video_stream = NULL, *audio_stream = NULL;
	AVCodecContext *video_decodec_ctx = NULL, *audio_decodec_ctx = NULL;

	// AVFormatContext.nb_stream ��¼�˸� URL �а����м�·��
	for (int i = 0; i<fmt_ctx->nb_streams; i++) {
		AVStream *stream = fmt_ctx->streams[i];
		AVCodecParameters *codec_par = stream->codecpar;
		AVCodec *decodec = NULL;
		AVCodecContext *decodec_ctx = NULL;

		av_log(NULL, AV_LOG_INFO, "find audio stream index=%d, type=%s, codec id=%d",
			i, av_get_media_type_string(codec_par->codec_type), codec_par->codec_id);

		// ��ý�����
		decodec = avcodec_find_decoder(codec_par->codec_id);
		if (!decodec) {
			av_log(NULL, AV_LOG_ERROR, "fail to find decodec\n");
			goto clean2;
		}

		av_log(NULL, AV_LOG_INFO, "find codec name=%s\t%s", decodec->name, decodec->long_name);

		// ��������������ľ��
		decodec_ctx = avcodec_alloc_context3(decodec);
		if (!decodec_ctx) {
			av_log(NULL, AV_LOG_ERROR, "fail to allocate codec context\n");
			goto clean2;
		}

		// ��������Ϣ��������������
		if (avcodec_parameters_to_context(decodec_ctx, codec_par) < 0) {
			av_log(NULL, AV_LOG_ERROR, "fail to copy codec parameters to decoder context\n");
			avcodec_free_context(&decodec_ctx);
			goto clean2;
		}

		// ��ʼ��������
		if ((ret = avcodec_open2(decodec_ctx, decodec, NULL)) < 0) {
			av_log(NULL, AV_LOG_ERROR, "Failed to open %s codec\n", decodec->name);
			return ret;
		}

		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			// ��Ƶ�����ԣ�֡�ʣ����� av_guess_frame_rate() �Ǳ��룬��ҵ���Ƿ���Ҫʹ��֡�ʲ���
			decodec_ctx->framerate = av_guess_frame_rate(fmt_ctx, stream, NULL);
			av_log(NULL, AV_LOG_INFO, "video framerate=%d/%d", decodec_ctx->framerate.num, decodec_ctx->framerate.den);
			video_stream_idx = i;
			video_stream = stream;
			video_decodec_ctx = decodec_ctx;
		}
		else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream_idx = i;
			audio_stream = stream;
			audio_decodec_ctx = decodec_ctx;
		}
	}


	av_log(NULL, AV_LOG_INFO, "video_decodec_ctx=%p audio_decodec_ctx=%p", video_decodec_ctx, audio_decodec_ctx);

	/* begin ������� */
	AVPacket *pkt;
	AVFrame *frame;

	// ����ԭʼ�ļ���packet�Ļ���
	pkt = av_packet_alloc();
	if (!pkt) {
		av_log(NULL, AV_LOG_ERROR, "Could not allocate video frame\n");
		goto clean3;
	}

	// ���� AV ֡ ���ڴ棬����ָ��
	frame = av_frame_alloc();
	if (!frame) {
		av_log(NULL, AV_LOG_ERROR, "Could not allocate video frame\n");
		goto clean4;
	}

	while (av_read_frame(fmt_ctx, pkt) >= 0) {
		if (pkt->size) {
			/*
			demux �⸴��
			ԭʼ���������У���ͬ��ʽ�����ύ����һ�𣨶�·���ã�
			��ԭʼ���ж�ȡ��ÿһ�� packet ���������ǲ�һ���ģ���Ҫ�ж� packet ���������������ʹ���
			*/
			if (pkt->stream_index == video_stream_idx) {
				// �����������ԭʼѹ������ packet
				if ((ret = avcodec_send_packet(video_decodec_ctx, pkt)) < 0) {
					av_log(NULL, AV_LOG_ERROR, "Error sending a packet for decoding, ret=%d", ret);
					break;
				}
				/*
				���������Ƶ֡
				avcodec_receive_frame()���� EAGAIN ��ʾ��Ҫ����֡���������
				�� MPEG�ȸ�ʽ, P֡(Ԥ��֡)��Ҫ����I֡(�ؼ�֡)����ǰ���P֡��ʹ�ñȽϻ��߲�ַ�ʽ����
				��ȡframe��Ҫѭ������Ϊ��ȡ���packet�󣬿��ܻ�ö��frame
				*/
				while (ret >= 0) {
					ret = avcodec_receive_frame(video_decodec_ctx, frame);
					if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
						break;
					}

					/*
					DEBUG ��ӡ����Ƶ��ʱ��
					pts = display timestamp
					��Ƶ���л�׼ʱ�� time_base ����ÿ 1 pts ��ʱ����(��λ��)
					ʹ�� pts * av_q2d(time_base) ��֪��ǰ֡����ʾʱ��
					*/
					if (video_decodec_ctx->frame_number % 100 == 0) {
						av_log(NULL, AV_LOG_INFO, "read video No.%d frame, pts=%d, timestamp=%f seconds",
							video_decodec_ctx->frame_number, frame->pts, frame->pts * av_q2d(video_stream->time_base));
					}

					/*
					�ڵ�һ����Ƶ֡��ȡ�ɹ�ʱ�����Խ��У�
					������Ҫת�룬��ʼ����Ӧ�ı�����
					������Ҫ�ӹ�����������ˮӡ����ת�ȣ������ʼ�� filter
					*/
					if (video_decodec_ctx->frame_number == 1) {

					}
					else {

					}


					av_frame_unref(frame);
				}
			}
			else if (pkt->stream_index == audio_stream_idx) {

			}
		}

		av_packet_unref(pkt);
		av_frame_unref(frame);
	}
	/* end ������� */

	// send NULL packet, flush data
	avcodec_send_packet(video_decodec_ctx, NULL);
	avcodec_send_packet(audio_decodec_ctx, NULL);

clean5:
	av_frame_free(&frame);
	//av_parser_close(parser);
clean4:
	av_packet_free(&pkt);
clean3:
	if (NULL != video_decodec_ctx)
		avcodec_free_context(&video_decodec_ctx);
	if (NULL != audio_decodec_ctx)
		avcodec_free_context(&audio_decodec_ctx);
clean2:
	av_freep(&fmt_ctx->pb->buffer);
	av_freep(&fmt_ctx->pb);
clean1:
	avformat_close_input(&fmt_ctx);
end:
	return ret;
}