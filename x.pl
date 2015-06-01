#!/usr/bin/perl

open FILE,">","out_words.txt" or die "cannot open ouputfile";
while(<>)
{
    chomp;
    @a=split/,/;
    $root=shift @a;
    print FILE "$root\n";
    foreach my $w (@a)
    {
           $word{$root}{$w}=1;
           $word{$w}{$root}=1;
           print FILE "$w\n";
    }
}
close FILE;

print "[\n";
foreach my $w (keys %word)
{
    my $sep;
    foreach my $k (keys %{$word{$w}})
    {
         print 
         qq!$sep {
            "source" : "$w",
            "target" : "$k",
             "type" : "suit"
         }!;
        $sep=",\n";
    }
}
print "]\n";
