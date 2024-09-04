#!/bin/bash
ARGS="Wrong number of arguments"
FATAL="Fatal error"
ROOT_MAIN="./mini_serv_practice3.c"
err=0
port=0
while [ $port -lt 1024 -o $port -gt 10000 ]; do
    port=$RANDOM
done
rm -rf mini_serv log*.txt
printf "Running tests on port $port...\n"

check()
{
[ $? -eq 1 ] && printf "✅" || { printf "❌"; err=$(( $err + 1 )); }
[ "$(cat log.txt)" = "$1" ] && printf "\033[70G✅" || { printf "\033[70G❌"; err=$(( $err + 1 )); }
}


check_diff()
{
INDENT=50

case "$1" in
    5 | 7)
        INDENT=55
        ;;
esac

diff "log$1.txt" "assets/test$1" &> /dev/null
[ $? -eq 0 ] && printf "\033[${INDENT}G✅" || { printf "\033[${INDENT}G❌"; err=$(( $err + 1 )); }
}

### COMPIL
printf "\033[36m[+] Checking compilation...\033[0m\033[50G"
gcc -Wall -Wextra -Werror $ROOT_MAIN -o mini_serv &> log.txt
[ "$(cat log.txt)" = "" ] && printf "✅"|| { printf "\033[31mCompilation error...\033[0m"; cat log.txt; rm log.txt; exit 1; }


### HANDLE ERRORS
printf "\n\033[36m[+] Checking errors\033[0m\033[50GExit status\033[70GDiff Output\n"
printf "\033[33m[+] Checking no arguments output...\033[0m\033[50G"
./mini_serv &> log.txt
check "$ARGS"

printf "\n\033[33m[+] Checking two many arguments output...\033[0m\033[50G"
./mini_serv 1 2 &> log.txt
check "$ARGS"

printf "\n\033[33m[+] Checking syscall error output...\033[0m\033[50G"
./mini_serv "$port" > log.txt &
./mini_serv "$port" &> log.txt
check "$FATAL"
printf "\n[+] Killing mini_serv...\n"
pkill mini_serv

### DIFF OUTPUT
printf "\n\033[36m[+] Checking output...\033[0m\033[50G"
printf "\n\033[33m[+] 1 client and 1 message...\033[0m\033[50G"
./mini_serv "$port" > log1.txt &
r=$(nc localhost "$port" > log1.txt) &
sleep 0.3
echo "coucou" | nc localhost "$port"
check_diff 1
printf "\n[+] Killing mini_serv...\n"
pkill mini_serv


printf "\n\033[33m[+] 1 client and many messages...\033[0m\033[50G"
port=$(( $port - 5 ))

./mini_serv "$port" > log2.txt &
r=$(nc localhost "$port" > log2.txt) &
sleep 0.3
echo "coucou" | nc localhost "$port"
echo "good morning" | nc localhost "$port"
# hola가 client3
echo "holà que tal?" | nc localhost "$port"
# GREEN이 클4  RED 클4
echo -e "\033[32mGREEN\n\t\033[31mRED" | nc localhost "$port"
# 공백이 클5
echo "" | nc localhost "$port"
# 시작할때 client 6: [0mhello		bonjour
echo -e "\033[0mhello\t\tbonjour" | nc localhost "$port"
echo "last one" | nc localhost "$port"
echo "✅" | nc localhost "$port"
check_diff 2
echo -e "\n[+] Killing mini_serv...\n"
pkill mini_serv


printf "\n\033[33m[+] 1 client and multi lines...\033[0m\033[50G"
./mini_serv "$(( $port + 1 ))"  > log3.txt &
r=$(nc localhost "$(( $port + 1 ))" > log3.txt) &
sleep 0.3
cat assets/lorem | nc localhost "$(( $port + 1 ))"
check_diff 3
printf "\n[+] Killing mini_serv...\n"
pkill mini_serv


printf "\n\033[33m[+] 2 clients and a simple message...\033[0m\033[50G"
./mini_serv "$(( $port + 2 ))" > log4.txt &
r=$(nc localhost "$(( $port + 2 ))" > log4.txt) &
sleep 0.3
r2=$(nc localhost "$(( $port + 2 ))" > log5.txt) &
sleep 0.3
echo "hello" | nc localhost "$(( $port + 2 ))"
check_diff 4
check_diff 5
printf "\n[+] Killing mini_serv...\n"
pkill mini_serv


printf "\n\033[33m[+] 2 clients and multi ligne message...\033[0m\033[50G"
./mini_serv "$(( $port - 1 ))"  > log6.txt &
r=$(nc localhost "$(( $port - 1 ))" > log6.txt) &
sleep 0.3
r2=$(nc localhost "$(( $port - 1 ))" > log7.txt) &
sleep 0.3
cat assets/lorem | nc localhost "$(( $port - 1 ))"
check_diff 6
check_diff 7
printf "\n[+] Killing mini_serv...\n"
pkill mini_serv


### SUMMARY
[ $err -eq 0 ] && printf "\n\033[32mAll tests passed :)\033[0m" || printf "\033[31m$err test(s) failed :(\033[0m"

sleep 1.5
### CLEAN
rm -rf mini_serv log*.txt

exit $err
