#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "utils.h"


int main(int argc, char const *argv[])
{
    unsigned long person_length;
    Person* person_map   = (Person *)  mmapr(makepath(argv[1], "person",   "bin"), &person_length);

    int location_found[1000];
    int last=0;
    int i,j;
    for(i=0; i<&person_length; ++i){
        int found=0;
        for(j=0; j<1000 && !found; j++){
            if( (person_map+i)->location == location_found[j]){
                found=1;
            }
        }
        if(!found){
            last++;
            if(last==1000){
                printf("too many\n");
                exit(1);
            }
            location_found[last]=(person_map+i)->location;
        }
    }
    printf("%s\n", last);
    return 0;
}