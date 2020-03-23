create table votes
	(
		id int(5) unsigned  auto_increment primary key,
		songName varchar(50) not null,
		username varchar(100) not null,
		vote int(2) not null,
		genre varchar(50) not null
	);