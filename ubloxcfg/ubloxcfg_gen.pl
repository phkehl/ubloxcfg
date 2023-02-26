#!/usr/bin/perl
####################################################################################################
# u-blox 9 positioning receivers configuration library
#
# Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org),
# https://oinkzwurgl.org/hacking/ubloxcfg
#
# This program is free software: you can redistribute it and/or modify it under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation, either version 3
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along with this
# program. If not, see <https://www.gnu.org/licenses/>.
#
####################################################################################################

use strict;
use warnings;

use feature 'state';

use FindBin;

use lib "$FindBin::Bin/../3rdparty/perl";

use JSON::PP;
use Time::HiRes qw(time);
use Path::Tiny;
use Data::Float qw();

my $DEBUG = 0;
my $OUT   = shift(@ARGV);
my @IN    = @ARGV;

die("RTFS :-)") unless ($OUT && ($#IN > -1));

####################################################################################################

do
{
    my @sources = ();
    my @items = ();

    foreach my $JSON (@IN)
    {
        # read raw JSON (with comments)
        my $json = '';
        eval { $json = path($JSON)->slurp_raw(); };
        if ($! || $@)
        {
            my $err = ("$!" || "$@"); $err =~ s{\s*\r?\n$}{};
            print(STDERR "Cannot read '$JSON': $err\n");
            exit(1);
        }
        DEBUG("$JSON: json raw %d bytes", length($json));

        # remove (some) comments
        $json =~ s{^\s*//.*}{}mg;
        #$json =~ s{//.*$}{}mg;
        DEBUG("$JSON: json w/o comments %d bytes", length($json));

        # parse JSON
        my $data;
        eval { $data = JSON::PP->new()->utf8()->decode($json); };
        if ($! || $@)
        {
            my $err = ("$!" || "$@"); $err =~ s{\s*\r?\n$}{}; $err =~ s{at $0 line .*}{};
            print(STDERR "$JSON: parse fail: $err\n");
            exit(1);
        }

        if (!UNIVERSAL::isa($data->{items}, 'ARRAY'))
        {
            print(STDERR "$JSON: Unexpected data structure!\n");
            exit(1);
        }

        push(@sources, @{$data->{sources}});
        push(@items, @{$data->{items}});

    }

    # verify content
    if (!verifyItems(\@items))
    {
        exit(1);
    }


    DEBUG("Have %d configuration items.", $#items + 1);

    # generate output
    my $errors = 0;
    if (genCodeC(\@items, \@sources))
    {
        exit(0);
    }
    else
    {
        exit(1);
    }

};

sub verifyItems
{
    my ($data) = @_;

    my $errors = 0;
    my %namesSeen = ();
    my %idsSeen = ();
    for (my $ix = 0; $ix <= $#{$data}; $ix++)
    {
        my $item = $data->[$ix];
        if (!$item->{name} || !$item->{id} || !defined $item->{size})
        {
            print(STDERR "Missing data at index $ix!\n");
            $errors++;
            next;
        }
        if ($namesSeen{ $item->{name} })
        {
            print(STDERR "Duplicate name '$item->{name}' at index $ix!\n");
            $errors++;
        }
        $namesSeen{ $item->{name} }++;
        if ($idsSeen{ $item->{id} })
        {
            print(STDERR "Duplicate name '$item->{id}' at index $ix!\n");
            $errors++;
        }
        $idsSeen{ $item->{id} }++;
        my $id = hex($item->{id});
        if ($id & 0x80000000)
        {
            print(STDERR "Use of reserved ID bit 31 at index $ix ($item->{name})\n");
            $errors++;
        }
        if ($id & 0x0f000000)
        {
            print(STDERR "Use of reserved ID bits 27..24 at index $ix ($item->{name})\n");
            $errors++;
        }
        if ($id & 0x0000f000) # u-blox docu is wrong. ID is 12 bits, not 8
        {
            print(STDERR "Use of reserved ID bits 15..12 at index $ix ($item->{name})\n");
            $errors++;
        }
        my %sizeNumToSize = ( 1 => 0, 2 => 1, 3 => 2, 4 => 4, 5 => 8 );
        my $sizeNum = ($id >> 28) & 0xf;
        if ( !defined $sizeNumToSize{$sizeNum} || ($sizeNumToSize{$sizeNum} != $item->{size}) )
        {
            printf(STDERR "ID and size mismatch (ID 0x%x......., size $item->{size}) at index $ix ($item->{name})!\n", $sizeNum);
            $errors++;
        }
        my %sizeToType = ( 0 => { L => 1 }, 1 => { U1 => 1, I1 => 1, X1 => 1, E1 => 1 }, 2 => { U2 => 1, I2 => 1, X2 => 1, E2 => 1 },
                           4 => { U4 => 1, I4 => 1, X4 => 1, E4 => 1, R4 => 1 }, 8 => { U8 => 1, I8 => 1, X8 => 1, R8 => 1 } );
        if (!$sizeToType{$item->{size}} || !$sizeToType{$item->{size}}->{$item->{type}})
        {
            print(STDERR "Size and type mismatch (size $item->{size}, type $item->{type}) at index $ix ($item->{name})!\n");
            $errors++;
        }
    }

    return $errors ? 0 : 1;
}


####################################################################################################
# generate c code
sub genCodeC
{
    my ($items, $sources) = @_;

    my $errors = 0;

    my $numItems = $#{$items} + 1;

    my $c = '';
    my $h = '';
    my $cc = '';
    my $hh = '';

    my $top = "// u-blox 9 positioning receivers configuration library\n" .
              "//\n" .
              "// Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org),\n" .
              "// https://oinkzwurgl.org/hacking/ubloxcfg\n" .
              "//\n" .
              "//    This program is free software: you can redistribute it and/or modify it under the terms of the\n" .
              "//    GNU Lesser General Public License as published by the Free Software Foundation, either version 3\n" .
              "//    of the License, or (at your option) any later version.\n" .
              "//\n" .
              "//    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;\n" .
              "//    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n" .
              "//    See the GNU Lesser General Public License for more details.\n" .
              "//\n" .
              "//    You should have received a copy of the GNU Lesser General Public License along with this\n" .
              "//    program. If not, see <https://www.gnu.org/licenses/>.\n" .
              "//\n" .
              "// This file is automatically generated. Do not edit.\n" .
              "\n";

    $h .= $top;
    $h .= "\n";
    $h .= "#ifndef __UBLOXCFG_GEN_H__\n";
    $h .= "#define __UBLOXCFG_GEN_H__\n";
    $h .= "\n";
    $h .= "#ifdef __cplusplus\n";
    $h .= "extern \"C\" {\n";
    $h .= "#endif\n";
    $h .= "\n";
    $h .= "/*!\n";
    $h .= "    \\defgroup UBLOXCFG_ITEMS Configuration items\n";
    $h .= "    \\ingroup UBLOXCFG\n";
    $h .= "\n";
    $h .= "    Sources:\n";
    $h .= "    - $_\n" for (@{$sources});
    $h .= "\n";
    $h .= "    \@{\n";
    $h .= "*/\n";

    $c .= $top;
    $c .= "// Sources:\n";
    $c .= "// - $_\n" for (@{$sources});
    $c .= "\n";
    $c .= "#include <stddef.h>\n";
    $c .= "#include \"ubloxcfg.h\"\n";
    $c .= "#include \"ubloxcfg_gen.h\"\n";
    $c .= "\n";

    my %sizeNames = ( 0 => 'UBLOXCFG_SIZE_BIT', 1 => 'UBLOXCFG_SIZE_ONE', 2 => 'UBLOXCFG_SIZE_TWO',
                      4 => 'UBLOXCFG_SIZE_FOUR', 8 => 'UBLOXCFG_SIZE_EIGHT');

    my @itemStructs = ();
    my %msgCfgs = ();
    my $maxItemNameLen = 0;
    my $maxConstNamesLen = 0;

    # generate item definitions
    my $lastGroup = '';
    for (my $itemIx = 0; $itemIx < $numItems; $itemIx++)
    {
        my $item = $items->[$itemIx];
        my $itemNameC = $item->{name}; $itemNameC =~ s{-}{_}g;               # CFG_FOO_BAR
        my $groupName = $item->{name}; $groupName =~ s{^(CFG-[^-]+)-.+}{$1}; # CFG-FOO
        my $groupNameC = $groupName; $groupNameC =~ s{-}{_}g;                # CFG_FOO
        DEBUG("item %s %s, group %s", $item->{id}, $item->{name}, $groupName);

        # ##### header file #####

        # new group (for Doxygen)
        if ($groupName ne $lastGroup)
        {
            # end previous group
            if ($itemIx != 0)
            {
                $h .= "\n";
                $h .= "///@}\n"; # UBLOXCFG_ITEMS_FOO (group)
            }
            # start new group
            $h .= "\n";
            $h .= "/*!\n";
            $h .= "    \\defgroup UBLOXCFG_$groupNameC $groupName\n";
            $h .= "    @\{\n";
            $h .= "*/\n";
            $h .= "\n";
            $h .= sprintf("#define UBLOXCFG_${groupNameC}_ID  0x%08x //!< Group ID of the $groupName-* items\n", hex($item->{id}) & 0x00ff0000);
            $lastGroup = $groupName;
        }

        $h .= "\n";
        $h .= "/*!\n";
        $h .= "    \\defgroup UBLOXCFG_$itemNameC $item->{name} ($item->{title})\n";
        $h .= "    \@{\n";
        $h .= "*/\n";
        $h .= sprintf("#define %-50s %-40s //!< %s\n", "UBLOXCFG_${itemNameC}_ID",   $item->{id}, "ID of $item->{name}");
        $h .= sprintf("#define %-50s %-40s //!< %s\n", "UBLOXCFG_${itemNameC}_STR",  "\"$item->{name}\"", "Name of $item->{name}");
        $h .= sprintf("#define %-50s %-40s //!< %s\n", "UBLOXCFG_${itemNameC}_TYPE", $item->{type}, "Type of $item->{name}");
        if ($item->{consts})
        {
            if ($item->{type} =~ m{^E})
            {
                $h .= "//! constants for $item->{name}\n";
                $h .= "typedef enum UBLOXCFG_${itemNameC}_e\n";
                $h .= "{\n";
                for (my $cIx = 0; $cIx <= $#{$item->{consts}}; $cIx++)
                {
                    my $const = $item->{consts}->[$cIx];
                    $h .= sprintf("    %-52s = %-40s //!< %s\n", "UBLOXCFG_${itemNameC}_$const->{name}", "$const->{value}" . ($cIx < $#{$item->{consts}} ? ',' : ''), $const->{title});
                }
                $h .= "} UBLOXCFG_${itemNameC}_t;\n";
            }
            else
            {
                $h .= "// constants for $item->{name}\n";
                for (my $cIx = 0; $cIx <= $#{$item->{consts}}; $cIx++)
                {
                    my $const = $item->{consts}->[$cIx];
                    $h .= sprintf("#define %-50s %-40s //!< %s\n", "UBLOXCFG_${itemNameC}_$const->{name}", $const->{value}, $const->{title});
                }
            }

            my $constNameLen = length(join('|', map { $_->{name} } @{$item->{consts}}));
            if ($constNameLen > $maxConstNamesLen)
            {
                $maxConstNamesLen = $constNameLen;
            }
        }
        $h .= "///@}\n"; # UBLOXCFG_ITEMS_FOO_BAR (item)

        # end last group
        if ($itemIx == $#${items})
        {
            $h .= "\n";
            $h .= "///@}\n"; # UBLOXCFG_ITEMS_FOO (group)
        }

        if (length($item->{name}) > $maxItemNameLen)
        {
            $maxItemNameLen = length($item->{name});
        }

        # ##### source file #####

        my $cItem = '';
        my $itemStruct = 'ubloxcfg_' . lcfirst(join('', map { ucfirst(lc($_)) } split('_', ${itemNameC})));
        push(@itemStructs, $itemStruct);

        $cItem .= "static const UBLOXCFG_ITEM_t $itemStruct =\n";
        $cItem .= "{\n";
        $cItem .= sprintf("    .id = $item->{id}, .name = %-50s .type = %-17s .size = %s\n",
            "\"$item->{name}\",", "UBLOXCFG_TYPE_$item->{type},", "$sizeNames{$item->{size}},");
        $cItem .= sprintf("    .order = %4d, .title =\"%s\",\n", $itemIx + 1, $item->{title});
        my $unitStr = '';
        if ($item->{unit})
        {
            $unitStr = ".unit = \"$item->{unit}\"";
        }
        my $scaleStr = '';
        if ($item->{scale})
        {
            my $f = $item->{scale} =~ m{(.+)\^(.+)} ? $1 ** $2 : 1.0 * $item->{scale};
            $scaleStr = sprintf('.scale = %-10s .scalefact = %s /* = %.20e */,',
                "\"$item->{scale}\",",  Data::Float::float_hex($f), $f);
        }
        if ($scaleStr && $unitStr)
        {
            $cItem .= sprintf("    %-20s%s,\n", "$unitStr,", $scaleStr);
        }
        elsif ($unitStr)
        {
            $cItem .= "        $unitStr,\n";
        }
        elsif ($scaleStr)
        {
            $cItem .= sprintf("        %-20s%s,\n", "", $scaleStr);
        }
        if (!$item->{consts})
        {
            substr($cItem, -2, 1, ''); # remove trailing comma
        }
        else
        {
            my $nConsts = $#{$item->{consts}} + 1;
            my $constsStruct = $itemStruct . '_consts';
            $cItem .= sprintf("    .nConsts = %3d, .consts = %s\n", $nConsts, $constsStruct);

            my $cConsts = '';
            $cConsts .= "static const UBLOXCFG_CONST_t ${constsStruct}[$nConsts] =\n";
            $cConsts .= "{\n";
            foreach my $const (@{$item->{consts}})
            {
                $cConsts .= "    {\n";
                $cConsts .= sprintf("        .name = %-20s .value = %-20s .val = { .%s = %s },\n",
                    "\"$const->{name}\",", "\"$const->{value}\",", substr($item->{type}, 0, 1), $const->{value});
                $cConsts .= "        .title = \"$const->{title}\"\n";
                $cConsts .= "    },\n";
            }
            substr($cConsts, -2, 1, '');
            $cConsts .= "};\n";
            $c .= "\n$cConsts";
        }
        $cItem .= "};\n";
        $c .= "\n$cItem";

        # is it a message rate config?
        # UBX:  CFG-MSGOUT-UBX_NAV_PVT_UART1
        # NMEA: CFG-MSGOUT-NMEA_ID_GGA_UART1, CFG-MSGOUT-PUBX_ID_POLYP_UART1
        # RTCM: CFG-MSGOUT-RTCM_3X_TYPE1005_UART1, CFG-MSGOUT-RTCM_3X_TYPE4072_0_UART1
        if ($item->{name} =~ m{^(CFG-MSGOUT-(.+)_(UART1|UART2|SPI|I2C|USB))$})
        {
            my ($cfg, $msg, $port) = ($1, $2, $3);
            # get message name from description
            if ($item->{title} =~ m{output rate of the ([-_A-Z0-9]+)}i)
            {
                my $name = uc($1);
                #DEBUG("msgCfg: $name $port $cfg");
                $msgCfgs{$name}->{$port} = { item => $cfg, itemNameC => $itemNameC, struct => $itemStruct };
            }
            else
            {
                print(STDERR "Cannot determine message name from $item->{name}'s title!\n");
                $errors++;
            }
        }
    }

    # generate list of config item definitions
    $c .= "\n";
    $c .= "static const UBLOXCFG_ITEM_t * const ubloxcfg_allItems[$numItems] =\n";
    $c .= "{\n";
    $c .= "    &$_,\n" for (@itemStructs);
    substr($c, -2, 1, '');
    $c .= "};\n";
    $hh .= "#define _UBLOXCFG_NUM_ITEMS $numItems\n";
    $hh .= "const void **_ubloxcfg_allItems(void);\n";
    $cc .= "const void **_ubloxcfg_allItems(void) { return (const void **)ubloxcfg_allItems; }\n";

    # generate aliases for message rate configs
    my @msgNames = sort keys %msgCfgs;
    $h .= "\n";
    $h .= "/*!\n";
    $h .= "    \\defgroup UBLOXCFG_MSGOUT Output message rates (aliases)\n";
    $h .= "    \@{\n";
    $h .= "*/\n";
    foreach my $msgName (@msgNames)
    {
        my $msgNameC = $msgName; $msgNameC =~ s{-}{_}g; # UBX_FOO_BAR

        $h .= "\n";
        $h .= "/*!\n";
        $h .= "    \\defgroup UBLOXCFG_MSGOUT_$msgNameC $msgName\n";
        $h .= "    \@{\n";
        $h .= "*/\n";

        $h .= sprintf("#define %-50s %-50s //!< Message %s name\n", "UBLOXCFG_${msgNameC}_STR", "\"$msgName\"", $msgName);
        foreach my $port (qw(UART1 UART2 SPI I2C USB))
        {
            if ($msgCfgs{$msgName}->{$port})
            {
                $h .= sprintf("#define %-50s %-50s //!< Config item ID for $msgName rate on port $port\n",
                    "UBLOXCFG_${msgNameC}_${port}_ID", "UBLOXCFG_$msgCfgs{$msgName}->{$port}->{itemNameC}_ID");
                $h .= sprintf("#define %-50s %-50s //!< Config item name for $msgName rate on port $port\n",
                    "UBLOXCFG_${msgNameC}_${port}_STR", "UBLOXCFG_$msgCfgs{$msgName}->{$port}->{itemNameC}_STR");
                $h .= sprintf("#define %-50s %-50s //!< Config item type for $msgName rate on port $port\n",
                    "UBLOXCFG_${msgNameC}_${port}_TYPE", "UBLOXCFG_$msgCfgs{$msgName}->{$port}->{itemNameC}_TYPE");
            }
        }
        $h .= "///@}\n";
    }
    $h .= "///@}\n";

    # generate lut for message name to config item
    my @msgrateStructs = ();
    foreach my $msgName (@msgNames)
    {
        my $msgrateStruct = 'ubloxcfg_' . lcfirst(join('', map { ucfirst(lc($_)) } split('-', $msgName)));
        push(@msgrateStructs, $msgrateStruct);
        $c .= "\n";
        $c .= "static const UBLOXCFG_MSGRATE_t $msgrateStruct =\n";
        $c .= "{\n";
        $c .= "    .msgName   = \"$msgName\",\n";
        $c .= "    .itemUart1 = " . ($msgCfgs{$msgName}->{UART1} ? '&' . $msgCfgs{$msgName}->{UART1}->{struct} : "NULL") . ",\n";
        $c .= "    .itemUart2 = " . ($msgCfgs{$msgName}->{UART2} ? '&' . $msgCfgs{$msgName}->{UART2}->{struct} : "NULL") . ",\n";
        $c .= "    .itemSpi   = " . ($msgCfgs{$msgName}->{SPI}   ? '&' . $msgCfgs{$msgName}->{SPI}->{struct}   : "NULL") . ",\n";
        $c .= "    .itemI2c   = " . ($msgCfgs{$msgName}->{I2C}   ? '&' . $msgCfgs{$msgName}->{I2C}->{struct}   : "NULL") . ",\n";
        $c .= "    .itemUsb   = " . ($msgCfgs{$msgName}->{USB}   ? '&' . $msgCfgs{$msgName}->{USB}->{struct}   : "NULL") . ",\n";
        substr($c, -2, 1, '');
        $c .= "};\n";
    }
    my $numMsgRates = $#msgNames + 1;
    $c .= "static const UBLOXCFG_MSGRATE_t * const ubloxcfg_allRates[$numMsgRates] =\n";
    $c .= "{\n";
    $c .= "    &$_,\n" for (@msgrateStructs);
    substr($c, -2, 1, '');
    $c .= "};\n";
    $hh .= "#define _UBLOXCFG_NUM_RATES $numMsgRates\n";
    $hh .= "const void **_ubloxcfg_allRates(void);\n";
    $cc .= "const void **_ubloxcfg_allRates(void) { return (const void **)ubloxcfg_allRates; }\n";
    $hh .= "#define _UBLOXCFG_MAX_ITEM_LEN $maxItemNameLen\n";
    $hh .= "#define _UBLOXCFG_MAX_CONSTS_LEN $maxConstNamesLen\n";


    my $numSources = $#{$sources} + 1;
    $c .= "static const char * const ubloxcfg_allSources[$numSources] =\n";
    $c .= "{\n";
    $c .= "    \"$_\",\n" for (@{$sources});
    #substr($c, -2, 1, '');
    $c .= "};\n";
    $hh .= "#define _UBLOXCFG_NUM_SOURCES $numSources\n";
    $hh .= "const char **_ubloxcfg_allSources(void);\n";
    $cc .= "const char **_ubloxcfg_allSources(void) { return (const char **)ubloxcfg_allSources; }\n";

    # add accessors for api
    $h .= "\n";
    $h .= "#ifndef _DOXYGEN_\n";
    $h .= $hh;
    $h .= "#endif\n";
    $c .= "\n";
    $c .= "#ifndef _DOXYGEN_\n";
    $c .= $cc;
    $c .= "#endif\n";

    # end header
    $h .= "\n";
    $h .= "#ifdef __cplusplus\n";
    $h .= "}\n";
    $h .= "#endif\n";
    $h .= "\n";
    $h .= "#endif // __UBLOXCFG_GEN_H__\n";
    $h .= "\n";
    $h .= "///@}\n";

    # end source
    $c .= "\n";

    # write output
    eval { path("$OUT.h")->spew_raw($h); };
    if ($! || $@)
    {
        my $err = ("$!" || "$@"); $err =~ s{\s*\r?\n$}{};
        print(STDERR "Failed writing '$OUT.h': $err\n");
        $errors++;
    }
    eval { path("$OUT.c")->spew_raw($c); };
    if ($! || $@)
    {
        my $err = ("$!" || "$@"); $err =~ s{\s*\r?\n$}{};
        print(STDERR "Failed writing '$OUT.c': $err\n");
        $errors++;
    }

    return $errors ? 0 : 1;
}

####################################################################################################
# funky functions

sub DEBUG
{
    return unless ($DEBUG);
    my ($strOrFmt, @args) = @_;
    if ($strOrFmt =~ m/%/)
    {
        printf(STDERR "$strOrFmt\n", @args);
    }
    else
    {
        print(STDERR "$strOrFmt\n");
    }
    return 1;
}


####################################################################################################
__END__
