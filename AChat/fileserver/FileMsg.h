#pragma once

#include <stdint.h>

enum file_msg_type
{
    file_msg_type_unknown,
    msg_type_upload_req,
    msg_type_upload_resp,
    msg_type_download_req,
    msg_type_download_resp,
};

enum file_msg_error_code
{
    file_msg_error_unknown,
    file_msg_error_progress,
    file_msg_error_complete,
    file_msg_error_not_exist
};

enum client_net_type
{
    client_net_type_broadband,
    client_net_type_cellular
};

#pragma pack(push, 1)
struct file_msg_header
{
    int64_t  packagesize;
};

#pragma pack(pop)


