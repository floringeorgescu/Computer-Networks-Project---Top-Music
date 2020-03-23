create table comments
	(
		id int(5) unsigned  auto_increment primary key,
		username varchar(50) not null,
		songName varchar(50) not null,
		comment varchar(300) not null
	);