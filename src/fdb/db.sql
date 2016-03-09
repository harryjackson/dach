create table fdb_files(
fname         TEXT primary key not null,
fsize         BIGINT not null,
md5_partial   varchar(32) default "0",
md5_full      varchar(32) default "0"
);
