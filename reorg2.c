#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "utils.h"

#define PERSON_BUFFER 200000
#define KNOWS_BUFFER 1000000

Person *person_map;
unsigned int *knows_map;
char *new_knows_output_file;
char *new_person_output_file;
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
    unsigned short count;
    unsigned int knows_pos;
    unsigned int total_count = 0;
    unsigned int new_first = 0;
    unsigned long *temp_first = malloc(sizeof(long) * (person_length / sizeof(Person)));
    unsigned short *temp_count = malloc(sizeof(short) * (person_length / sizeof(Person)));


    Person *person, *knows;
    FILE *new_knows = open_binout(new_knows_output_file);
    FILE *new_person = open_binout(new_person_output_file);

    Person* person_buffer = malloc(sizeof(Person)*PERSON_BUFFER);
    unsigned int knows_buffer[KNOWS_BUFFER];

    Person person_copy;

    for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
    {
        if(person_offset % 10000 == 0)
        {
            printf("Reached person %i of %li\n", person_offset, person_length);
        }
        person = &person_map[person_offset];
        new_first = total_count;

        count = 0;
        // check if friend lives in same city and likes artist
        for (knows_offset = person->knows_first;
                knows_offset < person->knows_first + person->knows_n;
                knows_offset++)
        {
            //this is person's friend, let's check if it's reciprocal
            knows_pos = knows_map[knows_offset];
            knows = &person_map[knows_pos];

            if(location_map[person_offset] != location_map[knows_pos]) continue;


            for(knows_offset2 = knows->knows_first; knows_offset2 < knows->knows_first + knows->knows_n; knows_offset2++)
            {
                //if I find person_offset  here then there is mutual friendship
                if(person_offset == knows_map[knows_offset2] )
                {
                    knows_buffer[total_count % KNOWS_BUFFER] = knows_pos;
                    ++count;
                    ++total_count;
                    if((total_count % KNOWS_BUFFER) == 0)
                    {
                        printf("writing knows\n");
                        fwrite(&knows_buffer, sizeof(int), KNOWS_BUFFER, new_knows);
                    }
                }
            }
        }
        person_buffer[person_offset % PERSON_BUFFER] = *person;
        person_buffer[person_offset % PERSON_BUFFER].knows_first = new_first;
        person_buffer[person_offset % PERSON_BUFFER].knows_n = count;

        if((person_offset % PERSON_BUFFER) == 0)
        {
            printf("writing person\n");
            fwrite(person_buffer, sizeof(Person), PERSON_BUFFER, new_person);

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
    new_person_output_file = makepath(argv[1], "new_person", "bin");
    // this does not do anything yet. But it could...

    /* memory-map files created by loader */
    person_map   = (Person *)         mmaprw(makepath(argv[1], "person",   "bin"), &person_length);
    knows_map    = (unsigned int *)   mmapr(makepath(argv[1], "knows",    "bin"), &knows_length);

    save_locations();
    reorg_knows();

    return 0;
}
