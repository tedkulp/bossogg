#!/usr/bin/perl

use RPC::XML;
use RPC::XML::Client;
use Term::ReadLine;
use Term::ReadKey;
use Getopt::Long;

$boshellversion = "0.0.2-cvs";
$boshellauthor = "Ted Kulp";

$servername = "localhost";
$portnum = "4069";

$LINES = 25;
$COLS = 80;
eval {
	($COLS, $LINES) = GetTerminalSize();
};
#print "LINES:$LINES\n";
#print "COLS:$COLS\n";

GetOptions (
	'hostname|h:s'	=> \$servername,
	'port|p:s'	=> \$portnum,
	'help|?'	=> \$help
);

if ($help) {
	print "Boss Ogg Shell Client\n";
	print "-hostname -h	Hostname to connect to\n";
	print "-port     -p	Port to connect to\n";
	print "-help     -?	This message\n";
	exit;
}

$url = "http://$servername:$portnum";

$server = RPC::XML::Client->new($url);

#Get Command Set
eval {$status = $server->send_request('system.listMethods')->value } || die "Could not connect to $url\n";
@commandset = @$status;
#foreach my $somecommand (@commandset) {
#	print $somecommand."\n";
#}

#Get Version Number
$result = $server->send_request('util', 'version')->value;
%version = %$result;
#foreach my $somecommand (keys %version) {
#	print $version{$somecommand}."\n";
#}

print "boshell v$boshellversion\n";
print "By $boshellauthor\n";
print "Connected to " . $version{'name'} . " v" . $version{'version'} . "\n\n";

$term = new Term::ReadLine 'boshell';
while ( defined ($_ = $term->readline($servername.":".$portnum."> ")) ) {
	if ($_ =~ /quit/) {
		print "Closing\n";
		exit(0);
	}
	elsif ($_ =~ /help {1,}([A-Za-z]+)/) {
		my $helptext = $server->send_request('system.methodHelp',$1)->value;

		if ($helptext ne undef and !($helptext =~ /HASH/)) {
			print $helptext."\n";
		}
		else {
			print "No help found!\n";
		}
	}
	elsif ($_ =~ /help {0}/) {
		my @newarray;
		push (@newarray,"Available Server Commands");
		push (@newarray,"------------------");
		push (@newarray,@commandset);
		push (@newarray,"");
		push (@newarray,"boshell Commands");
		push (@newarray,"----------------");
		push (@newarray,"help - Get help on a particular server command");
		push (@newarray,"quit - Exit boshell");
		print_list(\@newarray);
	}
	else {
		processcommand($_);
	}
}

sub processcommand {

	my $str = @_[0];
	if ($str =~ /^([A-Za-z_.]+)$/) {
		my $result = $server->send_request($1);
		if ($result ne undef) {
			processresult($1, $result,0);
		}
	}
	elsif ($str =~ /^([A-Za-z_.]+) {1,}(.*)$/) {
		my @theargs = split(/ /, $2);
		my @params;
		foreach $onearg (@theargs) {
			$onearg =~ s/^"//;
			$onearg =~ s/"$//;
			if ($onearg =~ /^[0-9]+$/) {
				push(@params, RPC::XML::int->new($onearg));
			}
			elsif ($onearg =~ /^[0-9.]+$/) {
				push(@params, RPC::XML::double->new($onearg));
			}
			else {
				push(@params, RPC::XML::string->new($onearg));
			}
		}
		my $result = $server->send_request($1, @params);
		if ($result ne undef) {
			processresult($1, $result,0);
		}
	}
	else {
		print "Not a valid command\n";
	}
}

sub processresult {

	my $cmd = @_[0];
	my $result = @_[1];	
	my $recursive = @_[2];
	my $keyname = @_[3];
	#print $result."\n";

	if ($result =~ m/fault/i) {
		my %faulthash = %$result;
		my $thefaultstring = $faulthash{'faultString'};
		print "Fault: ".$$thefaultstring."\n";	
	}
	else {
		if ($cmd eq "listartists") {
			process_listartists($result);
		}
		elsif ($cmd eq "listalbums") {
			process_listalbums($result);
		}
		elsif ($cmd eq "listsongs") {
			process_listsongs($result);
		}
		elsif ($cmd eq "listqueue") {
			process_listqueue($result);
		}
		else {
			if ($result =~ /ARRAY/) {
				#print "Found array...\n";
				for ($i = 0; $i < $recursive; $i++) {
					print "\t";
				}
				print "Array:\n";
				foreach my $thismember (@$result) {
					processresult($cmd, $thismember,$recursive+1);
				}
			}
			elsif ($result =~ /HASH/ and $result =~ /base64/) {
				for ($i = 0; $i < $recursive; $i++) {
					print "\t";
				}
				if ($keyname ne undef) {
					print $keyname.":";
				}
				$scalarvar = $$result{"value"};
				print $scalarvar . "(base64)\n";
			}
			elsif ($result =~ /HASH/) {
				#print "Found hash...\n";
				for ($i = 0; $i < $recursive; $i++) {
					print "\t";
				}
				print "Hash:\n";
				foreach my $thismember (keys %$result) {
					$scalarvar = $$result{$thismember};
					processresult($cmd, $scalarvar,$recursive+1,$thismember);
				}
			}
			elsif ($result =~ /SCALAR/) {
				#print "Found scalar...\n";
				if ($recursive > 0) {
					for ($i = 0; $i < $recursive; $i++) {
						print "\t";
					}
				}
				if ($keyname ne undef) {
					print $keyname.":";
				}
				print $$result . "\n";
			}
		}
	}
}

sub print_list {
	my $dohoriz = 0;
	my $txtarrayref = $_[0];
	my @txtarray = @$txtarrayref;
	if (@_[1] ne undef) {
		$dohoriz = 1;
	}
	if (@txtarray > $LINES) {
		my $totalrows = @txtarray;
		my $currentrow = 0;
		while ($currentrow < $totalrows) {
			for (my $i = 0; $i < $LINES - 2; $i++) {
				if ($currentrow < @txtarray) {
					print $txtarray[$currentrow] . "\n";
					$currentrow++;
				}
			}
			if ($currentrow < @txtarray) {
				print "-- Press any key (q to end)--\n";
				ReadMode 'cbreak';
				my $key = ReadKey(0);
				ReadMode 'normal';
				if ($key eq "q") {
					$currentrow = $totalrows;
				}
			}
		}
	}
	else {
		foreach my $someline (@txtarray) {
			print "$someline\n";
		}
	}
}

sub process_listartists {
	my $result = @_[0];
	my @thearray = @$result;
	my @txtresults;
	foreach my $somehashref (@thearray) {
		my %somehash = %$somehashref;
		my $theaid = $somehash{'aid'};
		my $theartist = $somehash{'artistname'};
		push(@txtresults, sprintf("%6d %s",$$theaid,$$theartist));
	}
	print_list(\@txtresults);
}

sub process_listalbums {
	my $result = @_[0];
	my @thearray = @$result;
	my @txtresults;
	foreach my $somehashref (@thearray) {
		my %somehash = %$somehashref;
		my $thetid = $somehash{'tid'};
		my $thealbum = $somehash{'albumname'};
		my $theyear = $somehash{'year'};
		push(@txtresults, sprintf("%6d %s(%4d)",$$thetid,$$thealbum,$$theyear));
	}
	print_list(\@txtresults);
}

sub process_listsongs {
	my $result = @_[0];
	my @thearray = @$result;
	my @txtresults;
	foreach my $somehashref (@thearray) {
		my %somehash = %$somehashref;
		my $thesid = $somehash{'sid'};
		my $thesong = $somehash{'songname'};
		my $thetrack = $somehash{'track'};
		my $thesec = $somehash{'sec'};
		push(@txtresults, sprintf("%6d %2d. %s(%d)",$$thesid,$$thetrack,$$thesong,$$thesec));
	}
	print_list(\@txtresults);
}

sub process_listqueue {
	my $result = @_[0];
	my @thearray = @$result;
	my @txtresults;
	push(@txtresults, sprintf("index    sid ##  songname",$$theindex,$$thesid,$$thetrack,$$thesong));
	foreach my $somehashref (@thearray) {
		my %somehash = %$somehashref;
		my $theindex = $somehash{'index'};
		my $thesid = $somehash{'sid'};
		my $thesong = $somehash{'songname'};
		my $theartist = $somehash{'artist'};
		my $thetrack = $somehash{'track'};
		push(@txtresults, sprintf("%5d %6d %2d. %s - %s",$$theindex,$$thesid,$$thetrack,$$thesong,$$theartist));
	}
	print_list(\@txtresults);
}
