//zlibѹ�����ѹʹ��ʾ��
//#include "zlib.h"

//int CompressBuf()
//{
//    char text[] = "zlib compress and uncompress test\nturingo@163.com\n2012-11-05\n";
//    uLong tlen = strlen(text) + 1;  /* ��Ҫ���ַ����Ľ�����'\0'Ҳһ������ */
//    char* buf = NULL;
//    uLong blen;
//
//    /* ���㻺������С����Ϊ������ڴ� */
//    blen = compressBound(tlen); /* ѹ����ĳ����ǲ��ᳬ��blen�� */
//    if ((buf = (char*)malloc(sizeof(char) * blen)) == NULL)
//    {
//        printf("no enough memory!\n");
//        return -1;
//    }
//
//    /* ѹ�� */
//    if (compress((Bytef*)buf, &blen, (const Bytef*)text, tlen) != Z_OK)
//    {
//        printf("compress failed!\n");
//        return -1;
//    }
//
//    /* ��ѹ�� */
//    if (uncompress((Bytef*)text, &tlen, (const Bytef*)buf, blen) != Z_OK)
//    {
//        printf("uncompress failed!\n");
//        return -1;
//    }
//
//    /* ��ӡ��������ͷ��ڴ� */
//    printf("%s", text);
//    if (buf != NULL)
//    {
//        free(buf);
//        buf = NULL;
//    }
//
//    return 0;
//}