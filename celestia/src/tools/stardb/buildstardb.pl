#!/usr/bin/perl

# buildstardb.pl
#
# Builds the stars.dat and revised.stc files for Celestia
# For usage instructions, run with the -? command line switch
#
# Version 1.0 - Andrew Tribick (2008-07-17)

# This code makes use of the following datasets:
#
# ESA, 1997, The Hipparcos Catalogue, ESA SP-1200
#   available at http://cdsarc.u-strasbg.fr/viz-bin/Cat?I/239
#
# Hipparcos, the new Reduction of the Raw data
#   Floor van Leeuwen, 2007 "Hipparcos, the New Reduction of the Raw Data"
#   Astrophysics & Space Science Library #350.
#   available at http://cdsarc.u-strasbg.fr/viz-bin/Cat?I/311


use Math::Trig;

# default file paths
$HIP_PATH  = 'hip_main.dat';
$HIP2_PATH = 'hip2.dat';
$DAT_PATH  = 'stars.dat';
$TXT_PATH  = 'stars.txt';

# default drop level
# 0 - fix bad parallaxes; 1 - fix bad parallaxes for bright stars;
# 2 - don't fix parallaxes; 3 - strict drop criteria
$DROP_LEVEL = 1;

# by default turn spectral type guesser on
$GUESS_TYPES = 1;

# some physical/astronomical constants
$LY_PER_PARSEC = 3.26167; # taken from astro.h
$J2000Obliquity = deg2rad(23.4392911);
$cc = cos($J2000Obliquity);
$ss = sin($J2000Obliquity);
@eqToCel = (
	[1,   0,    0],
	[0, $cc, -$ss],
	[0, $ss,  $cc]
);

# B-V magnitudes and spectral types
# from Lang, K.  "Astrophysical Data: Planets and Stars"(1991)
%SpBV = (
	'O5' => -0.33,
	'O8' => -0.32,
	'O9' => -0.31,
	'B0' => -0.30,
	'B1' => -0.265,
	'B2' => -0.24,
	'B3' => -0.205,
	'B5' => -0.17,
	'B6' => -0.15,
	'B7' => -0.135,
	'B8' => -0.11,
	'B9' => -0.075,
	'A0' => -0.02,
	'A1' => 0.01,
	'A2' => 0.05,
	'A3' => 0.08,
	'A5' => 0.15,
	'A7' => 0.20,
	'A8' => 0.25,
	'F0' => 0.30,
	'F2' => 0.35,
	'F5' => 0.44,
	'F8' => 0.52,
	'G0' => 0.58,
	'G2' => 0.63,
	'G5' => 0.68,
	'G8' => 0.74,
	'K0' => 0.81,
	'K1' => 0.86,
	'K2' => 0.91,
	'K3' => 0.96,
	'K5' => 1.15,
	'K7' => 1.33,
	'M0' => 1.40,
	'M1' => 1.46,
	'M2' => 1.49,
	'M3' => 1.51,
	'M4' => 1.54,
	'M5' => 1.64,
	'M6' => 1.73,
	'M7' => 1.80,
	'M8' => 1.93
);

# digit meanings for SpectralClass data type
%SC_StarType = (
	'NormalStar' => 0x0000,
	'WhiteDwarf' => 0x1000,
	'NeutronStar' => 0x2000,
	'BlackHole' => 0x3000,
	'Mask' => 0xf000
);
%SC_SpecClass = (
	'O' => 0x0000,
	'B' => 0x0100,
	'A' => 0x0200,
	'F' => 0x0300,
	'G' => 0x0400,
	'K' => 0x0500,
	'M' => 0x0600,
	'R' => 0x0700,
	'S' => 0x0800,
	'N' => 0x0900,
	'WC' => 0x0a00,
	'WN' => 0x0b00,
	'?' => 0x0c00,
	'L' => 0x0d00,
	'T' => 0x0e00,
	'C' => 0x0f00,
	'DA' => 0x0000,
	'DB' => 0x0100,
	'DC' => 0x0200,
	'DO' => 0x0300,
	'DQ' => 0x0400,
	'DZ' => 0x0500,
	'D' => 0x0600,
	'DX' => 0x0700,
	'Mask' => 0x0f00
);
%SC_Subclass = (
	'0' => 0x0000,
	'1' => 0x0010,
	'2' => 0x0020,
	'3' => 0x0030,
	'4' => 0x0040,
	'5' => 0x0050,
	'6' => 0x0060,
	'7' => 0x0070,
	'8' => 0x0080,
	'9' => 0x0090,
	'?' => 0x00a0,
	'Mask' => 0x00f0
);
%SC_LumClass = (
	'Ia0' => 0x0000,
	'Ia' => 0x0001,
	'Ib' => 0x0002,
	'II' => 0x0003,
	'III' => 0x0004,
	'IV' => 0x0005,
	'V' => 0x0006,
	'VI' => 0x0007,
	'?' => 0x0008,
	'Mask' => 0x000f
);

# data stored in these arrays
%stars = (); # star details
%m_sys = (); # multiple star system details

# parse command line
$i = 0;
while($i <= $#ARGV) {
	if($ARGV[$i] =~ m/^\-d/) {
		# parameter is DROP_LEVEL
		if(length($ARGV[$i]) > 2) {
			$DROP_LEVEL = int(substr($ARGV[$i], 2));
		} elsif($i < $#ARGV) {
			$i++;
			$DROP_LEVEL = int($ARGV[$i]);
		}
		# if invalid option given, use default
		if(($DROP_LEVEL < 0) || ($DROP_LEVEL > 3)) {
			$DROP_LEVEL = 1;
		}
	} elsif($ARGV[$i] =~ m/^\-s/) {
		# parameter specifies whether to guess spectral types
		if(length($ARGV[$i]) > 2) {
			$GUESS_TYPES = int(substr($ARGV[$i], 2));
		} elsif($i < $#ARGV) {
			$i++;
			$GUESS_TYPES = int($ARGV[$i]);
		}
		# if invalid option given, use default
		if(($GUESS_TYPES < 0) || ($GUESS_TYPES > 1)) {
			$GUESS_TYPES = 1;
		}
	} elsif($ARGV[$i] eq '--fix-all') {
		$DROP_LEVEL = 0;
	} elsif($ARGV[$i] eq '--fix-bright') {
		$DROP_LEVEL = 1;
	} elsif($ARGV[$i] eq '--no-fix') {
		$DROP_LEVEL = 2;
	} elsif($ARGV[$i] eq '--use-strict') {
		$DROP_LEVEL = 3;
	} elsif($ARGV[$i] eq '--guess-types') {
		$GUESS_TYPES = 2;
	} elsif($ARGV[$i] eq '--revised-types') {
		$GUESS_TYPES = 1;
	} elsif($ARGV[$i] eq '--no-types') {
		$GUESS_TYPES = 0;
	} elsif($ARGV[$i] =~ m/^\-\-hip\-file=(.*)$/) {
		$HIP_PATH = $1;
	} elsif($ARGV[$i] =~ m/^\-\-hip2\-file=(.*)$/) {
		$HIP2_PATH = $1;
	} elsif($ARGV[$i] =~ m/^\-\-dat\-file=(.*)$/) {
		$DAT_PATH = $1;
	} elsif($ARGV[$i] =~ m/^\-\-txt\-file=(.*)$/) {
		$TXT_PATH = $1;
	} elsif(($ARGV[$i] =~ m/^\-\?/) || ($ARGV[$i] eq '--help')) {
		# print program help information and exit
		print "buldstardb.pl v0.4 - Builds the Celestia star database and revised.stc\n\n";
		print "Usage: buildstardb.pl [-?] [-d DROPLEVEL] [-s SPECTRA]\n";
		print "                      [--hip-file=FILE] [--hip2-file=FILE] [--dat-file=FILE]\n";
		print "                      [--txt-file=FILE]\n";
		print "Optional arguments:\n";
		print "  -?, --help       displays this help page and exit.\n";
		print "  -d DROPLEVEL     set acceptance criteria and whether to fix dubious parallax\n";
		print "                     measurements. -d0 fixes all dubious parallaxes. -d1 only\n";
		print "                     fixes bright star parallaxes. -d2 does not fix any\n";
		print "                     parallaxes. -d3 uses stricter acceptance criteria for\n";
		print "                     stars with large measurement uncertainties.\n";
		print "  -s SPECTRA       set when spectral type guessing should be used.\n";
		print "                     -s0 prevents spectral type guessing. -s1 enables spectral\n";
		print "                     type guessing.\n";
		print "  --hip-file=FILE  specify Hipparcos catalog file.\n";
		print "  --hip2-file=FILE specify Hipparcos, the New Reduction Astrometric Catalogue\n";
		print "                     file.\n";
		print "  --dat-file=FILE  specify binary star database output.\n\n";
		print "  --txt-file=FILE  specify text star database output.\n\n";
		print "  --fix-all        fix dubious parallaxes for all stars. Equivalent to -d0.\n";
		print "  --fix-bright     fix dubious parallaxes for bright stars. Equivalent to -d1\n";
		print "  --no-fix         do not fix dubious parallaxes. Equivalent to -d2\n";
		print "  --use-strict     use strict acceptance criteria. Equivalent to -d3\n\n";
		print "  --no-types       do not guess spectral types. Equivalent to -s0\n";
		print "  --guess-types    put spectral type guesses in stars.dat. Equivalent to -s1\n\n";
		print "Running with no options is equivalent to using -d1 -s1 and searches for the\n";
		print "datafiles hip_main.dat and hip2.dat in the current directory, and the\n";
		print "output files will be stars.dat and stars.txt in the current directory.\n\n";
		print "The hip_main.dat file can be downloaded from CDS at\n";
		print "http://cdsarc.u-strasbg.fr/viz-bin/Cat?I/239\n";
		print "The hip2.dat file can be downloaded from CDS at\n";
		print "http://cdsarc.u-strasbg.fr/viz-bin/Cat?I/311\n";
		exit(0);
	}
	$i++;
}

ReadHipparcos();
ReadOldHipparcos();
FixData();
CheckStars();
WriteDat();

# ---------------------------- END OF MAIN PROGRAM --------------------------- #

# --------------------------- INPUT/OUTPUT FUNCTIONS ------------------------- #

# Read the Astrometric Catalogue into associative array
sub ReadHipparcos {
	print "Reading Astrometric Catalog...\n";

	local(*HIPFILE);
	if(!open(HIPFILE, '<', $HIP2_PATH)) {
		print "  ERROR: Could not open $HIP2_PATH\n";
		return;
	}

	my $numStars = 0;
	while(my $curLine = <HIPFILE>) {
		chomp $curLine;

		my $HIP = Trim(substr($curLine, 0, 6));
		# note all entries are inserted into list in case subsequent
		# processing inserts missing properties.
		my %star = (
			'RArad'    => substr($curLine,  15, 13),
			'DErad'    => substr($curLine,  29, 13),
			'Plx'      => substr($curLine,  43,  7),
			'e_RArad'  => substr($curLine,  69,  6),
			'e_DErad'  => substr($curLine,  76,  6),
			'e_Plx'    => substr($curLine,  83,  6),
			'Hpmag'    => substr($curLine, 129,  7),
			'B-V'      => substr($curLine, 152,  6)
		);

		# strip whitespace from values
		foreach my $key (keys %{$stars{$HIP}}) {
			$stars{$HIP}{$key} =~ s/\s//g;
		}
		
		# add data
		$stars{$HIP} = {
			'RArad'   => $star{'RArad'},
			'DErad'   => $star{'DErad'},
			'RAhms'   => '',
			'DEdms'   => '',
			'Plx'     => $star{'Plx'},
			'e_RArad' => $star{'e_RArad'},
			'e_DErad' => $star{'e_DErad'},
			'e_Plx'   => $star{'e_Plx'},
			'BTmag'   => '',
			'VTmag'   => '',
			'Hpmag'   => $star{'Hpmag'},
			'B-V'     => $star{'B-V'}
		};

		$numStars++;
	}
	close(HIPFILE);
	
	print "  Read a total of $numStars records.\n";
}

# Read Hipparcos Main Catalog to get Vmag, BTmag, VTmag, SpType
# which are not present in the new revision
sub ReadOldHipparcos {
	print "Reading Hipparcos Main Catalog...\n";

	local(*HIPFILE);
	if(!open(HIPFILE, '<', $HIP_PATH)) {
		print "  ERROR: Could not open $HIP_PATH\n";
		return;
	}

	my $numStars = 0;
	while(my $curLine = <HIPFILE>) {
		chomp $curLine;

		# check that this is hip_main.dat
		die "ERROR: Bad catalog format in $HIP_PATH\n" if(substr($curLine, 0, 1) ne 'H');

		my $HIP = Trim(substr($curLine, 8, 6));
		if(exists $stars{$HIP}) {
			# add values into entry
			$stars{$HIP}{'Vmag'}   = Trim(substr($curLine,  41,  5));
			$stars{$HIP}{'BTmag'}  = Trim(substr($curLine, 217,  6));
			$stars{$HIP}{'VTmag'}  = Trim(substr($curLine, 230,  6));
			$stars{$HIP}{'SpType'} = Trim(substr($curLine, 435, 12));
			# terminate SpType at first space
			$stars{$HIP}{'SpType'} =~ m/^([^\s]*)/;
			$stars{$HIP}{'SpType'} = $1;
		}

		# increment tally
		$numStars++;
	}
	close(HIPFILE);
	
	print "  Read a total of $numStars records.\n";
}

sub WriteDat {
	my $numStars = keys %stars;
	print "Writing databases...\n";

	print "  Writing binary database to $DAT_PATH\n";
	local(*DATFILE);
	open(DATFILE, '>', $DAT_PATH) or die "ERROR: Could not write to $DAT_PATH\n";
	binmode(DATFILE);
	
	print "  Writing text database to $TXT_PATH\n";
	local(*TXTFILE);
	open(TXTFILE, '>', $TXT_PATH) or die "ERROR: Could not write to $TXT_PATH\n";
	
	# write file header
	print DATFILE pack('a8ccL', 'CELSTARS', 0, 1, $numStars);
	print TXTFILE sprintf("%u\n", $numStars);
	
	# write each star
	foreach $HIP (sort { $a <=> $b } keys %stars) {
		my $dist = PlxToDistance($stars{$HIP}{'Plx'});
		my $theta = $stars{$HIP}{'RArad'} + pi;
		my $phi = $stars{$HIP}{'DErad'} - pi / 2;
		my @xyz = (
			 $dist * cos($theta) * sin($phi),
			 $dist * cos($phi),
			-$dist * sin($theta) * sin($phi)
		);
		my $xc = $eqToCel[0][0] * $xyz[0] + $eqToCel[1][0] * $xyz[1] + $eqToCel[2][0] * $xyz[2];
		my $yc = $eqToCel[0][1] * $xyz[0] + $eqToCel[1][1] * $xyz[1] + $eqToCel[2][1] * $xyz[2];
		my $zc = $eqToCel[0][2] * $xyz[0] + $eqToCel[1][2] * $xyz[1] + $eqToCel[2][2] * $xyz[2];
		my $absMag = AppMagToAbsMag($stars{$HIP}{'Vmag'}, $stars{$HIP}{'Plx'});
		my $spType = ParseSpType($stars{$HIP}{'SpType'});
		print DATFILE pack('LfffsS', $HIP, $xc, $yc, $zc, $absMag * 256, $spType);
		print TXTFILE sprintf("%u  %.9f %+.9f %.6g %.2f %s\n", $HIP,
		                      rad2deg($stars{$HIP}{'RArad'}), rad2deg($stars{$HIP}{'DErad'}),
							  $dist, $stars{$HIP}{'Vmag'}, $stars{$HIP}{'SpType'});
	}
	
	close(DATFILE);
	close(TXTFILE);
	
	print "  Wrote a total of $numStars stars.\n";
}

# -------------------------- DATA HANDLING ROUTINES -------------------------- #

# fix missing data
sub FixData {
	print "Fixing data...\n";
	foreach my $HIP (keys %stars) {
		my $Bt = $stars{$HIP}{'BTmag'};
		my $Vt = $stars{$HIP}{'VTmag'};
		my $Hpmag = $stars{$HIP}{'Hpmag'};
		my $BtVt = '';
		my $BtVt = $Bt - $Vt if(($Bt ne '') && ($Vt ne ''));

		# check if Hipparcos measured position - otherwise use the hms format
		if(($stars{$HIP}{'RArad'} eq '') || ($stars{$HIP}{'DErad'} eq '')) {
			my $RAstr = $stars{$HIP}{'RAhms'};
			my $DEstr = $stars{$HIP}{'DEdms'};
			if(($RAstr ne '') && ($DEstr ne '')) {
				my @RAhms = split(/\s+/, $RAstr);
				my @DEdms = split(/\s+/, $DEstr);
				$stars{$HIP}{'RArad'} = ($RAhms[0]*15 + $RAhms[1]/4  + $RAhms[2]/240) * pi / 180;
				$stars{$HIP}{'DErad'} = ($DEdms[0]    + $DEdms[1]/60 + $DEdms[2]/3600) * pi / 180;
				# set large error
				$stars{$HIP}{'e_RArad'} = 1000;
				$stars{$HIP}{'e_DErad'} = 1000;
			}
		}

		# if Vmag missing, calculate from Bt and Vt magnitudes or Hpmag
		if(($stars{$HIP}{'Vmag'} eq '') && ($Vt ne '')) {
			if($BtVt eq '') {
				$stars{$HIP}{'Vmag'} = VtToVmag($Vt, 0);
			} else {
				$stars{$HIP}{'Vmag'} = VtToVmag($Vt, $BtVt);
			}
		} elsif(($stars{$HIP}{'Vmag'} eq '') && ($Hpmag ne '')) {
			$stars{$HIP}{'Vmag'} = HpToVmag($Hpmag, 0);
		}

		# if B-V missing, calculate from Bt and Vt magnitudes
		$stars{$HIP}{'B-V'} = BtVtToBV($BtVt) if(($stars{$HIP}{'B-V'} eq '') && ($BtVt ne ''));
		
		# if star has unknown spectral type, attempt to guess from B-V
		if((SpTypeToString(ParseSpType($stars{$HIP}{'SpType'})) eq '?') && ($GUESS_TYPES == 1)) {
			$stars{$HIP}{'SpType'} = GuessSpType($stars{$HIP}{'B-V'}) if($stars{$HIP}{'B-V'} ne '');
		}
		
		# fix parallaxes
		# if no measured parallax, leave alone (star not measured by Hipparcos)
		# bright stars (Vmag <= 6) affected are XI UMa, XI Sco and HIP 115125
		if($stars{$HIP}{'Plx'} ne '') {
			# if DROP_LEVEL == 0, fix all bad or suspect parallaxes
			# if DROP_LEVEL == 1, fix bad or suspect parallaxes for bright stars
			if(($stars{$HIP}{'Plx'} < 0.4) && (($DROP_LEVEL == 0) || (($DROP_LEVEL == 1) && ($stars{$HIP}{'Vmag'} <= 6)))) {
				$stars{$HIP}{'Plx'} = 0.4;
				$stars{$HIP}{'e_Plx'} = 1000;
			}
		}
	}
	print "  Fixed.\n";
}

# drop stars with bad data
sub CheckStars {
	print "Checking data...\n";
	my $good = 0;
	my $dubious = 0;
	my $dropped = 0;
	my $brightdrop = 0;
	foreach my $HIP (keys %stars) {
		my $badness = TestDubious($stars{$HIP});
		if($badness <= 3) {
			# good stars are fine
			$good++;
		} elsif(($DROP_LEVEL < 3) && (($badness <= 6) || (($badness < 100) && (($DROP_LEVEL == 0) || ($stars{$HIP}{'Vmag'} <= 6))))) {
			# star is dubious but included
			$dubious++;
		} else {
			# drop star
			$brightdrop++ if(($stars{$HIP}{'Vmag'} ne '') && ($stars{$HIP}{'Vmag'} <= 6));
			delete $stars{$HIP};
			$dropped++;
		}
	}
	print "  $good stars with good data included.\n";
	print "  $dubious stars with dubious data included.\n";
	print "  $dropped stars dropped, of which $brightdrop are bright stars.\n";
}

# dubiousness function based on HipparcosStar::analyze() in buildstardb.cpp
sub TestDubious {
	my $star = shift;
	my $dubious = 0;
	
	# if there is no magnitude information, we can't use this star
	$dubious += 100 if($star->{'Vmag'} eq '');
	
	# test parallax
	if(($star->{'Plx'} eq '') || ($star->{'Plx'} <= 0)) {
		#  negative or missing parallax - reject
		$dubious += 100;
	} elsif($star->{'Plx'} < 0.2) {
		# low parallax - dubious
		$dubious += 4;
	} elsif($star->{'Plx'} < 0.4) {
		# low parallax - slightly dubious
		$dubious += 2;
	}
	if($star->{'Plx'} <= $star->{'e_Plx'}) {
		# parallax value less than error - treat as dubious
		$dubious += 2;
	} elsif($star->{'Plx'} <= (2*$star->{'e_Plx'})) {
		# parallax within factor of 2 of error - slightly dubious
		$dubious += 1;
	}
	
	# if no position information, reject the star
	$dubious += 100 if(($star->{'RArad'} eq '') || ($star->{'DErad'} eq ''));
	# test for large position errors
	if($star->{'e_RArad'} >= 1000) {
		# this situation corresponds to an hms value - dubious
		$dubious += 4;
	} else {
		my $e_RADec = sqrt($star->{'e_RArad'} ** 2 + $star->{'e_DErad'} ** 2);
		if($e_RADec > 75) {
			# very large position error - reject
			$dubious += 100;
		} elsif($e_RADec > 40) {
			# large position error -  dubious
			$dubious += 2;
		} elsif($e_RADec > 20) {
			# large position error - slightly dubious
			$dubious += 1;
		}
	}
	return $dubious;
}

# ------------------------ ASTROPHYSICAL CALCULATIONS ------------------------ #

# convert apparent magnitude to absolute magnitude using parallax
sub AppMagToAbsMag {
	my $appMag = shift;
	my $plx = shift;
	return $appMag - 5 * Log10(100 / $plx);
}

# convert parallax to distance in light years
sub PlxToDistance {
	my $plx = shift;
	return 1000/$plx * $LY_PER_PARSEC;
}

# --------------------- MAGNITUDE SYSTEM TRANSFORMATIONS --------------------- #

# convert Vt magnitude to Vmag
# from Mamajek, Meyer & Liebert (2002), AJ 124 (3), 1670-1694
sub VtToVmag {
	my $Vt = shift;
	my $BtVt = shift;
	return $Vt + 9.7e-04 - 1.334e-01 * $BtVt + 5.486e-02 * $BtVt * $BtVt - 1.998e-02 * $BtVt * $BtVt * $BtVt;
}

# convert Hp magnitude to Vmag
# based on cubic polynomial fit to data in Bessel, M.S (2000), PASP 112, 961-965
sub HpToVmag {
	my $Hp = shift;
	my $BtVt = shift;
	return $Hp - 7.967e-03 - 2.537e-01 * $BtVt + 1.073e-01 * $BtVt * $BtVt - 2.678e-03 * $BtVt * $BtVt * $BtVt;
}

# convert Bt-Vt to B-V
# from Mamajek, Meyer & Liebert (2002), AJ 124 (3), 1670-1694
sub BtVtToBV {
	my $BtVt = shift;
	my $BV = $BtVt - 7.813e-03 * $BtVt - 1.489e-01 * $BtVt * $BtVt + 3.384e-02 * $BtVt * $BtVt * $BtVt;
	return $BV;
}

# ------------------------- SPECTRAL CLASS HANDLING -------------------------- #

# Implements the stellar class parser from stellarclass.cpp
sub ParseSpType {
	my $st = shift;
	$st =~ s/\s//g;

	$st = '?' if($st eq '');
	
	my $i = 0;
	my $state = 'BeginState';

	my $starType = $SC_StarType{'NormalStar'};
	my $specClass = $SC_SpecClass{'?'};
	my $subclass = $SC_Subclass{'?'};
	my $lumClass = $SC_LumClass{'?'};

	while($state ne 'EndState') {
		my $c = ($i < length($st)) ? substr($st, $i, 1) : '';
		if($state eq 'BeginState') {
			if($c eq 'Q') {
				$starType = $SC_StarType{'NeutronStar'};
				$state = 'EndState';
			} elsif($c eq 'X') {
				$starType = $SC_StarType{'BlackHole'};
				$state = 'EndState';
			} elsif($c eq 'D') {
				$starType = $SC_StarType{'WhiteDwarf'};
				$specClass = $SC_SpecClass{'D'};
				$state = 'WDTypeState';
				$i++;
			} elsif($c eq 's') {
				$state = 'SubdwarfPrefixState';
				$i++;
			} elsif($c eq '?') {
				$state = 'EndState';
			} else {
				$state = 'NormalStarClassState';
			}
		} elsif($state eq 'WolfRayetTypeState') {
			if($c =~ m/[CN]/) {
				$specClass = $SC_SpecClass{'W'.$c};
				$state = 'NormalStarSubclassState';
				$i++;
			} else {
				$specClass = $SC_SpecClass{'WC'};
				$state = 'NormalStarSubclassState';
				$i++;
			}
		} elsif($state eq 'SubdwarfPrefixState') {
			if($c eq 'd') {
				$lumClass = $SC_LumClass{'VI'};
				$state = 'NormalStarClassState';
				$i++;
			} else {
				$state = 'EndState';
			}
		} elsif($state eq 'NormalStarClassState') {
			if($c eq 'W') {
				$state = 'WolfRayetTypeState';
			} elsif($c =~ m/[OBAFGKMRSNLTC]/) {
				$specClass = $SC_SpecClass{$c};
				$state = 'NormalStarSubclassState';
			} else {
				$state = 'EndState';
			}
			$i++;
		} elsif($state eq 'NormalStarSubclassState') {
			if($c =~ m/[0-9]/) {
				$subclass = $SC_Subclass{$c};
				$state = 'NormalStarSubclassDecimalState';
				$i++;
			} else {
				$state = 'LumClassBeginState';
			}
		} elsif($state eq 'NormalStarSubclassDecimalState') {
			if($c eq '.') {
				$state = 'NormalStarSubclassFinalState';
				$i++;
			} else {
				$state = 'LumClassBeginState';
			}
		} elsif($state eq 'NormalStarSubclassFinalState') {
			if($c =~ m/[0-9]/) {
				$state = 'LumClassBeginState';
			} else {
				$state = 'EndState';
			}
			$i++;
		} elsif($state eq 'LumClassBeginState') {
			if($c eq 'I') {
				$state = 'LumClassIState';
			} elsif($c eq 'V') {
				$state = 'LumClassVState';
			} else {
				$state = 'EndState';
			}
			$i++;
		} elsif($state eq 'LumClassIState') {
			if($c eq 'I') {
				$state = 'LumClassIIState';
			} elsif($c eq 'V') {
				$lumClass = $SC_LumClass{'V'};
				$state = 'EndState';
			} elsif($c eq 'a') {
				$state = 'LumClassIaState';
			} elsif($c eq 'b') {
				$lumClass = $SC_LumClass{'Ib'};
				$state = 'EndState';
			} elsif($c eq '-') {
				$state = 'LumClassIdashState';
			} else {
				$lumClass = $SC_LumClass{'Ib'};
				$state = 'EndState';
			}
			$i++;
		} elsif($state eq 'LumClassIIState') {
			if($c eq 'I') {
				$lumClass = $SC_LumClass{'III'};
				$state = 'EndState';
			} else {
				$lumClass = $SC_LumClass{'II'};
				$state = 'EndState';
			}
		} elsif($state eq 'LumClassIdashState') {
			if($c eq 'a') {
				$state = 'LumClassIaState';
			} elsif($c eq 'b') {
				$lumClass = $SC_LumClass{'Ib'};
				$state = 'EndState';
			} else {
				$lumClass = $SC_LumClass{'Ia'};
				$state = 'EndState';
			}
		} elsif($state eq 'LumClassIaState') {
			if($c eq '0') {
				$lumClass = $SC_LumClass{'Ia0'};
				$state = 'EndState';
			} else {
				$lumClass = $SC_LumClass{'Ia'};
				$state = 'EndState';
			}
		} elsif($state eq 'LumClassVState') {
			if($c eq 'I') {
				$lumClass = $SC_LumClass{'VI'};
				$state = 'EndState';
			} else {
				$lumClass = $SC_LumClass{'V'};
				$state = 'EndState';
			}
		} elsif($state eq 'WDTypeState') {
			if($c =~ m/[ABCOQXZ]/) {
				$specClass = $SC_SpecClass{'D'.$c};
				$i++;
			} else {
				$specClass = $SC_SpecClass{'D'};
			}
			$state = 'WDExtendedTypeState';
		} elsif($state eq 'WDExtendedTypeState') {
			if($c =~ m/[ABCOQZXVPHE]/) {
				$i++;
			} else {
				$state = 'WDSubclassState';
			}
		} elsif($state eq 'WDSubclassState') {
			if($c =~ m/[0-9]/) {
				$subclass = $SC_Subclass{$c};
				$i++;
			}
			$state = 'EndState';
		} else {
			die "ERROR: Unknown state in spectral class parser\n";
		}
	}
	return $starType + $specClass + $subclass + $lumClass;
}

# Convert spectral class code to string
sub SpTypeToString {
	my $spType = shift;
	my $st = '?';
	if(($spType & $SC_StarType{'Mask'}) == $SC_StarType{'NormalStar'}) {
		foreach my $sp (keys %SC_SpecClass) {
			if(($sp !~ /^D/) && ($sp ne 'Mask')) {
				$st = $sp if(($spType & $SC_SpecClass{'Mask'}) == $SC_SpecClass{$sp});
			}
		}
		if($st ne '?') {
			foreach my $sc (keys %SC_Subclass) {
				if($sp ne 'Mask') {
					if(($spType & $SC_Subclass{'Mask'}) == $SC_Subclass{$sc}) {
						$st .= $sc if($sc ne '?');
					}
				}
			}
			foreach my $lc (keys %SC_LumClass) {
				if($lc ne 'Mask') {
					if(($spType & $SC_LumClass{'Mask'}) == $SC_LumClass{$lc}) {
						$st .= $lc if($lc ne '?');
					}
				}
			}
		}
	} elsif(($spType & $SC_StarType{'Mask'}) == $SC_StarType{'WhiteDwarf'}) {
		foreach my $wt (keys %SC_SpecClass) {
			if($wt =~ m/^D/) {
				$st = $wt if(($spType & $SC_SpecClass{'Mask'}) == $SC_SpecClass{$wt});
			}
		}
		if($st ne '?') {
			foreach my $sc (keys %SC_Subclass) {
				if($sp ne 'Mask') {
					if(($spType & $SC_Subclass{'Mask'}) == $SC_Subclass{$sc}) {
						$st .= $sc if($sc ne '?');
					}
				}
			}
		}
	} elsif(($spType & $SC_StarType{'Mask'}) == $SC_StarType{'NeutronStar'}) {
		$st = 'Q';
	} elsif (($spType & $SC_StarType{'Mask'}) == $SC_StarType{'BlackHole'}) {
		$st = 'X';
	}
	return $st;
}

# Guess the spectral type from the B-V colour index - use closest match to table
sub GuessSpType {
	my $BV = shift;
	my $st = '?';
	my $minDelta = 9999;
	foreach my $trial_st (keys %SpBV) {
		if(abs($BV - $SpBV{$trial_st}) < $minDelta) {
			$st = $trial_st;
			$minDelta = abs($BV - $SpBV{$trial_st});
		}
	}
	return $st;
}

# ---------------- STRING HANDLING AND MATHEMATICAL FUNCTIONS ---------------- #

# remove leading and trailing spaces from a string
sub Trim {
	my $st = shift;
	$st =~ s/(^\s+)|(\s+$)//g;
	return $st;
}

# calculate log base 10 of a number
sub Log10 {
	my $n = shift;
	return log($n)/log(10);
}
