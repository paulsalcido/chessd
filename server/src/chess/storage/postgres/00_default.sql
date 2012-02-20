create schema main;
create table main.openid (
    id varchar primary key
    ,email varchar not null unique
    ,identity varchar not null unique
    ,fullname varchar not null
    ,created float default extract(epoch from now())
);
create table main.player (
    id varchar primary key
    ,openid varchar references main.openid(id) not null
    ,username varchar unique not null
    ,fullname varchar not null
    ,password varchar not null
    ,salt varchar not null
    ,created float default extract(epoch from now())
);
