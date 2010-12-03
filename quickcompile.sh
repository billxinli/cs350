#!/bin/bash
echo "Type h for help"

while true; do
	echo -ne "CS350: "
	read command
	case $command in
                h)
                        echo "Commands that are defined:"
                        echo "c - To compile the os, you will need to give the assignment number."
                        echo "e - To execute the last compiled OS."
                        echo "q - To quit."
                ;;

		s)
			cd ~/cs350-os161/os161-1.11/kern/
			svn update
		;;

		c)
			echo -ne "Which assignment? "
			read asst
			if [ $asst -eq $asst 2> /dev/null -a $asst -lt 5 ] 
			then
				cd ~/cs350-os161/os161-1.11/kern/conf
				./config ASST${asst}
				cd ../compile/ASST${asst}
				make depend
				make 
				make install
			fi
			echo =================================================
			echo done compiling asst${asst}
			echo =================================================
			                        cd ~/cs350-os161/root
                        sys161 kernel "p testbin/add 1 2; q"
                        echo =================================================
                        echo done
                        echo =================================================

		;;
		e)
			cd ~/cs350-os161/root
			sys161 kernel
			echo =================================================
			echo done
			echo =================================================

		;;
                t)
			cd ~/cs350-os161/os161-1.11/kern/conf
			./config ASST2
			cd ../compile/ASST2
			make clean > /dev/null
			if [ $? -gt 0 ]; then
				echo "ERROR!"
			fi
			make depend > /dev/null
			if [ $? -gt 0 ]; then
				echo "ERROR!"
			fi
			make > /dev/null
			if [ $? -gt 0 ]; then
				echo "ERROR!"
			fi
			make install > /dev/null
			if [ $? -gt 0 ]; then
				echo "ERROR!"
			fi
			cd ~/cs350-os161/root
			sys161 kernel "p testbin/badcall; q"
                ;;

		q)
			exit 0
		;;

	esac


done
