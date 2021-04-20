#include <list>
#include "../ProxyConn.h"
#include "../HttpClient.h"
#include "../SyncCenter.h"
#include "Login.h"
#include "UserModel.h"
#include "TokenValidator.h"
#include "json/json.h"
#include "Common.h"
#include "IM.Server.pb.h"
#include "Base64.h"
#include "InterLogin.h"
#include "ExterLogin.h"
#include <AsyncLog.h>
CInterLoginStrategy g_loginStrategy;

hash_map<string, list<uint32_t> > errorTimeMap;
std::mutex loginMutex;
namespace DB_PROXY {
    
    
void doLogin(ImPdu* pPdu, uint32_t conn_uuid)
{
    
    ImPdu* pPduResp = new ImPdu;
    
    IM::Server::IMValidateReq msg;
    IM::Server::IMValidateRsp msgResp;
    if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
    {
        
        string strDomain = msg.user_name();
        string strPass = msg.password();
        
        msgResp.set_user_name(strDomain);
        msgResp.set_attach_data(msg.attach_data());
        
        do
        {
            list<uint32_t>& errorTimeList = errorTimeMap[strDomain];
            uint32_t tmNow = time(NULL);
            
            //清理超过30分钟的错误时间点记录
            /*
             清理放在这里还是放在密码错误后添加的时候呢？
             放在这里，每次都要遍历，会有一点点性能的损失。
             放在后面，可能会造成30分钟之前有10次错的，但是本次是对的就没办法再访问了。
             */
            auto itTime=errorTimeList.begin();
            for(; itTime!=errorTimeList.end();++itTime)
            {
                if(tmNow - *itTime > 30*60)
                {
                    break;
                }
            }
            if(itTime != errorTimeList.end())
            {
            	unique_lock<std::mutex> lock(loginMutex);
                errorTimeList.erase(itTime, errorTimeList.end());
            }
            
            // 判断30分钟内密码错误次数是否大于10
            if(errorTimeList.size() > 10)
            {
                itTime = errorTimeList.begin();
                if(tmNow - *itTime <= 30*60)
                {
                    msgResp.set_result_code(6);
                    msgResp.set_result_string("用户名/密码错误次数过多");
                    pPduResp->SetPBMsg(&msgResp);
                    pPduResp->SetSeqNum(pPdu->GetSeqNum());
                    pPduResp->SetServiceId(IM::BaseDefine::SID_OTHER);
                    pPduResp->SetCommandId(IM::BaseDefine::CID_OTHER_VALIDATE_RSP);
                    ProxyConn::AddResponsePdu(conn_uuid, pPduResp);
                    return ;
                }
            }
        } while(false);
        
        LOGD("%s request login.", strDomain.c_str());
        
        
        IM::BaseDefine::UserInfo cUser;
        
        if(g_loginStrategy.doLogin(strDomain, strPass, cUser))
        {
            IM::BaseDefine::UserInfo* pUser = msgResp.mutable_user_info();
            pUser->set_user_id(cUser.user_id());
            pUser->set_user_gender(cUser.user_gender());
            pUser->set_department_id(cUser.department_id());
            pUser->set_user_nick_name(cUser.user_nick_name());
            pUser->set_user_domain(cUser.user_domain());
            pUser->set_avatar_url(cUser.avatar_url());
            
            pUser->set_email(cUser.email());
            pUser->set_user_tel(cUser.user_tel());
            pUser->set_user_real_name(cUser.user_real_name());
            pUser->set_status(0);

            pUser->set_sign_info(cUser.sign_info());
           
            msgResp.set_result_code(0);
            msgResp.set_result_string("成功");
            
            //如果登陆成功，则清除错误尝试限制
            {
				unique_lock<std::mutex> lock(loginMutex);
				list<uint32_t>& errorTimeList = errorTimeMap[strDomain];
				errorTimeList.clear();
            }
        }
        else
        {
            uint32_t tmCurrent = time(NULL);
            //密码错误，记录一次登陆失败
        	{
            	unique_lock<std::mutex> lock(loginMutex);
                list<uint32_t>& errorTimeList = errorTimeMap[strDomain];
                errorTimeList.push_front(tmCurrent);
        	}

            LOGD("get result false");
            msgResp.set_result_code(1);
            msgResp.set_result_string("用户名/密码错误");
        }
    }
    else
    {
        msgResp.set_result_code(2);
        msgResp.set_result_string("服务端内部错误");
    }
    
    
    pPduResp->SetPBMsg(&msgResp);
    pPduResp->SetSeqNum(pPdu->GetSeqNum());
    pPduResp->SetServiceId(IM::BaseDefine::SID_OTHER);
    pPduResp->SetCommandId(IM::BaseDefine::CID_OTHER_VALIDATE_RSP);
    ProxyConn::AddResponsePdu(conn_uuid, pPduResp);
}

};

