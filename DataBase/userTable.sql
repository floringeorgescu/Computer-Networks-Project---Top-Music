create table users
	(
		id int(5) unsigned  auto_increment primary key,
		username varchar(50) not null unique,
		`password` varchar(50) not null,
		type int(1) not null default 0,
		canVote int(1) not null default 1
	);