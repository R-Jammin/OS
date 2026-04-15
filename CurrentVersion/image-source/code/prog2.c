int main() 
{
    char * message1 = "Running Filetst...";
    char * message2 = "filetst";

    printString(3, 5, 5, message1);
    wait(1);
    systemExec(message2, 50, 0, 0, 0);
    
    systemExit(0);
}