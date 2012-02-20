package game::server::Model::schemas::main::Result::Player;

# Created by DBIx::Class::Schema::Loader
# DO NOT MODIFY THE FIRST PART OF THIS FILE

use strict;
use warnings;

use Moose;
use MooseX::NonMoose;
use namespace::autoclean;
extends 'DBIx::Class::Core';

__PACKAGE__->load_components("InflateColumn::DateTime");

=head1 NAME

game::server::Model::schemas::main::Result::Player

=cut

__PACKAGE__->table("main.player");

=head1 ACCESSORS

=head2 id

  data_type: 'text'
  is_nullable: 0
  original: {data_type => "varchar"}

=head2 openid

  data_type: 'text'
  is_foreign_key: 1
  is_nullable: 0
  original: {data_type => "varchar"}

=head2 username

  data_type: 'text'
  is_nullable: 0
  original: {data_type => "varchar"}

=head2 fullname

  data_type: 'text'
  is_nullable: 0
  original: {data_type => "varchar"}

=head2 password

  data_type: 'text'
  is_nullable: 0
  original: {data_type => "varchar"}

=head2 salt

  data_type: 'text'
  is_nullable: 0
  original: {data_type => "varchar"}

=head2 created

  data_type: 'double precision'
  default_value: date_part('epoch'::text, now())
  is_nullable: 1

=cut

__PACKAGE__->add_columns(
  "id",
  {
    data_type   => "text",
    is_nullable => 0,
    original    => { data_type => "varchar" },
  },
  "openid",
  {
    data_type      => "text",
    is_foreign_key => 1,
    is_nullable    => 0,
    original       => { data_type => "varchar" },
  },
  "username",
  {
    data_type   => "text",
    is_nullable => 0,
    original    => { data_type => "varchar" },
  },
  "fullname",
  {
    data_type   => "text",
    is_nullable => 0,
    original    => { data_type => "varchar" },
  },
  "password",
  {
    data_type   => "text",
    is_nullable => 0,
    original    => { data_type => "varchar" },
  },
  "salt",
  {
    data_type   => "text",
    is_nullable => 0,
    original    => { data_type => "varchar" },
  },
  "created",
  {
    data_type     => "double precision",
    default_value => \"date_part('epoch'::text, now())",
    is_nullable   => 1,
  },
);
__PACKAGE__->set_primary_key("id");
__PACKAGE__->add_unique_constraint("player_username_key", ["username"]);

=head1 RELATIONS

=head2 openid

Type: belongs_to

Related object: L<game::server::Model::schemas::main::Result::Openid>

=cut

__PACKAGE__->belongs_to(
  "openid",
  "game::server::Model::schemas::main::Result::Openid",
  { id => "openid" },
  { is_deferrable => 1, on_delete => "CASCADE", on_update => "CASCADE" },
);


# Created by DBIx::Class::Schema::Loader v0.07010 @ 2012-02-19 18:41:12
# DO NOT MODIFY THIS OR ANYTHING ABOVE! md5sum:X+d15EK6KIsRN5KdM/aQWQ


# You can replace this text with custom code or comments, and it will be preserved on regeneration
__PACKAGE__->meta->make_immutable;
1;
