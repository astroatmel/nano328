#include <unistd.h>

int main(void)
{
char *args0[]={"chmod","666","/dev/ttyACM0",(char)0};
char *args1[]={"chmod","666","/dev/ttyACM1",(char)0};

pid_t pid = fork();
if ( pid ) execv("/bin/chmod",args0 );  // rememver that execv stops execution here, because it replaces the current process, this is why I use fork
else       execv("/bin/chmod",args1 );
return 0;
}
