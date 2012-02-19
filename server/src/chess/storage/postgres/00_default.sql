create table player (id varchar primary key,username varchar unique not null,fullname varchar not null,password varchar not null,created float default extract(epoch from now()));
