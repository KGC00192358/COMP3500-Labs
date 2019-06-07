#!/bin/bash

declare -a TESTS=("FCFS" "SJF" "RR1MS" "RR5MS" "RR10MS" "RR15MS" "RR20MS" "RR25MS" "RR50MS")
declare -a QUANTA=(1 5 10 15 20 25 50)
CURRENTQ=0

cd /home/u3/kgc0019/COMP3500-Labs/lab1
mkdir "./TEST_RESULTS"
cd ./TEST_RESULTS
	for TEST in "${TESTS[@]}"
	do
		if [ -f "$TEST Data" ]
		then
			read -p " $TEST Data Found, Overwite? " -n 1 -r
			echo
			if [[ $REPLY =~ ^[Yy]$ ]]
			then	
				echo "Testing $TEST..."
				if [[ $TEST == "FCFS" ]]
				then
					../pm 1 >> "$TEST Data"
				fi 
				if [[ $TEST == "SJF" ]]
				then
					../pm 2 >> "$TEST Data"
				fi
				if [[ $TEST == RR* ]]
				then
					echo "Test with Q = ${QUANTA[$CURRENTQ]}"
					../pm 3 ${QUANTA[$CURRENTQ]} >> "$TEST Data"
					CURRENTQ=$CURRENTQ+1
				fi
			fi
		else
			touch "$TEST Data"
			echo "Testing $TEST..."
			if [[ $TEST == "FCFS" ]]
			then
				../pm 1 >> "$TEST Data"
			fi 
			if [[ $TEST == "SJF" ]]
			then
				../pm 2 >> "$TEST Data"
			fi
			if [[ $TEST == RR* ]]
			then
				echo "Test with Q = ${QUANTA[$CURRENTQ]}"
				../pm 3 ${QUANTA[$CURRENTQ]} >> "$TEST Data"
				CURRENTQ=$CURRENTQ+1
			fi
		fi
		echo "$TEST test results: " >> "Main Data"
		sed -n 1253,1261p "$TEST Data">tmp
		cat tmp >> "Main Data"
		(cat ""; echo) >> "Main Data"	
		echo "$TEST test complete!"
	done
	#Clean Up
	echo "Entering Cleanup"
#	for FILE in "${TESTS[@]}"
#	do
#		rm "$FILE Data"
#	done
