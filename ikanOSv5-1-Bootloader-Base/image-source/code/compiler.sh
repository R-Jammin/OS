echo "Starting Compiler Script..."
cc prog1.c
al prog1.s
chmod prog1 -rwxrwxrwx
prog1
cc prog2.c
al prog2.s
chmod prog2 -rwxrwxrwx
prog2
rm prog1.s
rm prog1
rm prog2.s
rm prog2
echo "Compiler Script End..."
END