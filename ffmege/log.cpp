/*
mpeg ����־���÷�

#include <libavutil/log.h>
// ������־����
av_log_set_level(AV_LOG_DEBUG)
// ��ӡ��־
av_log(NULL, AV_LOG_INFO,"...%s\n",op)

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavutil/log.h>
}
// ���������־�ĺ��������׸�ʹ����ʵ��
extern void Ffmpeglog(int, char*);

static void log_callback(void *avcl, int level, const char *fmt, va_list vl)
{
	(void)avcl;
	char log[1024] = { 0 };
	//��int vsnprintf(buffer,bufsize ,fmt, argptr) , va_list �ǿɱ������ָ�룬��ط�����: va_start(), type va_atg(va_list, type), va_end()
	int n = vsnprintf(log, 1024, fmt, vl);
	if (n > 0 && log[n - 1] == '\n')
		log[n - 1] = 0;
	if (strlen(log) == 0)
		return;
	Ffmpeglog(level, log);
}

void set_log_callback()
{
	// �� av ������ע����־�ص�����
	av_log_set_callback(log_callback);
}
