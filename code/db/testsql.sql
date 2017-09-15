drop table if exists `user`;
create table `user`
(
	`id` int(10) unsigned zerofill not null auto_increment,
	`user_key` varchar(255) not null default '',
	`login_time` int(4) unsigned not null default '0',
	`logout_time` int(4) unsigned not null default '0',
	`online_minutes` int(4) not null default '0',
	`offline_minutes` int(4) not null default '0',
	primary key(`id`)
)default charset=utf8;

insert into `user` values('1','abcccddddd','1244555','22222','2','75');
insert into `user` values('2','ddcca','6643','11','4','76');
insert into `user` values('3','kkgsrf','345','22333','33434','78');
insert into `user` values('4','哈哈','7777','221544','555','90');
insert into `user` values('5','呵呵呵','565453','7743','8888','900000');
insert into `user` values('6','啦啦啦','84313','8896','999','89000');
insert into `user` values('7','rrr','66554','666','14444','9000');
insert into `user` values('8','十面埋伏','55642','55445','6666','9527');