package game::server::View::main;

use strict;
use warnings;

use base 'Catalyst::View::TT';

__PACKAGE__->config(
    TEMPLATE_EXTENSION => '.tt',
    WRAPPER => 'wrapper.tt',
    render_die => 1,
);

=head1 NAME

game::server::View::main - TT View for game::server

=head1 DESCRIPTION

TT View for game::server.

=head1 SEE ALSO

L<game::server>

=head1 AUTHOR

Paul Salcido,,,

=head1 LICENSE

This library is free software. You can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

1;
