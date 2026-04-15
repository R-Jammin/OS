int main() 
{
    char * message1 = "hello";
    char * message2 = "from";
    char * message3 = "ikanOS Compiler";

    printString(3, 5, 5, message1);
    wait(1);
    printString(3, 6, 5, message2);
    wait(1);
    printString(3, 7, 5, message3);
    wait(1);

    systemShowInodeDetail(2);
    wait(2);
    
    systemBlockViewer(519);
    wait(2);

    systemDiskSectorViewer(0);
    wait(2);

    clearScreen();
    
    systemShowGlobalObjects();

    int i = 0;
    while (i < 2) {
        wait(1);
        i = i + 1;
    }
    
    clearScreen();

    char * message = "Summing 1 + 2...";
    printString(0x0F, 5, 5, message);

    int v=1;
    int sum = v + 2;

    int* buf = 0x403800;
    itoa(sum, buf);
  
    printString(0x0F, 7, 5, buf);

    wait(3);

    systemExit(0);
}