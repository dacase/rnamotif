#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#define MAXLINE 500
#define MAXSEQ 500

struct linked_list  {
  float score;
  int seq_num;
  char title[MAXLINE];
  char seq[MAXSEQ];
  struct linked_list *next;
};

void extract_id(char phrase[]);

int main(int argc, char **argv)
{

  int compare_entry, low, junk, tmp_seq_num, previous_seq_num;
  struct linked_list *head, *entry;
  char line[MAXLINE], tmp_line[150], firstline[MAXLINE];
  char previous_id[50], id[50];
  float tmp_score;
  FILE *rnamot;
  
  rnamot = fopen(argv[1], "r");

  low = 0;
  if (argc == 3)  {
    if (strcmp(argv[2], "low") == 0)  {
      low = 1;
    }
  }

  fgets(line, MAXLINE, rnamot);
  printf ("%s", line);
  fgets(line, MAXLINE, rnamot);
  printf ("%s", line);
  fgets(line, MAXLINE, rnamot);
  entry = (struct linked_list *)calloc(1,sizeof(struct linked_list));
  strcpy(firstline, line);

  head = (struct linked_list *)calloc(1,sizeof(struct linked_list));
  head -> score = 999;
  head -> seq_num = 0;
  head -> title[0] = '\0';
  head -> seq[0] = '\0';
  head -> next = NULL;

  fgets(line, MAXLINE, rnamot);
  sscanf(line, "%s %f %d %d", tmp_line, &tmp_score, &junk, &tmp_seq_num);
  strcpy(entry -> title, tmp_line);
  entry -> score = tmp_score;
  entry -> seq_num = tmp_seq_num;
  strcpy(entry -> seq, line);
  strcpy(previous_id, entry -> title);
  previous_seq_num = entry -> seq_num;
  entry -> next = NULL;
  head -> next = entry;

  while (fgets(line, MAXLINE, rnamot) != NULL)  {
    if (line[0] == '>')  {
/* skip this section
      sscanf(line, "%s", id);
      if ((strcmp(id, previous_id) == 0)  {
        compare_entry = 1;
      }
      else  {
        entry -> next = (struct linked_list *)calloc(1,sizeof(struct linked_list));
        strcpy(entry -> next -> title, line);
        compare_entry = 0;
        strcpy(previous_id, id);
      }
*/
    }
    else  {
/* second line */
      sscanf(line, "%s %f %d %d", tmp_line, &tmp_score, &junk, &tmp_seq_num);
      if ((strcmp(tmp_line, previous_id) != 0) ||
          (tmp_seq_num < (previous_seq_num - 2)) ||
          (tmp_seq_num > (previous_seq_num + 2)))  {
/* second line of new entry */
        entry -> next = (struct linked_list *)calloc(1,sizeof(struct linked_list));
        strcpy(entry -> next -> title, tmp_line);
        entry -> next -> score = tmp_score;
        entry -> next -> seq_num = tmp_seq_num;
        strcpy(entry -> next -> seq, line);
        entry -> next -> next = NULL;
        entry = entry -> next;
        strcpy(previous_id, entry -> title);
        previous_seq_num = entry -> seq_num;
      }
      else  {
/* second line of multiple entry */
        if (low == 0)  {
/* retain highest score */
          if (tmp_score > entry -> score)  {
            entry -> score = tmp_score;
            strcpy(entry -> seq, line);
          }
        }
        else  {
/* retain lowest score */
          if (tmp_score < entry -> score)  {
            entry -> score = tmp_score;
            strcpy(entry -> seq, line);
          }
        }
      }
    }
  }
  fclose(rnamot);

  head = head -> next;
  while (head != NULL)  {
    strcpy(tmp_line, head -> seq);
/*    extract_id(tmp_line);
    printf("%s", tmp_line); */
    printf("%s", firstline);
    printf("%s", head -> seq);
    head = head -> next;
  }
}

void extract_id(char phrase[])
{
  int i, count, num_partition;
  char tmp[150];

  i = 0;
  count = 0;
  num_partition = 0;
  while (phrase[i] != '\0')  {
    if (phrase[i] == '|')  {
      num_partition++;
    }
    else if (num_partition == 4)  {
      tmp[count] = phrase[i];
      count++;
    }
    i++;
  }
  tmp[count] = '\0';
  strcpy(phrase, tmp);
}
