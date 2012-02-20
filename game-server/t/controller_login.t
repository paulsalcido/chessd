use strict;
use warnings;
use Test::More;


use Catalyst::Test 'game::server';
use game::server::Controller::login;

ok( request('/login')->is_success, 'Request should succeed' );
done_testing();
