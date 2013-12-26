#!/bin/bash

# usage: automatizedtnd command subcommand <argument>

# controllo che ci sia almeno un sotto-comando e non ci siano più di 3 argomenti
if [ $# -lt 2 -o $# -gt 3 ]
then
	echo "usage: automatizedtnd command subcommand <argument>"
	exit 1
fi

COMMAND="$1"
SUBCOMMAND="$2"

# controllo se ho anche un argomento
if [[ $# -eq 3 ]]
then
	ARGUMENT="$3"
	echo "======================================"
	echo "eseguo il comando: "$COMMAND" "$SUBCOMMAND" "$ARGUMENT" nel demone dtnd..."
	echo "--------------------------------------"
else
	echo "======================================"
	echo "eseguo il comando: "$COMMAND" "$SUBCOMMAND" nel demone dtnd..."
	echo "--------------------------------------"
fi

/usr/bin/expect << EOD
# connessione telnet al demone dtnd
spawn telnet localhost 5050
# invia il comando
expect "%"
send "$COMMAND $SUBCOMMAND $ARGUMENT\r"
expect "%"
# salva l'output del comando nella variabile "result"
# questa variabile potrá essere visualizzata in seguito o salvata su file
#set results $expect_out(buffer)
# invia il comando di termine sessione ed attende l'eof
# exit from daemon
send "exit\r"
expect eof

EOD

#echo $results > "/log"
