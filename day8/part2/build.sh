#!/usr/bin/bash

echo -e "\e[1;37mSanta's elves are building your program...\e[0;37m"
if gcc -std=c89 -fsanitize=address main.c -o prog -lm; then
	echo -e "\e[1;32mThe elves have finished preparing your program!\e[0;37m"
else
	echo -e "\e[1;31mThe elves can't build your program because there is something deeply wrong with it!\e[0;37m"
fi
