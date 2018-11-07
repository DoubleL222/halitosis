for my $i (1..10) {
	my $output = `halite.exe --replay-directory replays/ -vvv --width 32 --height 32 "MyBot.exe" "MyBot.exe"`;
	print $output;
}