#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "utils.h"

#define PERSON_BUFFER 500000
#define KNOWS_BUFFER 2000000

Person *person_map;
unsigned int *knows_map;
char *new_knows_output_file;
char *new_person_output_file;
unsigned long person_length, knows_length;
unsigned long loc_size;
unsigned short *location_map;
unsigned short *visited_locations;

void save_locations()
{
    Person *person;
    unsigned int person_offset;
    int count = 0, found = 0;
    short location;

    location_map = malloc(sizeof(short) * (person_length / sizeof(Person)));
    loc_size = 600;
    visited_locations = malloc(sizeof(short) * loc_size);

    for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
    {
        person = &(person_map[person_offset]);
        location = person->location;
        location_map[person_offset] = location;

        found = 0;
        int i;
        for(i = 0; i < count && !found; ++i)
        {
            if(location == visited_locations[i])
            {
                found = 1;
            }
        }
        if(!found)
        {
            visited_locations[count] = location;
            ++count;
            if(count == loc_size)
            {
                printf("doubling size of visited\n");
                loc_size = loc_size * 2;
                visited_locations = realloc(visited_locations, loc_size * sizeof(short));
            }
        }
    }
    loc_size = count;
    printf("visited: %li\n", loc_size );

    visited_locations = realloc(visited_locations, loc_size * sizeof(short));


}

// void reorg_knows()
// {
//     unsigned int person_offset, person_friend;
//     unsigned long knows_offset, knows_offset2;
//     unsigned short count;
//     unsigned int knows_pos;
//     unsigned int total_count = 0;
//     unsigned int new_first = 0;
//     unsigned long *temp_first = malloc(sizeof(long) * (person_length / sizeof(Person)));
//     unsigned short *temp_count = malloc(sizeof(short) * (person_length / sizeof(Person)));


//     Person *person, *knows;
//     FILE *new_knows = open_binout(new_knows_output_file);
//     FILE *new_person = open_binout(new_person_output_file);

//     Person *person_buffer = malloc(sizeof(Person) * PERSON_BUFFER);
//     unsigned int knows_buffer[KNOWS_BUFFER];

//     Person person_copy;
//     timestamp();
//     for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
//     {
//         if(person_offset % 10000 == 0)
//         {
//             printf("Reached person %i of %li\n", person_offset, person_length);
//         }
//         person = &person_map[person_offset];
//         new_first = total_count;

//         count = 0;
//         // check if friend lives in same city and likes artist
//         for (knows_offset = person->knows_first;
//                 knows_offset < person->knows_first + person->knows_n;
//                 knows_offset++)
//         {
//             //this is person's friend, let's check if it's reciprocal
//             knows_pos = knows_map[knows_offset];
//             knows = &person_map[knows_pos];

//             if(location_map[person_offset] != location_map[knows_pos]) continue;


//             for(knows_offset2 = knows->knows_first; knows_offset2 < knows->knows_first + knows->knows_n; knows_offset2++)
//             {
//                 //if I find person_offset  here then there is mutual friendship
//                 if(person_offset == knows_map[knows_offset2] )
//                 {
//                     knows_buffer[total_count % KNOWS_BUFFER] = knows_pos;
//                     ++count;
//                     ++total_count;
//                     if((total_count % KNOWS_BUFFER) == 0)
//                     {
//                         printf("writing knows\n");
//                         fwrite(&knows_buffer, sizeof(int), KNOWS_BUFFER, new_knows);
//                     }
//                 }
//             }
//         }
//         person_buffer[person_offset % PERSON_BUFFER] = *person;
//         person_buffer[person_offset % PERSON_BUFFER].knows_first = new_first;
//         person_buffer[person_offset % PERSON_BUFFER].knows_n = count;

//         if(((person_offset + 1) % PERSON_BUFFER) == 0)
//         {
//             printf("writing person\n");
//             fwrite(person_buffer, sizeof(Person), PERSON_BUFFER, new_person);
//         }
//     }
//     //TODO clear buffers
//     timestamp();
//     return;
// }

void reorg_location()
{
    unsigned int person_offset, person_friend;
    unsigned long knows_offset, knows_offset2;
    unsigned short count;
    unsigned int knows_pos;
    unsigned long new_first = 0, total_count = 0;

    //unsigned long *temp_first = malloc(sizeof(long) * (person_length / sizeof(Person)));
    //unsigned short *temp_count = malloc(sizeof(short) * (person_length / sizeof(Person)));


    Person *person, *knows;
    FILE *new_knows = open_binout(new_knows_output_file);
    FILE *new_person = open_binout(new_person_output_file);

    //Person *person_buffer = malloc(sizeof(Person) * PERSON_BUFFER);
    //unsigned int knows_buffer[KNOWS_BUFFER];

    Person person_copy;
    timestamp();
    for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
    {

        person = &person_map[person_offset];

        person_copy = *person;
        person_copy.knows_first = total_count;

        count = 0;
        // check if friend lives in same city and likes artist
        for (knows_offset = person->knows_first;
                knows_offset < person->knows_first + person->knows_n;
                knows_offset++)
        {
            //this is person's friend, let's check if it's reciprocal
            knows_pos = knows_map[knows_offset];
            //knows = &person_map[knows_pos];

            if(location_map[person_offset] != location_map[knows_pos]) continue;
            count++;
            total_count++;
            fwrite(&knows_pos, sizeof(int), 1, new_knows);

            // knows_buffer[total_count % KNOWS_BUFFER] = knows_pos;
            // ++count;
            // ++total_count;
            // if((total_count % KNOWS_BUFFER) == 0)
            // {
            //     printf("writing knows\n");
            //     fwrite(&knows_buffer, sizeof(int), KNOWS_BUFFER, new_knows);
            // }
        }

        person_copy.knows_n = count;
        fwrite(&person_copy, sizeof(Person), 1, new_person);
        //  if(count>10)
        //     print_person(&person_copy);

        // if(person_offset < 10 )
        // {
        //     printf("Person Copy %i:\n", person_offset);
        //     print_person(&person_copy);
        // }

        // person_buffer[person_offset % PERSON_BUFFER] = *person;
        // person_buffer[person_offset % PERSON_BUFFER].knows_first = new_first;
        // person_buffer[person_offset % PERSON_BUFFER].knows_n = count;

        // if(((person_offset + 1) % PERSON_BUFFER) == 0)
        // {
        //     printf("writing person\n");
        //     fwrite(person_buffer, sizeof(Person), PERSON_BUFFER, new_person);
        // }
    }
    //TODO clear buffers
    fclose(new_knows);
    fclose(new_person);
    timestamp();
    return;
}


void reorg_location_mutual()
{
    unsigned int person_offset, person_friend;
    unsigned long knows_offset, knows_offset2;
    unsigned short count;
    unsigned int knows_pos;
    unsigned long new_first = 0, total_count = 0;

    //unsigned long *temp_first = malloc(sizeof(long) * (person_length / sizeof(Person)));
    //unsigned short *temp_count = malloc(sizeof(short) * (person_length / sizeof(Person)));


    Person *person, *knows;
    FILE *new_knows = open_binout(new_knows_output_file);
    FILE *new_person = open_binout(new_person_output_file);

    //Person *person_buffer = malloc(sizeof(Person) * PERSON_BUFFER);
    //unsigned int knows_buffer[KNOWS_BUFFER];

    Person person_copy;
    timestamp();
    for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
    {

        person = &person_map[person_offset];

        person_copy = *person;
        person_copy.knows_first = total_count;

        count = 0;
        // check if friend lives in same city and likes artist
        for (knows_offset = person->knows_first;
                knows_offset < person->knows_first + person->knows_n;
                knows_offset++)
        {
            //this is person's friend, let's check if it's reciprocal
            knows_pos = knows_map[knows_offset];
            knows = &person_map[knows_pos];
            for (knows_offset2 = knows->knows_first;
                    knows_offset2 < knows->knows_first + knows->knows_n;
                    knows_offset2++)
            {
                if(knows_map[knows_offset2] == person_offset){
                    count++;
                    total_count++;
                    fwrite(&knows_pos, sizeof(int), 1, new_knows);
                }
            }

            

            // knows_buffer[total_count % KNOWS_BUFFER] = knows_pos;
            // ++count;
            // ++total_count;
            // if((total_count % KNOWS_BUFFER) == 0)
            // {
            //     printf("writing knows\n");
            //     fwrite(&knows_buffer, sizeof(int), KNOWS_BUFFER, new_knows);
            // }
        }

        person_copy.knows_n = count;
        fwrite(&person_copy, sizeof(Person), 1, new_person);
        //  if(count>10)
        //     print_person(&person_copy);

        // if(person_offset < 10 )
        // {
        //     printf("Person Copy %i:\n", person_offset);
        //     print_person(&person_copy);
        // }

        // person_buffer[person_offset % PERSON_BUFFER] = *person;
        // person_buffer[person_offset % PERSON_BUFFER].knows_first = new_first;
        // person_buffer[person_offset % PERSON_BUFFER].knows_n = count;

        // if(((person_offset + 1) % PERSON_BUFFER) == 0)
        // {
        //     printf("writing person\n");
        //     fwrite(person_buffer, sizeof(Person), PERSON_BUFFER, new_person);
        // }
    }
    //TODO clear buffers
    timestamp();
    return;
}


void reorg()
{
    //
    unsigned int person_offset;
    unsigned long knows_offset, knows_offset2;
    unsigned short count;
    unsigned int total_count = 0;
    unsigned int person_count = 0;
    unsigned int knows_pos;
    unsigned int person_pos;
    unsigned short current_loc;
    Person *person_buffer = NULL;
    unsigned int *person_offset_buffer = NULL;
    // short* knows_buffer=NULL;
    unsigned int person_buffer_size = 5000;
    // unsigned int knows_buffer_size=40000;
    // unsigned int knows_buffer_pos;
    unsigned int new_first = 0;
    //unsigned long *temp_first = malloc(sizeof(long) * (person_length / sizeof(Person)));
    //unsigned short *temp_count = malloc(sizeof(short) * (person_length / sizeof(Person)));


    Person *person, *knows;
    FILE *new_knows = open_binout(new_knows_output_file);
    FILE *new_person = open_binout(new_person_output_file);



    //Person *person_buffer = malloc(sizeof(Person) * PERSON_BUFFER);
    //unsigned int knows_buffer[KNOWS_BUFFER];

    Person person_copy;
    timestamp();
    int loc_i;
    for(loc_i = 0; loc_i < loc_size; ++loc_i)
    {
        printf("location: %i\n", loc_i);
        person_buffer_size = 2000;
        person_buffer = realloc(person_buffer, sizeof(Person) * person_buffer_size);
        person_offset_buffer = realloc(person_offset_buffer, sizeof(int) * person_buffer_size);
        //knows_buffer=realloc(knows_buffer, sizeof(short) * knows_buffer_size);

        current_loc = location_map[loc_i];
        person_pos = 0;
        //knows_buffer_pos=0;
        for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
        {
            person = &person_map[person_offset];
            if(person_offset % 500000 == 0)
            {
                printf("Reached person %i \n", person_offset);
            }
            if(location_map[person_offset] == current_loc)
            {
                person_buffer[person_pos] = *person;
                person_offset_buffer[person_pos] = person_offset;
                ++person_pos;
                if(person_pos == person_buffer_size)
                {
                    person_buffer_size *= 2;
                    person_buffer = realloc(person_buffer, person_buffer_size * sizeof(Person));
                    person_offset_buffer = realloc(person_offset_buffer, person_buffer_size * sizeof(int));
                }
            }

        }
        unsigned int final_pos = person_pos;
        for(person_pos = 0; person_pos < final_pos; ++person_pos)
        {
            if(person_pos % 2000 == 0)
            {
                printf("Reached person %i \n", person_pos);
            }
            person = &person_buffer[person_pos];
            new_first = total_count;
            count = 0;

            // check if friend lives in same city and likes artist
            for (knows_offset = person->knows_first;
                    knows_offset < person->knows_first + person->knows_n;
                    knows_offset++)
            {
                //this is person's friend, let's check if it's reciprocal
                knows_pos = knows_map[knows_offset];
                unsigned int friend_pos = -1;
                //knows = &person_map[knows_pos];
                int i;
                for(i = 0; i < final_pos; ++i)
                {
                    if(knows_pos == person_offset_buffer[i])
                    {
                        friend_pos = i;
                        break;
                    }
                }
                if(friend_pos == -1) continue;
                knows = &person_buffer[friend_pos];
                for(knows_offset2 = knows->knows_first; knows_offset2 < knows->knows_first + knows->knows_n; knows_offset2++)
                {
                    //if I find person_offset  here then there is mutual friendship
                    if(person_offset_buffer[person_pos] == knows_map[knows_offset2] )
                    {
                        unsigned int pos = person_count + friend_pos;
                        fwrite(&pos, sizeof(int), 1, new_knows);
                        ++count;
                        ++total_count;

                        //TODO buffer optimization
                        // adjust counter after knows_buffer
                        //knows_buffer[total_count % KNOWS_BUFFER] = knows_pos;
                        // ++count;
                        //++total_count;
                        // if((total_count % KNOWS_BUFFER) == 0)
                        // {
                        //     printf("writing knows\n");
                        //     fwrite(&knows_buffer, sizeof(int), KNOWS_BUFFER, new_knows);
                        // }
                    }
                }
            }
            person->knows_first = new_first;
            person->knows_n = count;
        }
        person_count += final_pos;
        fwrite(person_buffer, sizeof(Person), final_pos, new_person);
    }
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
    person_map   = (Person *)         mmapr(makepath(argv[1], "person",   "bin"), &person_length);
    knows_map    = (unsigned int *)   mmapr(makepath(argv[1], "knows",    "bin"), &knows_length);

    save_locations();
    reorg_location(); //delete all friendships from knows file of friends in different cities

    person_map   = (Person *)         mmapr(makepath(argv[1], "new_person",   "bin"), &person_length);
    knows_map    = (unsigned int *)   mmapr(makepath(argv[1], "new_knows",    "bin"), &knows_length);
    new_knows_output_file = makepath(argv[1], "knows_reduced", "bin");
    new_person_output_file = makepath(argv[1], "person_reduced", "bin");

    reorg_location_mutual();
    return 0;
}
