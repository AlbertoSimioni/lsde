#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "utils.h"

#define QUERY_FIELD_QID 0
#define QUERY_FIELD_A1 1
#define QUERY_FIELD_A2 2
#define QUERY_FIELD_A3 3
#define QUERY_FIELD_A4 4
#define QUERY_FIELD_BS 5
#define QUERY_FIELD_BE 6

Person_compact *person_map;
Person_birthday *person_birthday_map;
unsigned int *knows_map;
unsigned char *scores_map; //This map isn't a mmap file but is an array populated in the precomputation
char *path; //Path to the folder with the data
Artist *artists_map;
unsigned int *liked_map;


unsigned long person_length, knows_length, score_length, artists_length, liked_length, person_birthday_length;

FILE *outfile;

//Binary search in the person_birthday_map, the map is ordered with the smallest birthday at the sttart of the array
//In the map there can be multiple entrys with the same birthday value, the function return always the index of the
//first element with that value.
int binsearch(int start, int end, short value)
{
    int middle;
    // Loop while the interval of possible matches is non-empty
    while (end > start)
    {
        middle = start + (end - start) / 2;

        if(person_birthday_map[middle].birthday < value)
        {
            start = middle + 1; // Value in (middle, end)
        }
        else if(person_birthday_map[middle].birthday > value)
        {
            end = middle; // Value in [start, middle)
        }
        else
        {
            //value founded, we now search for the first element with the same value in the map
            //by reducing the middle index
            while(middle >= start && person_birthday_map[middle].birthday == value)
            {
                --middle;
            }
            if(middle < start)
            {
                return start;
            }
            else
            {
                return middle + 1;
            }
        }
    }
    return start + 1;
}


int result_comparator(const void *v1, const void *v2)
{
    Result *r1 = (Result *) v1;
    Result *r2 = (Result *) v2;
    if (r1->score > r2->score)
        return -1;
    else if (r1->score < r2->score)
        return +1;
    else if (r1->person_id < r2->person_id)
        return -1;
    else if (r1->person_id > r2->person_id)
        return +1;
    else if (r1->knows_id < r2->knows_id)
        return -1;
    else if (r1->knows_id > r2->knows_id)
        return +1;
    else
        return 0;
}

//Precomputes the scores values storing them inside the scores_map. The function uses the artist_map and the likedBy_map
//That are created in the reorg phase.
void save_scores(unsigned short artist_id, unsigned short areltd[])
{
    unsigned int artist_offset; //offset in the artist_map
    unsigned long liked_offset; //offset in the likedBy_map
    Artist *artist;

    //for each person there is an entry with his score
    //if the score = 4, the person likes A1 (artist_id)
    scores_map = malloc(sizeof(char) * (person_length / sizeof(Person_compact)));
    bzero(scores_map, sizeof(char) * (person_length / sizeof(Person_compact)));

    //iterating through the artist_map only one time
    for (artist_offset = 0;
            artist_offset < artists_length / sizeof(Artist);
            artist_offset++)
    {

        artist = &artists_map[artist_offset];
        if(artist->interest_id == artist_id) //true: current artist = A1
        {
            //Assigns the value 4 to all the people that likes A1
            for(liked_offset = artist->likedBy_first;
                    liked_offset < artist->likedBy_first + artist->likedBy_n;
                    liked_offset++)
            {
                scores_map[liked_map[liked_offset]] = 4;
            }
        }

        //true: current artist = A2 or A3 or A4
        if(areltd[0] == artist->interest_id || areltd[1] == artist->interest_id || areltd[2] == artist->interest_id)
        {
            //Increament the score by 1 to all the people that likes the artist
            for(liked_offset = artist->likedBy_first;
                    liked_offset < artist->likedBy_first + artist->likedBy_n;
                    liked_offset++)
            {
                if(scores_map[liked_map[liked_offset]] != 4)
                    scores_map[liked_map[liked_offset]]++;
            }
        }
    }
}

void query(unsigned short qid, unsigned short artist, unsigned short areltd[], unsigned short bdstart, unsigned short bdend)
{
    unsigned int person_offset, knows_pos, offset; //offsets in person_map
    unsigned long knows_offset, knows_offset2; //offsets in knows_map

    Person_compact *person, *knows;
    unsigned char score;

    unsigned int result_length = 0, result_idx, result_set_size = 1000;
    Result *results = malloc(result_set_size * sizeof (Result));


    Person_birthday pbday;
    pbday.birthday = bdstart;
    int person_num = person_birthday_length / sizeof(Person_birthday);

    int start_pos = binsearch(0, person_num, bdstart); //retrieving the first person that is inside the bdstart,bdend interval

    for(offset = start_pos; offset < person_num && person_birthday_map[offset].birthday <= bdend; offset++)
    {

        score = scores_map[person_birthday_map[offset].person_offset];
        if (score == 0 || score == 4) continue; //score = 4 means that person likes A1

        person = &person_map[person_birthday_map[offset].person_offset];

        //iterating mutual friendships
        for (knows_offset = person->knows_first;
                knows_offset < person->knows_first + person->knows_n;
                knows_offset++)
        {
            knows_pos = knows_map[knows_offset];
            knows = &person_map[knows_pos];

            // check if friend likes A1
            if (scores_map[knows_pos] != 4) continue;
            // realloc result array if we run out of space
            if (result_length >= result_set_size)
            {
                result_set_size *= 2;
                results = realloc(results, result_set_size * sizeof (Result));
            }
            results[result_length].person_id = person->person_id;
            results[result_length].knows_id = knows->person_id;
            results[result_length].score = score;
            result_length++;
        }
    }


    // sort result
    qsort(results, result_length, sizeof(Result), &result_comparator);

    // output
    for (result_idx = 0; result_idx < result_length; result_idx++)
    {
        fprintf(outfile, "%d|%d|%lu|%lu\n", qid, results[result_idx].score,
                results[result_idx].person_id, results[result_idx].knows_id);
    }

}

void query_line_handler(unsigned char nfields, char **tokens)
{
    unsigned short q_id, q_artist, q_bdaystart, q_bdayend;
    unsigned short q_relartists[3];

    q_id            = atoi(tokens[QUERY_FIELD_QID]);
    q_artist        = atoi(tokens[QUERY_FIELD_A1]);
    q_relartists[0] = atoi(tokens[QUERY_FIELD_A2]);
    q_relartists[1] = atoi(tokens[QUERY_FIELD_A3]);
    q_relartists[2] = atoi(tokens[QUERY_FIELD_A4]);
    q_bdaystart     = birthday_to_short(tokens[QUERY_FIELD_BS]);
    q_bdayend       = birthday_to_short(tokens[QUERY_FIELD_BE]);

    //This two maps are used only in the precomputation, they are opened here to be able to close them after the precomputation
    artists_map = (Artist *)  mmapr(makepath(path, "artists",    "bin"), &artists_length);
    liked_map = (unsigned int *)  mmapr(makepath(path, "likedBy",    "bin"), &liked_length);

    save_scores(q_artist, q_relartists);

    //Closing the maps
    munmap(artists_map, artists_length);
    munmap(liked_map, liked_length);

    query(q_id, q_artist, q_relartists, q_bdaystart, q_bdayend);

}



int main(int argc, char *argv[])
{

    if (argc < 4)
    {
        fprintf(stderr, "Usage: [datadir] [query file] [results file]\n");
        exit(1);
    }
    path=argv[1];

    person_map   = (Person_compact *)    mmapr(makepath(argv[1], "person_compact",   "bin"), &person_length);
    knows_map    = (unsigned int *)   mmapr(makepath(argv[1], "knows_mutual",    "bin"), &knows_length);
    person_birthday_map = (Person_birthday *) mmapr(makepath(argv[1], "birthday", "bin"), &person_birthday_length);

    outfile = fopen(argv[3], "w");
    if (outfile == NULL)
    {
        fprintf(stderr, "Can't write to output file at %s\n", argv[3]);
        exit(-1);
    }

    parse_csv(argv[2], &query_line_handler);
    return 0;
}
