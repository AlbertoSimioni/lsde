#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>

#include "utils.h"

#define PERSON_BUFFER 500000
#define KNOWS_BUFFER 2000000
#define ARTIST_BUFFER_NUMBER 4100

Person *person_map;
unsigned int *knows_map;
unsigned int *knows_map_reduced;
unsigned short *interest_map;
char *person_output_file, *interest_output_file, *knows_output_file,* person_birthday_output_file;
char* likedBy_output_file,  *new_knows_output_file, *new_person_output_file, *artists_output_file;
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
    unsigned long total_count = 0;
    unsigned short* interests_buffer  = malloc(sizeof(short) * interests_buffer_size);
    unsigned long interest_offset, interest_buffer_offset;
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


    unsigned int artist_start;
    unsigned int artist_end = 0;
    unsigned int artist_counts[ARTIST_BUFFER_NUMBER];
    unsigned int likedBy_buffer_sizes[ARTIST_BUFFER_NUMBER];
    unsigned int *likedBy_buffer[ARTIST_BUFFER_NUMBER];
    unsigned long total_count2 = 0;
    unsigned int initial_likedBy_buffer_size = 20;
    int i;
    for(i = 0; i < ARTIST_BUFFER_NUMBER; i++){
            likedBy_buffer_sizes[i] = initial_likedBy_buffer_size;
        }
    for(i = 0;i < ARTIST_BUFFER_NUMBER; i++){
            likedBy_buffer[i] = malloc(sizeof(int) * likedBy_buffer_sizes[i]);
        }

    int stop = 0;
    while(!stop){
        artist_counts[4] = 5;
        bzero(artist_counts, sizeof(int) * ARTIST_BUFFER_NUMBER);
        stop = 0;
        artist_start = artist_end;
        if(artist_end + ARTIST_BUFFER_NUMBER >= interests_count){
            artist_end = interests_count;
            stop = 1;
        }
        else{
            artist_end += ARTIST_BUFFER_NUMBER;
        }
        printf("START = %i , END = %i\n",artist_start,artist_end );
        Artist artists_buffer[ARTIST_BUFFER_NUMBER];
        for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
        {
            person = &person_map[person_offset];
            for (interest_offset = person->interests_first;
                interest_offset < person->interests_first + person->interest_n;
                interest_offset++){
                person_interest =  interest_map[interest_offset];
                found = 0;
                int j;
                for(j = 0; j < ARTIST_BUFFER_NUMBER && !found; j++){
                    if(person_interest == interests_buffer[artist_start+j]){
                        found = 1;

                        if(artist_counts[j] == likedBy_buffer_sizes[j])
                            {
                                //printf("doubling size of likedBy %i\n", j);
                                likedBy_buffer_sizes[j] *= 2;
                                likedBy_buffer[j] = realloc(likedBy_buffer[j],sizeof(int) * likedBy_buffer_sizes[j]);
                            }

                        likedBy_buffer[j][artist_counts[j]] = person_offset;
                        artist_counts[j]++;
                        }
                    }
                }
            }
            //ALLA FINE AGGIORNO TUTTI I CAMPI DEGLI ARTIST E LI BUTTO GIU SU DISCO, AGGIORNO ANCHE IL TOTAL_COUNT
            for(i = 0;i < ARTIST_BUFFER_NUMBER; i++){
                fwrite(likedBy_buffer[i], sizeof(int), artist_counts[i], likedBy);
                artists_buffer[i].likedBy_first = total_count2;
                total_count2 += artist_counts[i];
                artists_buffer[i].interest_id = interests_buffer[artist_start+i];
                artists_buffer[i].likedBy_n = artist_counts[i];
                fwrite(&artists_buffer[i], sizeof(Artist), 1, artists);
            }
        }

/*
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
                    count++;
                    total_count++;
                    fwrite(&person_offset, sizeof(int), 1, likedBy);
                }
            }

        }
        artist.likedBy_n = count;

        fwrite(&artist, sizeof(Artist), 1, artists);
    }*/
    fclose(artists);
    fclose(likedBy);
    return;
}

void reduce_person(Person* p1, Person_compact* p2){
    p2->person_id=p1->person_id;
    p2->birthday=p1->birthday;
    p2->knows_first= p1->knows_first;
    p2->knows_n=p1->knows_n;
}


void reorg_person(){
    Person *person, *knows;
    FILE *new_person = open_binout(new_person_output_file);

    printf("size %i\n", person_length/sizeof(Person));

    unsigned int person_offset;
    Person_compact person_copy;
    for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
    {
        //printf("in ciclo\n");
        person = &person_map[person_offset];
        reduce_person(person, &person_copy);
        // if(person_copy.person_id != person->person_id)
        //     printf("diversi\n");
        // if(person_copy.birthday != person->birthday)
        //     printf("b diversi\n");
        // if(person_copy.knows_first!=person->knows_first)
        //     printf("knows diversi\n");
        // if(person_copy.knows_n!=person->knows_n)
        //     printf("n diversi\n");
        fwrite(&person_copy, sizeof(Person_compact), 1, new_person);
    }
    fclose(new_person);
}


void reorg_person_birthday(){
    Person *person, *knows;

    unsigned int person_offset;
    Person_birthday person_copy;
    Person_birthday* birthdays=malloc((person_length / sizeof(Person)) * sizeof(Person_birthday));
    for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
    {
        person = &person_map[person_offset];
        person_copy.person_offset=person_offset;
        person_copy.birthday=person->birthday;
        birthdays[person_offset]=person_copy;
    }
    printf("offset %i\n",person_offset );
    qsort(birthdays, person_offset, sizeof(Person_birthday), &person_birthday_comparator);
    /*for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
    {
        Person_birthday* p=&birthdays[person_offset];
        printf("bday %hi, offset %i\n",p->birthday, p->person_offset );
    }*/
    FILE *person_birthday_file = open_binout(person_birthday_output_file);
    fwrite(birthdays,person_offset,sizeof(Person_birthday),person_birthday_file);
    fclose(person_birthday_file);
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
    person_map = (Person *)   mmaprw(makepath(path, "person_reduced",    "bin"), &person_length);
    likedBy_output_file = makepath(path, "likedBy", "bin");
    artists_output_file = makepath(path, "artists", "bin");
    //reorg_interests();
    new_person_output_file = makepath(argv[1], "person_compact_reduced", "bin");
    reorg_person();
    person_birthday_output_file = makepath(argv[1], "person_birthday", "bin");
    reorg_person_birthday();

    return 0;
}
