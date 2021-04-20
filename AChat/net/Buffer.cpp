#include "Buffer.h"

#include "../base/Platform.h"
#include "Sockets.h"
#include "Callbacks.h"

using namespace net;

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

int32_t Buffer::readFd(int fd, int* savedErrno)
{
	// saved an ioctl()/FIONREAD call to tell how much to read
	char extrabuf[65536];
    const size_t writable = writableBytes();
//	struct iovec vec[2];
//
//	vec[0].iov_base = begin() + writerIndex_;
//	vec[0].iov_len = writable;
//	vec[1].iov_base = extrabuf;
//	vec[1].iov_len = sizeof extrabuf;
//	// when there is enough space in this buffer, don't read into extrabuf.
//	// when extrabuf is used, we read 128k-1 bytes at most.
//	const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
//	const ssize_t n = sockets::readv(fd, vec, iovcnt);
    const int32_t n = sockets::read(fd, extrabuf, sizeof(extrabuf));
	if (n <= 0)
	{
		*savedErrno = errno;
	}
	else if (implicit_cast<size_t>(n) <= writable)
	{
        writerIndex_ += n;
	}
	else
	{
        //Linuxƽ̨��ʣ�µ��ֽڲ���ȥ
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
	}
	// if (n == writable + sizeof extrabuf)
	// {
	//   goto line_30;
	// }
	return n;
}


