#!/usr/bin/bash

echo -e "\e[1;37mSanta's elves are building your programs...\e[0;37m"
if gcc -std=c89 -fsanitize=address main_p1.c -o prog1; then
	echo -e "\e[1;32mThe elves have finished preparing your part 1 program!\e[0;37m"
else
	echo -e "\e[1;31mThe elves can't build your part 1 program because there is something deeply wrong with it!\e[0;37m"
fi

if gcc -std=c89 -fsanitize=address main_p2.c -o prog2; then
	echo -e "\e[1;32mThe elves have finished preparing your part 2 program!\e[0;37m"
else
	echo -e "\e[1;31mThe elves can't build your part 2 program because there is something deeply wrong with it!\e[0;37m"
fi
