/* 
 * paradox.c for CS 537 Fall 2014 p1
 * by Sean Morton
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NUM_TRIALS 1000

double compute_probability(int n); 
int has_duplicates(int ary[], int size);

int main(int argc, char* argv[]) {

    if (argc != 5) {
        fprintf(stderr, "Usage: paradox -i inputfile -o outputfile\n");
        exit(1);
    }

    char *infile;
    char *outfile;
    int flag;

    opterr = 0; 
    while ((flag = getopt(argc, argv, "i:o:")) != -1) {
        switch(flag) {
            case 'i':
                infile = optarg;
                break;
            case 'o':
                outfile = optarg;
                break;
            default: 
                fprintf(stderr, "Usage: paradox -i inputfile -o outputfile\n"); 
                exit(1);
                
        }
    }    
        
    FILE *ifp = fopen(infile, "r");
    if (ifp == NULL) {
        fprintf(stderr, "Error: Cannot open file %s\n", infile);
        exit(1);
    }

    FILE *ofp = fopen(outfile, "w");
    if (ofp == NULL) {
        fprintf(stderr, "Error: Cannot open file %s\n", outfile);
        exit(1);
    }
 
    /* Read infile and output the probability of N people to outfile */   
    int n;
    while (fscanf(ifp, "%d", &n) != EOF) {
        fprintf(ofp, "%.2f\n", compute_probability(n));
    }

    fclose(ifp);
    fclose(ofp);
    
    exit(0);

}

/*
 * Computes the probability of two people having the same birthday 
 * given N people in the room
 */
double compute_probability(int n) {
    int positive_trials;
    int bdays[n];
    int i, j;
    
    srand(time(NULL));
    positive_trials = 0;
    for (i = 0; i < NUM_TRIALS; i++) {

        for (j = 0; j < n; j++) {
            bdays[j] = (rand() % 365) + 1;
        }
    
        if (has_duplicates(bdays, n) == 1) {
            positive_trials++;
        }
    }
    return (double)positive_trials/NUM_TRIALS;
}

/*
 * Tests to see if ary has any duplicates contained within it.
 * Return 1 if true; 0 if false
 */
int has_duplicates(int ary[], int size) {
    int i, j; 
    for (i = 0; i < size; i++) {
        for (j = i+1; j < size; j++) {
            if (ary[i] == ary[j]) {
                return 1; 
            }
        }
    }
    return 0;
}
