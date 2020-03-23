create table songs
	(
		id int(5) unsigned  auto_increment primary key,
		name varchar(50) not null unique,
		description varchar(100) not null,
		link varchar(100) not null
	);