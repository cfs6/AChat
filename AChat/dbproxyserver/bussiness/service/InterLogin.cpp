#include "InterLogin.h"
#include "../DBPool.h"
#include "EncDec.h"

bool CInterLoginStrategy::doLogin(const std::string &strName, const std::string &strPass, IM::BaseDefine::UserInfo& user)
{
    bool bRet = false;
    DBManager* pDBManger = DBManager::getInstance();
    DBConn* pDBConn = pDBManger->getDBConn("teamtalk_slave");
    if (pDBConn) {
        string strSql = "select * from IMUser where name='" + strName + "' and status=0";
        ResultSet* pResultSet = pDBConn->executeQuery(strSql.c_str());
        if(pResultSet)
        {
            string strResult, strSalt;
            uint32_t nId, nGender, nDeptId, nStatus;
            string strNick, strAvatar, strEmail, strRealName, strTel, strDomain,strSignInfo;
            while (pResultSet->next()) {
                nId = pResultSet->getInt("id");
                strResult = pResultSet->getString("password");
                strSalt = pResultSet->getString("salt");
                
                strNick = pResultSet->getString("nick");
                nGender = pResultSet->getInt("sex");
                strRealName = pResultSet->getString("name");
                strDomain = pResultSet->getString("domain");
                strTel = pResultSet->getString("phone");
                strEmail = pResultSet->getString("email");
                strAvatar = pResultSet->getString("avatar");
                nDeptId = pResultSet->getInt("departId");
                nStatus = pResultSet->getInt("status");
                strSignInfo = pResultSet->getString("sign_info");

            }

            string strInPass = strPass + strSalt;
            char szMd5[33];
            CMd5::MD5_Calculate(strInPass.c_str(), strInPass.length(), szMd5);
            string strOutPass(szMd5);
            //去掉密码校验
            //if(strOutPass == strResult)
            {
                bRet = true;
                user.set_user_id(nId);
                user.set_user_nick_name(strNick);
                user.set_user_gender(nGender);
                user.set_user_real_name(strRealName);
                user.set_user_domain(strDomain);
                user.set_user_tel(strTel);
                user.set_email(strEmail);
                user.set_avatar_url(strAvatar);
                user.set_department_id(nDeptId);
                user.set_status(nStatus);
  	        user.set_sign_info(strSignInfo);

            }
            delete  pResultSet;
        }
        pDBManger->relDBConn(pDBConn);
    }
    return bRet;
}
