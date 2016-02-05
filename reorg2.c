#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "utils.h"

Person *person_map;
unsigned int *knows_map;
char *new_knows_output_file;
unsigned long person_length, knows_length;
unsigned short *location_map;

void save_locations()
{
    Person *person;
    unsigned int person_offset;

    location_map = malloc(sizeof(short) * (person_length / sizeof(Person)));

    for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
    {
        person = &(person_map[person_offset]);
        location_map[person_offset] = person->location;
    }

}

void reorg_knows()
{
    unsigned int person_offset, person_friend;
    unsigned long knows_offset, knows_offset2;
    int count;
    unsigned int knows_pos;

    Person *person, *knows, *person_old;
    FILE *new_knows = open_binout(new_knows_output_file);

    for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
    {
        person = &person_map[person_offset];
        count=0;
        // check if friend lives in same city and likes artist
        for (knows_offset = person->knows_first;
                knows_offset < person->knows_first + person->knows_n;
                knows_offset++)
        {
            count++;
            //this is person's friend, let's check if it's reciprocal
            knows_pos = knows_map[knows_offset];
            knows = &person_map[knows_pos];   

            if(location_map[person_offset] != location_map[knows_pos]) continue;


            for(knows_offset2 = knows->knows_first; knows_offset2 < knows->knows_first + knows->knows_n; knows_offset2++)
            {
                //if I find person_offset  here then there is mutual friendship
                if(knows_pos == knows_map[knows_offset2] )
                {
                    fwrite(&person_friend , sizeof(int), 1, new_knows);
                }
            }
        }
        
    }
    return;
}

int main(int argc, char *argv[])
{
    char *person_output_file   = makepath(argv[1], "person",   "bin");
    char *interest_output_file = makepath(argv[1], "interest", "bin");
    char *knows_output_file    = makepath(argv[1], "knows",    "bin");
    new_knows_output_file = makepath(argv[1], "new_knows", "bin");
    // this does not do anything yet. But it could...

    /* memory-map files created by loader */
    person_map   = (Person *)         mmapr(makepath(argv[1], "person",   "bin"), &person_length);
    knows_map    = (unsigned int *)   mmapr(makepath(argv[1], "knows",    "bin"), &knows_length);

    save_locations();
    reorg_knows();

    return 0;
}
