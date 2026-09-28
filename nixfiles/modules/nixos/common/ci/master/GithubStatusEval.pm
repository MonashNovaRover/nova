package Hydra::Plugin::GithubStatusEval;

use utf8;
use strict;
use warnings;
use parent 'Hydra::Plugin';
use Template;
use Hydra::Helper::Nix;
use Hydra::Config;
use HTTP::Request;
use JSON::MaybeXS;
use LWP::UserAgent;

sub isEnabled {
    my ($self) = @_;
    return defined $self->{config}->{githubstatuseval};
}

sub common {
    my ($self, $jobset, $evaluation) = @_;

    my $cfg = $self->{config}->{githubstatus};
    my @config = defined $cfg ? ref $cfg eq "ARRAY" ? @$cfg : ($cfg) : ();
    my $baseurl = $self->{config}->{'base_uri'} || "http://localhost:3000";

    my $projectName = $jobset->get_column('project');
    my $jobsetName = $jobset->name;
    my $name = "$projectName:$jobsetName";
    my $github_job_name = $jobsetName =~ s/-pr-\d+//r;
    my $errorMsg = $jobset->errormsg;

    my $state = defined $evaluation ? ((defined $errorMsg && $errorMsg ne "") ? "failed" : "success") : "pending";
    my $url = defined $evaluation ? "$baseurl/eval/" . $evaluation->id : "$baseurl/jobset/$projectName:$jobsetName";
    my $desc = defined $evaluation ? "Hydra evaluation of $jobsetName" : "Hydra evaluation #" . $evaluation->id . " of $jobsetName";

    my $extendedContext = $conf->{context} // "continuous-integration/hydra:" . $jobsetName;
    my $shortContext = $conf->{context} // "ci/hydra:" . $github_job_name;
    my $context = $conf->{useShortContext} ? $shortContext : $extendedContext;

    my $body = encode_json(
        {
            state => $state,
            target_url => $url,
            description => $desc,
            context => $context
        });

    my $inputs_cfg = $conf->{inputs};
    my @inputs = defined $inputs_cfg ? ref $inputs_cfg eq "ARRAY" ? @$inputs_cfg : ($inputs_cfg) : ();
    my %seen = map { $_ => {} } @inputs;


    my $sendStatus = sub {
      my ($input, $owner, $repo, $rev) = @_;

      my $key = $owner . "-" . $repo . "-" . $rev;
      return if exists $seen{$input}->{$key};
      $seen{$input}->{$key} = 1;

      my $url = "https://api.github.com/repos/$owner/$repo/statuses/$rev";
      my $req = HTTP::Request->new('POST', $url);
      $req->header('Content-Type' => 'application/json');
      $req->header('Accept' => 'application/vnd.github.v3+json');
      $req->header('Authorization' => ($self->{config}->{github_authorization}->{$owner} // $conf->{authorization}));
      $req->content($body);
      my $res = $ua->request($req);
      print STDERR $res->status_line, ": ", $res->decoded_content, "\n" unless $res->is_success;
      my $limit = $res->header("X-RateLimit-Limit");
      my $limitRemaining = $res->header("X-RateLimit-Remaining");
      my $limitReset = $res->header("X-RateLimit-Reset");
      my $now = time();
      my $diff = $limitReset - $now;
      my $delay = (($limit - $limitRemaining) / $diff) * 5;
      if ($limitRemaining < 1000) {
        $delay = max(1, $delay);
      }
      if ($limitRemaining < 2000) {
        print STDERR "GithubStatus ratelimit $limitRemaining/$limit, resets in $diff, sleeping $delay\n";
        sleep $delay;
      } else {
        print STDERR "GithubStatus ratelimit $limitRemaining/$limit, resets in $diff\n";
      }
    }

    foreach my $input (@inputs) {
      my $i = $evaluation->jobsetevalinputs->find({ name => $input, altnr => 0 });
      if (! defined $i) {
          print STDERR "Evaluation $evaluation doesn't have input $input\n";
      }
      next unless defined $i;
      my $uri = $i->uri;
      my $rev = $i->revision;
      $uri =~ m![:/]([^/]+)/([^/]+?)(?:.git)?$!;
      $sendStatus->($input, $1, $2, $rev);
    }

}

sub evalStarted {
    my ($self, $traceID, $jobset) = @_;
    #pending
    #$self->common($jobset);
    # link them to https://hydra.novarover.space/jobset/nova/slides
}

sub evalFailed {
    my ($self, $traceID, $jobset, $errorChanged) = @_;
    #$self->common($jobset, $errorChanged);
    # failed
    #
}

sub evalCached {
    my ($self, $traceID, $jobset, $evaluation) = @_;
    $self->common($jobset, $evaluation);
}


# A successful evaluation can still have recorded an error, for the jobs within
# it that did not evaluate, and that is what this mail is most often about.
sub evalAdded {
    my ($self, $traceID, $jobset, $evaluation, $errorChanged) = @_;
    $self->common($jobset, $evaluation);
}


1;
