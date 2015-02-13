#include <stdio.h>
#include "md5.h"

const char *text = "hey there";

int main(int argc, char **argv) {
	MD5_CTX m_ctx;
	unsigned char result;

	printf("starting md5\n");
	MD5_Init(&m_ctx);
	MD5_Update(&m_ctx, (const void *) text, 10);


	MD5_Final(&result, &m_ctx);
	printf("md5 done\n");
	return 0;
}
