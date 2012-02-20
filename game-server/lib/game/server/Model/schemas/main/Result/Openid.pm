package game::server::Model::schemas::main::Result::Openid;

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

game::server::Model::schemas::main::Result::Openid

=cut

__PACKAGE__->table("main.openid");

=head1 ACCESSORS

=head2 id

  data_type: 'text'
  is_nullable: 0
  original: {data_type => "varchar"}

=head2 email

  data_type: 'text'
  is_nullable: 0
  original: {data_type => "varchar"}

=head2 identity

  data_type: 'text'
  is_nullable: 0
  original: {data_type => "varchar"}

=head2 fullname

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
  "email",
  {
    data_type   => "text",
    is_nullable => 0,
    original    => { data_type => "varchar" },
  },
  "identity",
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
  "created",
  {
    data_type     => "double precision",
    default_value => \"date_part('epoch'::text, now())",
    is_nullable   => 1,
  },
);
__PACKAGE__->set_primary_key("id");
__PACKAGE__->add_unique_constraint("openid_identity_key", ["identity"]);
__PACKAGE__->add_unique_constraint("openid_email_key", ["email"]);

=head1 RELATIONS

=head2 players

Type: has_many

Related object: L<game::server::Model::schemas::main::Result::Player>

=cut

__PACKAGE__->has_many(
  "players",
  "game::server::Model::schemas::main::Result::Player",
  { "foreign.openid" => "self.id" },
  { cascade_copy => 0, cascade_delete => 0 },
);


# Created by DBIx::Class::Schema::Loader v0.07010 @ 2012-02-19 18:41:12
# DO NOT MODIFY THIS OR ANYTHING ABOVE! md5sum:nbck2KmQae6+dZ1tfDf6oQ


# You can replace this text with custom code or comments, and it will be preserved on regeneration
__PACKAGE__->meta->make_immutable;
1;
