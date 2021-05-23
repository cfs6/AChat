
CREATE TABLE `IMAdmin`(
	`id`mdeiumint(6) unsigned NOT NULL AUTO_INCREMENT,
	`uname`varchar(40) NOT NULL COMMMENT '用户名',
	`pwd`char(32) NOT NULL COMMENT '经过加密的密码',
	`status` tinyint(2) unsigned NOT NULL DEFAULT '0' COMMENT '用户状态 0 :正常 1:删除 可扩展',
	`created` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '创建时间',
	`updated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '更新时间',
	PRIMARY KEY (`id`)
)ENGINE=InnoDB DEFAULT CHARSET=utf8


CREATE TABLE `IMGroup`(
    `id` int(11) NOT NULL AUTO_INCREMENT,
	`name` varchar(256) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '群名',
	`creatorID` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '创建者用户ID',
	`groupType` tinyint(3) unsigned NOT NULL DEFAULT '1' COMMENT '群组类型，1-固定;2-临时群',
    `userCnt` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '成员人数',
    `status` tinyint(3) unsigned NOT NULL DEFAULT '1' COMMENT '是否删除,0-正常，1-删除',
    `version` int(11) unsigned NOT NULL DEFAULT '1' COMMENT '群版本号',
    `lastChated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '最后聊天时间',
    `created` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '创建时间',
    `updated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '更新时间',
    PRIMARY KEY(`id`),
    KEY `idx_name` (`name`(191)),
    KEY `idx_creator` (`creator`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin COMMENT='IM群信息'
    
                    
CREATE TABLE `IMGroupMember` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `groupId` int(11) unsigned NOT NULL COMMENT '群Id',
    `userId` int(11) unsigned NOT NULL COMMENT '用户id',
    `status` tinyint(4) unsigned NOT NULL DEFAULT '1' COMMENT '是否退出群，0-正常，1-已退出',
    `created` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '创建时间',
    `updated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '更新时间',
    PRIMARY KEY (`id`),
    KEY `idx_groupId_userId_status` (`groupId`,`userId`,`status`),
    KEY `idx_userId_status_updated` (`userId`,`status`,`updated`),
    KEY `idx_groupId_updated` (`groupId`,`updated`)
) ENGINE=InnoDB AUTO_INCREMENT=68 DEFAULT CHARSET=utf8 COMMENT='用户和群的关系表'


CREATE TABLE `IMGroupMessage_(x)` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `groupId` int(11) unsigned NOT NULL COMMENT '用户的关系id',
    `userId` int(11) unsigned NOT NULL COMMENT '发送用户的id',
    `msgId` int(11) unsigned NOT NULL COMMENT '消息ID',
    `content` varchar(4096) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '消息内容',
    `type` tinyint(3) unsigned NOT NULL DEFAULT '2' COMMENT '群消息类型,1为群语音,2为文本',
    `status` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '消息状态',
    `created` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '创建时间',
    `updated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '更新时间',
    PRIMARY KEY (`id`),
    KEY `idx_groupId_status_created` (`groupId`,`status`,`created`),
    KEY `idx_groupId_msgId_status_created` (`groupId`,`msgId`,`status`,`created`)   
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin COMMENT='IM群消息表'
                    
                    
CREATE TABLE `IMMessage_0` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `relateId` int(11) unsigned NOT NULL COMMENT '用户的关系id',
    `fromId` int(11) unsigned NOT NULL COMMENT '发送用户的id',
    `toId` int(11) unsigned NOT NULL COMMENT '接收用户的id',
    `msgId` int(11) unsigned NOT NULL COMMENT '消息ID',
    `content` varchar(4096) COLLATE utf8mb4_bin DEFAULT '' COMMENT '消息内容',
    `type` tinyint(2) unsigned NOT NULL DEFAULT '1' COMMENT '消息类型',
    `status` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT '0正常 1被删除',
    `created` int(11) unsigned NOT NULL COMMENT '创建时间', 
    `updated` int(11) unsigned NOT NULL COMMENT '更新时间',     PRIMARY KEY (`id`),
    KEY `idx_relateId_status_created` (`relateId`,`status`,`created`),
    KEY `idx_relateId_status_msgId_created` (`relateId`,`status`,`msgId`,`created`),
    KEY `idx_fromId_toId_created` (`fromId`,`toId`,`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin           
                    
                    
CREATE TABLE `IMRecentSession` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `userId` int(11) unsigned NOT NULL COMMENT '用户id',
    `peerId` int(11) unsigned NOT NULL COMMENT '对方id',
    `type` tinyint(1) unsigned DEFAULT '0' COMMENT '类型，1-用户,2-群组',
    `status` tinyint(1) unsigned DEFAULT '0' COMMENT '用户:0-正常, 1-用户A删除,群组:0-正常, 1-被删除',
    `created` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '创建时间',
    `updated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '更新时间',
    PRIMARY KEY (`id`),
    KEY `idx_userId_peerId_status_updated` (`userId`,`peerId`,`status`,`updated`),
    KEY `idx_userId_peerId_type` (`userId`,`peerId`,`type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8                    

                    
CREATE TABLE `IMRelationShip` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `smallId` int(11) unsigned NOT NULL COMMENT '用户A的id',
    `bigId` int(11) unsigned NOT NULL COMMENT '用户B的id',
    `status` tinyint(1) unsigned DEFAULT '0' COMMENT '用户:0-正常, 1-用户A删除,群组:0-正常, 1-被删除',
    `created` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '创建时间',
    `updated` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '更新时间',
    PRIMARY KEY (`id`),
    KEY `idx_smallId_bigId_status_updated` (`smallId`,`bigId`,`status`,`updated`)   
) ENGINE=InnoDB DEFAULT CHARSET=utf8                    

                    
CREATE TABLE `IMUser` (
    `id` int(11) unsigned NOT NULL AUTO_INCREMENT COMMENT '用户id',
    `sex` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT '1男2女0未知',
    `name` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '用户名',
    `domain` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '拼音',
    `nick` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '花名,绰号等',
    `signInfo` varchar(255) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '用户签名',
    `password` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '密码',
    `salt` varchar(4) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '混淆码',
    `phone` varchar(11) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '手机号码',
    `email` varchar(64) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT 'email',
    `avatar` varchar(255) COLLATE utf8mb4_bin DEFAULT '' COMMENT '自定义用户头像',
    `status` tinyint(2) unsigned DEFAULT '0' COMMENT '0. 在线 1. 隐身 2. 离开',
    `created` int(11) unsigned NOT NULL COMMENT '创建时间',
    `updated` int(11) unsigned NOT NULL COMMENT '更新时间',
    PRIMARY KEY (`id`),
    KEY `idx_domain` (`domain`),
    KEY `idx_name` (`name`),
    KEY `idx_phone` (`phone`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin
                    
         