package game::server::Controller::login;
use Moose;
use namespace::autoclean;

use Net::OpenID::Consumer;
use LWPx::ParanoidAgent;
use Data::Dumper;

BEGIN {extends 'Catalyst::Controller'; }

=head1 NAME

game::server::Controller::login - Catalyst Controller

=head1 DESCRIPTION

Catalyst Controller.

=head1 METHODS

=cut


=head2 index

=cut

sub index :Path :Args(0) {
    my ( $self, $c ) = @_;
       
        my $oid = Net::OpenID::Consumer->new(
            ua => LWP::UserAgent->new,
            required_root => $c->request->base,
            consumer_secret => 'lksjoibnvoaibsodiasfd',
        );
        my $cid = $oid->claimed_identity('https://www.google.com/accounts/o8/id');
        if ( $cid ) {
            $cid->set_extension_args(
                'http://openid.net/srv/ax/1.0',
                {
                    required => 'email,firstname,lastname',
                    mode => 'fetch_request',
                    'type.email' => 'http://axschema.org/contact/email',
                    'type.firstname' => 'http://axschema.org/namePerson/first',
                    'type.lastname' => 'http://axschema.org/namePerson/last',
                },
            );
            my $redurl = $cid->check_url(
                return_to => $c->request->base.'login/openid/',
                trust_root => $c->request->base,
                delayed_return => 1,
            );
            $redurl =~ s/\.e1([\=\.])/.ax$1/g;
            $c->log->info('Redirect url: '.$redurl);
            $c->response->redirect($redurl);
            $c->detach;
            return;
        }
}

sub openid :Local :Args(0) {
    my ( $self, $c ) = @_;
    
    my $oid = Net::OpenID::Consumer->new(
        ua => LWP::UserAgent->new,
        required_root => $c->request->base,
        consumer_secret => 'lksjoibnvoaibsodiasfd',
        args => $c->request->params,
        );
    $oid->handle_server_response(
        not_openid => sub {
            $c->log->info("not_openid");
        },
        setup_required => sub {
            my $setup_url = shift;
            $c->log->info("setup_required $setup_url");
            $c->response->redirect($setup_url);
            $c->detach;
        },
        cancelled => sub {
            $c->log->info("cancelled");
        },
        verified => sub {
            my $vident = $oid->verified_identity;
            my $items = $vident->signed_extension_fields('http://openid.net/srv/ax/1.0');
            my $fullname = undef;
            my $email = undef;
            my $identity = $vident->url;
            if ( $items->{'value.fullname'} ) {
                $fullname = $items->{'value.fullname'};
            } elsif ( $items->{'value.firstname'} && $items->{'value.lastname'} ) {
                $fullname = $items->{'value.firstname'} . ' ' . $items->{'value.lastname'};
            }
            if ( $items->{'value.email'} ) {
                $email = $items->{'value.email'};
            }
            $c->log->info("$identity $fullname $email");
            my $oidm = $c->model('main::OpenId')->find({
                email => $email,
            });
            unless ( $oidm ) {
                $oidm = $c->model('main::OpenId')->create({
                    id => $c->uuid,
                    identity => $identity,
                    email => $email,
                    fullname => $fullname,
                });
            }
            if ( $oidm->identity ne $identity || $oidm->fullname ne $fullname ) {
                $oidm->update({identity => $identity,fullname=>$fullname});
            }
            $c->stash->{openid} = $oidm;
            $c->setup_user_session($oidm);
        },
        error => sub {
            my $err = shift;
            $c->log->info("error $err");
        },
    );
    $c->detach;
}

sub end : Private {
    my ( $self, $c ) = @_;

    $c->forward( $c->view('main') );
}


=head1 AUTHOR

Paul Salcido,,,

=head1 LICENSE

This library is free software. You can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

__PACKAGE__->meta->make_immutable;

1;
