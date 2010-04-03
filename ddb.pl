#!/usr/bin/perl -w
#
# Disc DataBase

use strict;

#use DBI;
use DBI qw(:sql_types);
use File::Find;
use Getopt::Long;
use Pod::Usage;

# Configuration
my $config_file = $ENV{"HOME"} . "/.ddbrc";

# Flags
my $verbose = 1;
my $directory = "";
my $add = "";
my $remove = "";
my $list = "";
my $help = "";
my @filenames = ();
my @records = ();
my %results = ();
my %config = ();
my $dbh = "";

# Queries
my $search_query = "SELECT disc,directory,file FROM ddb WHERE %s LIKE ?";
my $is_in_query = "SELECT DISTINCT disc FROM ddb WHERE disc=?";
my $list_disc_query = "SELECT DISTINCT disc FROM ddb";
my $list_file_query = "SELECT directory,file FROM ddb WHERE disc LIKE ?";
my $list_dir_query = "SELECT directory FROM ddb WHERE disc LIKE ?";
my $delete_query = "DELETE FROM ddb WHERE disc=?";
my $add_query = "INSERT INTO ddb (directory, file, disc) VALUES (?, ?, ?)";
my $optimize_query = "OPTIMIZE TABLE ddb";
my $transaction_begin_query = "BEGIN";
my $transaction_end_query = "COMMIT";

sub read_config
{
  open(CONFIG, "<$config_file")
    or &setup_config();

  while(<CONFIG>)
  {
    chomp;     # Taken from Perl Cookbook
    s/#.*//;
    s/^\s+//;
    s/\s+$//;
    next unless length;
    my ($key, $value) = split(/\s*=\s*/, $_, 2);
    $config{$key} = $value;
  }
  close(CONFIG)
    or die "Could not close config file $config_file: $!\n";
}

sub setup_config
{
  print "You do not seem to have the config file\n";
  print "Please type the DSN: ";
  chomp($config{"DSN"} = <STDIN>);
  print "Please type the username: ";
  chomp($config{"username"} = <STDIN>);
  print "Please type the password: ";

#  eval "require Term::ReadKey";
#
#  if($@ eq "")
#  {
#    use Term::ReadKey;
#
#    my $password1 = "";
#    my $password2 = "";
#
#    ReadMode 'noecho';
#    $password1 = ReadLine(0);
#    print "\nPlease type the password again: ";
#    $password2 = ReadLine(0);
#    ReadMode 'normal';
#
#    if($password1 eq $password2)
#    {
#      chomp($config{"password"} = $password1);
#    }
#    else
#    {
#      die "Wrong password!\n";
#    }
#  }
#  else
#  {
    warn "Password will be visible!\n";
    chomp($config{"password"} = <STDIN>);
#  }

  open(CONFIG, ">$config_file")
    or die "Could not open config file $config_file: $!\n";

  foreach $_ (keys(%config))
  {
    print CONFIG "$_ = " . $config{$_} . "\n";
  }

  close(CONFIG)
    or die "Could not close config file $config_file: $!\n";

  print "\nNow restart the program.\n";
  exit;
}

sub print_results
{
  foreach $_ (sort(keys(%results)))
  {
    print $_ . "\n";
  }
}

sub search_file # 0. arg: file or directory to search
{
  my $sth;

  if($directory)
  {
    $sth = $dbh -> prepare(sprintf($search_query, "directory"));
  }
  else
  {
    $sth = $dbh -> prepare(sprintf($search_query, "file"));
  }

  $sth -> bind_param(1, "%" . $_[0] . "%", {TYPE => SQL_VARCHAR});
  $sth -> execute();

  while(my($disc, $dir, $file) = $sth -> fetchrow_array())
  {
    $results{$disc . ": " . $dir . "/" . ($directory ? "" : $file)} = "";
  }

  print_results();
}

sub add_dir     # 0. arg: directory to add as a disc
{
  my $sth = $dbh -> prepare($is_in_query);
  my $found;

  $sth -> bind_param(1, "$add", {TYPE => SQL_VARCHAR});
  $sth -> execute();
  $found = $sth -> fetchrow_array();
  $sth -> finish();

  if((defined $found) and ($add eq $found))
  {
    print "Disc " . $add . " already in database!\n";
    exit(1);
  }

  find({wanted => \&check_file, follow => 0, no_chdir => 0}, $_[0]);
}

sub check_file  # 0. arg: the file to check
{
  if(-d $File::Find::name)
  {
    push(@records, $File::Find::name);
    push(@records, "");
  }
  else
  {
    push(@records, $File::Find::dir);
    push(@records, $_);
  }
}

sub list_disc   # 0. arg: disc or directory to list
{
  my $list_query = $list_disc_query;
  my $sth;

  if(defined($_))
  {
    if(!$directory)
    {
      $list_query = $list_file_query;
    }
    else
    {
      $list_query = $list_dir_query;
    }
  }

  $sth = $dbh -> prepare($list_query);

  if(defined($_))
  {
    $sth -> bind_param(1, "%" . $_ . "%", {TYPE => SQL_VARCHAR});
  }

  $sth -> execute();

  if(!defined($_))
  {
    while(my $disc = $sth -> fetchrow_array())
    {
      $results{$disc} = "";
    }
  }
  else
  {
    if(!$directory)
    {
      while(my($dir, $file) = $sth -> fetchrow_array())
      {
        $results{$dir . "/" . (defined($file) ? $file : "")} = "";
      }
    }
    else
    {
      while(my $dir = $sth -> fetchrow_array())
      {
        $results{$dir . "/"} = "";
      }
    }
  }

  print_results();
}

sub remove_disc
{
  my $sth;

  verbosity(2, "Removing disc from database...");
  $sth = $dbh -> prepare($delete_query);
  $sth -> bind_param(1, "$remove", {TYPE => SQL_VARCHAR});
  $sth -> execute();
  verbosity(3, "Done.");

#  verbosity(2, "Optimizing table...");
#  $sth = $dbh -> do($optimize_query);
#  verbosity(3, "Done.");
}

sub verbosity   # 0. arg: minimal verbosity, 1. arg: message
{
  if($verbose >= $_[0])
  {
    print $_[1] . "\n";
  }
}

GetOptions("verbose+"   => \$verbose,
           "quiet!"     => sub { $verbose-- },
           "directory"  => \$directory,
           "add=s"      => \$add,
           "remove=s"   => \$remove,
           "list"       => \$list,
           "help|?"     => \$help);

# Read config
&read_config();

# DB connect
verbosity(2, "Opening database...");
$dbh = DBI -> connect($config{"DSN"}, $config{"username"}, $config{"password"},
                      {PrintError => 0, RaiseError => 1});
verbosity(3, "Done.");

if(scalar(@ARGV) == 0 && !$remove || $help)
{
  if($list)
  {
    list_disc(undef);
    exit(0);
  }

  pod2usage();
}
elsif($remove)
{
  remove_disc();
  exit(0);
}
else
{
  @filenames = @ARGV;
}

foreach (@filenames)
{
  if(scalar(@filenames) > 1)
  {
    verbosity(1, "$_:\n");
  }

  if($add)
  {
    add_dir($_);
  }
  elsif($list)
  {
    list_disc($_);
  }
  else
  {
    search_file($_);
  }
}

if($add)
{
  my $sth;
  my ($key,$value);

  verbosity(2, "Inserting files into the database...");
  $sth = $dbh -> do($transaction_begin_query);
  $sth = $dbh -> prepare($add_query);
  while(scalar(@records) > 0)
  {
    $key = shift(@records);
    $value = shift(@records);
    $sth -> execute($key, $value, $add);
  }
  $sth = $dbh -> do($transaction_end_query);
  verbosity(3, "Done.");
  
#  verbosity(2, "Optimizing table...");
#  $sth = $dbh -> do($optimize_query);
#  verbosity(3, "Done.");
}

verbosity(2, "Closing database...");
$dbh -> disconnect();
verbosity(3, "Done.");


__END__

=head1 NAME

ddb - Disc DataBase

=head1 SYNOPSIS

ddb [options] [file ...]

  Options:
    No options                  Search file(s).
    -a, --add title             Adds disc to the database.
    -d, --directory             Search directory.
    -r, --remove title          Removes disc from database.
    -l, --list                  Lists the given disc or directory.
    -h, -?, --help              Prints this help message.
    -v, --verbose               Increase verbosity.
    -q, --quiet                 Decrease verbosity.

=head1 OPTIONS

=over 8

=item B<-a, --add title>

Add contents of the disc using the given title to the database.
File(s) is treated as a directory on which the disc is mounted.
(With MySQL it optimizes the table after each addition).

=item B<-d, --directory>

Treat file(s) as a directory.

=item B<-r, --remove title>

Remove records for a given disc from the database.
(With MySQL it optimizes the table after each deletion).

=item B<-l, --list>

List discs in the database or the contents of the disc.
File(s) is treated as a disc name.

=item B<-h, -?, --help>

Prints the help message and exits.

=item B<-v, --verbose>

Increases verbosity.

=item B<-q, --quiet>

Decreases verbosity.

=back

=head1 DESCRIPTION

B<ddb> stores filenames (for example) from a CD-ROM into a database and searches
for files or directories in this database.

The SQL structure of the database is (for MySQL, done as admin):

  CREATE DATABASE ddb;
  CREATE TABLE ddb (directory TINYBLOB NOT NULL, file TINYBLOB,
                    disc TINYBLOB NOT NULL);
  GRANT INSERT, SELECT, DELETE ON ddb.ddb TO user;

=head1 RETURN CODES

B<ddb> has the following return codes:

=over 8

=item C<0>

Success.

=item C<1>

Disc already in database.

=cut

