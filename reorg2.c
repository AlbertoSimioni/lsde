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
#define LOCATION_BUFFER 600
#define INTEREST_BUFFER 1000

Person *person_map;
unsigned int *knows_map;
unsigned int *knows_map_reduced;
unsigned short *interest_map;
unsigned long person_length, knows_length, interest_length;
unsigned long loc_size;
unsigned short *location_map;
unsigned short *visited_locations;


//This function looks at all the locations where the people in the person_map live and create an array of unique locations called visited_locations and one that contains all the locations for each person (for faster access)
void save_locations()
{
    Person *person;
    unsigned int person_offset;
    int count = 0, found = 0; //count is the number of locations found
                            // found is a boolean variable
    short location;

    location_map = malloc(sizeof(short) * (person_length / sizeof(Person)));
    loc_size = LOCATION_BUFFER; //we set a variable size to the array of locations 
    visited_locations = malloc(sizeof(short) * loc_size);

    //runs through the person_map and add a location to the visited_locations array each time a new one is found
    for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
    {
        person = &(person_map[person_offset]);
        location = person->location;

        //we also save the location of each person in the location_map array
        location_map[person_offset] = location;

        found = 0;
        int i;
        for(i = 0; i < count && !found; ++i)
        {
            if(location == visited_locations[i])
            {
                found = 1; //found a new one
            }
        }
        if(!found)
        {
            visited_locations[count] = location;
            ++count;
            if(count == loc_size)
            {
                loc_size = loc_size * 2;
                visited_locations = realloc(visited_locations, loc_size * sizeof(short));
            }
        }
    }
    loc_size = count;
    //compact the visited locations array to the size of the elements it contains
    visited_locations = realloc(visited_locations, loc_size * sizeof(short));
}

//Removes from the knows table all the relationship between people that lives in different cities and updates accordingly the person_map
void reorg_location(char* path, char *knows_output_file, char * person_output_file)
{
    //offsets in the person_map
    unsigned int person_offset, knows_pos;
    //offset in the knows_map
    unsigned long knows_offset;

    unsigned short count;
    //keeps track of the position where we are writing on the new knows file
    unsigned long total_count = 0;

    Person *person, *knows;
    FILE *knows_file = open_binout(makepath(path, knows_output_file, "bin"));
    FILE *person_file = open_binout(makepath(path, person_output_file, "bin"));

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

            if(location_map[person_offset] != location_map[knows_pos]) continue;
            count++;
            total_count++;
            fwrite(&knows_pos, sizeof(int), 1, knows_file);
        }

        person_copy.knows_n = count;
        fwrite(&person_copy, sizeof(Person), 1, person_file);
       
    }
    fclose(person_file);
    fclose(knows_file);
    munmap(person_map, person_length);
    munmap(knows_map, knows_length);
    //open the correct person and knows map
    person_map   = (Person *)       mmapr(makepath(path, person_output_file,   "bin"), &person_length);
    knows_map    = (unsigned int *) mmapr(makepath(path, knows_output_file,    "bin"), &knows_length);

    return;
}


void reorg_location_mutual(char *path, char *knows_output_file, char *person_output_file)
{
    //offsets in the person_map
    unsigned int person_offset, knows_pos;
    //offsets in the knows map
    unsigned long knows_offset, knows_offset2;
    unsigned short count;
    unsigned int person_count=0;
    unsigned long total_count = 0;
    //buffer containing the new offsets of person_map 
    unsigned int *offests_buffer=malloc(sizeof(int) *person_length / sizeof(Person));

    Person *person, *knows;
    FILE *knows_file = open_binout(makepath(path, "knows_temp", "bin"));
    FILE *person_file = open_binout(makepath(path, person_output_file, "bin"));

    Person person_copy;
    person_count=0;
    for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
    {
        person = &person_map[person_offset];

        person_copy = *person;
        person_copy.knows_first = total_count;

        //count how many of the friends are mutual
        count = 0;
        // check if friend lives in same city and likes artist
        for (knows_offset = person->knows_first;
                knows_offset < person->knows_first + person->knows_n;
                knows_offset++)
        {
            //this is person's friend, let's check if it's mutual
            knows_pos = knows_map[knows_offset];
            knows = &person_map[knows_pos];
            //search between the friend's knows to find the first person
            for (knows_offset2 = knows->knows_first;
                    knows_offset2 < knows->knows_first + knows->knows_n;
                    knows_offset2++)
            {
                //found a mutual friendship
                if(knows_map[knows_offset2] == person_offset){
                    count++;
                    total_count++;
                    fwrite(&knows_pos, sizeof(int), 1, knows_file);
                }
            }
        }

        person_copy.knows_n = count;
         if(count>0){ //only write a person with at least one mutual friendship
            fwrite(&person_copy, sizeof(Person), 1, person_file);
            offests_buffer[person_offset]=person_count;
            ++person_count;
        }

    }
    fclose(knows_file);
    fclose(person_file);

    knows_map_reduced    = (unsigned int *)   mmapr(makepath(path, "knows_temp",    "bin"), &knows_length);
    knows_file = open_binout(makepath(path, knows_output_file, "bin"));
    //substitute in the knows file the updated offsets
    for(knows_offset=0; knows_offset<knows_length/sizeof(int); ++knows_offset)
     {
        fwrite(&offests_buffer[knows_map_reduced[knows_offset]], sizeof(int), 1, knows_file);
    }
    munmap(person_map, person_length);
    person_map = (Person *) mmapr(makepath(path, person_output_file, "bin"), &person_length);

    return;
}


void reorg_interests(char *path, char* liked_file, char* artists_file)
{
    //CODE FOR CREATING THE likedBy table and to create the new interests table
    FILE *artists = open_binout(makepath(path, artists_file, "bin"));
    FILE *likedBy = open_binout(makepath(path, liked_file, "bin"));
    unsigned int interests_buffer_size = INTEREST_BUFFER;
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

    fclose(artists);
    fclose(likedBy);
    return;
}

//transform a Person in Person_compact by removing not useful fields
void reduce_person(Person* p1, Person_compact* p2){
    p2->person_id=p1->person_id;
    p2->knows_first= p1->knows_first;
    p2->knows_n=p1->knows_n;
}

//remove field birthday, interest_first and interest_n from each Person in the person file
void reorg_person(char *path, char *person_file){
    Person *person, *knows;
    FILE *new_person = open_binout(makepath(path, person_file, "bin"));

    unsigned int person_offset;
    Person_compact person_copy;
    for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
    {
        person = &person_map[person_offset];
        reduce_person(person, &person_copy);

        fwrite(&person_copy, sizeof(Person_compact), 1, new_person);
    }
    fclose(new_person);
}

//order the person by birthday and write them in a new file
void reorg_person_birthday(char *path, char *birthday_file){
    
    FILE *person_birthday_file = open_binout(makepath(path, birthday_file, "bin"));
    
    Person *person;
    unsigned int person_offset;
    Person_birthday person_copy;
    //buffer containing each person (offset) and its birthday
    Person_birthday* birthdays=malloc((person_length / sizeof(Person)) * sizeof(Person_birthday));
    for (person_offset = 0; person_offset < person_length / sizeof(Person); person_offset++)
    {
        person = &person_map[person_offset];
        person_copy.person_offset=person_offset;
        person_copy.birthday=person->birthday;
        birthdays[person_offset]=person_copy;
    }
    //sorting the person by birthday (from the erliest)
    qsort(birthdays, person_length / sizeof(Person), sizeof(Person_birthday), &person_birthday_comparator);

    fwrite(birthdays,person_offset,sizeof(Person_birthday),person_birthday_file);
    fclose(person_birthday_file);
}

int main(int argc, char *argv[])
{
    /* memory-map files created by loader */
    person_map   = (Person *)         mmapr(makepath(argv[1], "person",   "bin"), &person_length);
    knows_map    = (unsigned int *)   mmapr(makepath(argv[1], "knows",    "bin"), &knows_length);
    interest_map = (unsigned short *) mmapr(makepath(argv[1], "interest", "bin"), &interest_length);

    save_locations();

    reorg_location(argv[1], "person_location", "knows_location"); //delete all friendships from knows file of friends in different cities

    reorg_location_mutual(argv[1], "knows_mutual", "person_mutual");

    reorg_interests(argv[1], "likedBy", "artists");
    
    reorg_person_birthday(argv[1], "birthday");

    reorg_person(argv[1], "person_compact_reduced");


    return 0;
}
