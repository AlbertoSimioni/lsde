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
unsigned int *knows_map_reduced;
unsigned short *interest_map;
char* likedBy_output_file;
char *new_knows_output_file;
char *new_person_output_file;
char* artists_output_file;
char* likedBy_output_file;
char* path;
unsigned long person_length, knows_length, interest_length;
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

void reorg_location()
{
    unsigned int person_offset, person_friend;
    unsigned long knows_offset, knows_offset2;
    unsigned short count;
    unsigned int knows_pos;
    unsigned long new_first = 0, total_count = 0;
    unsigned int *offests_buffer=malloc(sizeof(int) * person_length/sizeof(Person));

    //unsigned long *temp_first = malloc(sizeof(long) * (person_length / sizeof(Person)));
    //unsigned short *temp_count = malloc(sizeof(short) * (person_length / sizeof(Person)));
printf("new size person: %li\n", person_length / sizeof(Person));


    Person *person, *knows;
    FILE *new_knows = open_binout(new_knows_output_file);
    FILE *new_person = open_binout(new_person_output_file);

    Person person_copy;
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
    fclose(new_person);
    fclose(new_knows);

    //TODO clear buffers
    return;
}


void reorg_location_mutual()
{
    unsigned int person_offset, person_friend, person_count=0;
    unsigned long knows_offset, knows_offset2;
    unsigned short count;
    unsigned int knows_pos;
    unsigned long new_first = 0, total_count = 0;
    unsigned int *offests_buffer=malloc(sizeof(int) *person_length / sizeof(Person));


    Person *person, *knows;
    FILE *new_knows = open_binout(new_knows_output_file);
    FILE *new_person = open_binout(new_person_output_file);

    //Person *person_buffer = malloc(sizeof(Person) * PERSON_BUFFER);
    //unsigned int knows_buffer[KNOWS_BUFFER];

    Person person_copy;
    person_count=0;
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

        }

        person_copy.knows_n = count;
         if(count>0){ //skip writing of a person with no friends
            fwrite(&person_copy, sizeof(Person), 1, new_person);
            offests_buffer[person_offset]=person_count;
            ++person_count;
        }

    }
    fclose(new_knows);
    fclose(new_person);



    printf("reached: %s\n", new_knows_output_file);
    knows_map_reduced    = (unsigned int *)   mmapr(makepath(path, "knows_reduced_temp",    "bin"), &knows_length);
    new_knows_output_file = makepath(path, "knows_reduced", "bin");
    new_knows = open_binout(new_knows_output_file);

    for(knows_offset=0; knows_offset<knows_length/sizeof(int); ++knows_offset)
     {

        fwrite(&offests_buffer[knows_map_reduced[knows_offset]], sizeof(int), 1, new_knows);
    }


    return;
}


void reorg_interests()
{
    //CODE FOR CREATING THE likedBy table and to create the new interests table
    FILE *artists = open_binout(artists_output_file);
    FILE *likedBy = open_binout(likedBy_output_file);
    unsigned int interests_buffer_size = 1000;
    unsigned int interests_count = 0;
    unsigned int count = 0;
    unsigned int total_count = 0;
    unsigned short* interests_buffer  = malloc(sizeof(short) * interests_buffer_size);
    long interest_offset, interest_buffer_offset;
    unsigned short current_interest, person_interest;
    unsigned short found;
    unsigned int person_offset;
    Person *person;
    //Populating the array of different interests

    for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
    {

        person = &person_map[person_offset];
        for (interest_offset = person->interests_first;
                interest_offset < person->interests_first + person->interest_n;
                interest_offset++){
            found = 0;
            current_interest = interest_map[interest_offset];
            unsigned int i;
            for(i = 0; i < interests_count && !found; i++){
                if(current_interest == interests_buffer[i]){
                    found = 1;
                }
            }

            if(!found)
                {
                if(interests_count == interests_buffer_size)
                    {
                        printf("doubling size of interests\n");
                        interests_buffer_size = interests_buffer_size * 2;
                        interests_buffer = realloc(interests_buffer, interests_buffer_size * sizeof(short));
                    }
                interests_buffer[interests_count] = current_interest;
                interests_count++;
                }
            }
    }

    //for(interest_offset = 0; interest_offset < interests_count; interest_offset++){ printf("%hi\n",interests_buffer[interest_offset] );}
    printf("%li \n", person_length / sizeof(Person));
    printf("interests: %i\n",interests_count);
    //ora scorro tutti gli interessi uno alla volta. Nella nuova interest devo salvare first e count
    for(interest_buffer_offset = 0; interest_buffer_offset < interests_count; interest_buffer_offset++){
        //printf("%li\n", interest_buffer_offset);
        current_interest = interests_buffer[interest_buffer_offset];
        count = 0;
        Artist artist;
        artist.interest_id = current_interest;
        artist.likedBy_first = total_count;
        for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
        {
            person = &person_map[person_offset];
            for (interest_offset = person->interests_first;
                interest_offset < person->interests_first + person->interest_n;
                interest_offset++){
                person_interest =  interest_map[interest_offset];
                if(person_interest == current_interest){
                    if(person_offset == 111112){ printf("%hi\n",current_interest);}
                    count++;
                    total_count++;
                    fwrite(&person_offset, sizeof(int), 1, likedBy);
                }
            }

        }
        artist.likedBy_n = count;

        fwrite(&artist, sizeof(Artist), 1, artists);
    }
    fclose(artists);
    fclose(likedBy);
    return;
}


int main(int argc, char *argv[])
{
    path=argv[1];
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
    new_knows_output_file = makepath(argv[1], "knows_reduced_temp", "bin");
    new_person_output_file = makepath(argv[1], "person_reduced", "bin");

    reorg_location_mutual();
    interest_map = (unsigned short *) mmapr(makepath(path, "interest", "bin"), &interest_length);
    person_map = (Person *)   mmapr(makepath(path, "person_reduced",    "bin"), &person_length);
    likedBy_output_file = makepath(path, "likedBy", "bin");
    artists_output_file = makepath(path, "artists", "bin");
    reorg_interests();

    return 0;
}
