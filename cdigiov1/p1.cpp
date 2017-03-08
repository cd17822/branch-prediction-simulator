#include <iostream>
#include <fstream>
#include <string>

#define NUM_BIMODAL_TABLES 7
#define NUM_GSHARE_TABLES 9 // 3 through 11
#define GSHARE_MAP_SIZE 2048
#define TOURNAMENT_PREFERENCE_MAP_SIZE 2048
#define TOURNAMENT_BIMODAL_MAP_SIZE 2048
#define TOURNAMENT_GSHARE_MAP_SIZE 2048
#define TOURNAMENT_GSHARE_TABLE_BITS 11
#define TAKEN 1
#define STRONGLY_NOT_TAKEN 0
#define STRONGLY_TAKEN 3
#define PREFER_GSHARE 0
#define PREFER_BIMODAL 3

using namespace std;

int NUM_BRANCHES = 0;

int ALWAYS_TAKEN_CORRECT_COUNT = 0;

int NEVER_TAKEN_CORRECT_COUNT = 0;

int BIMODAL1_CORRECT_COUNT[NUM_BIMODAL_TABLES];
char BIMODAL1_MAP16[16];
char BIMODAL1_MAP32[32];
char BIMODAL1_MAP128[128];
char BIMODAL1_MAP256[256];
char BIMODAL1_MAP512[512];
char BIMODAL1_MAP1024[1024];
char BIMODAL1_MAP2048[2048];
char* BIMODAL1_MAP[] = { BIMODAL1_MAP16,
                         BIMODAL1_MAP32,
                         BIMODAL1_MAP128,
                         BIMODAL1_MAP256,
                         BIMODAL1_MAP512,
                         BIMODAL1_MAP1024,
                         BIMODAL1_MAP2048 };

int BIMODAL2_CORRECT_COUNT[NUM_BIMODAL_TABLES];
char BIMODAL2_MAP16[16];
char BIMODAL2_MAP32[32];
char BIMODAL2_MAP128[128];
char BIMODAL2_MAP256[256];
char BIMODAL2_MAP512[512];
char BIMODAL2_MAP1024[1024];
char BIMODAL2_MAP2048[2048];
char* BIMODAL2_MAP[] = { BIMODAL2_MAP16,
                         BIMODAL2_MAP32,
                         BIMODAL2_MAP128,
                         BIMODAL2_MAP256,
                         BIMODAL2_MAP512,
                         BIMODAL2_MAP1024,
                         BIMODAL2_MAP2048 };

int GSHARE_CORRECT_COUNT[NUM_GSHARE_TABLES];
int GSHARE_HISTORY[NUM_GSHARE_TABLES];
char GSHARE_MAP[NUM_GSHARE_TABLES][GSHARE_MAP_SIZE];

int TOURNAMENT_CORRECT_COUNT = 0;
char TOURNAMENT_BIMODAL_MAP[TOURNAMENT_BIMODAL_MAP_SIZE];
int TOURNAMENT_GSHARE_HISTORY;
char TOURNAMENT_GSHARE_MAP[TOURNAMENT_GSHARE_MAP_SIZE];
char TOURNAMENT_PREFERENCE_MAP[TOURNAMENT_PREFERENCE_MAP_SIZE];

int bimodalTableSizeAtIndex(int index) {
  return (index <= 1) ? 1 << (index + 4) : 1 << (index + 5);
}

void initBimodalTables() {
  for (int i = 0; i < NUM_BIMODAL_TABLES; ++i) {
    BIMODAL1_CORRECT_COUNT[i] = 0;
    BIMODAL2_CORRECT_COUNT[i] = 0;
    for (int j = 0; j < bimodalTableSizeAtIndex(i); ++j) {
      BIMODAL1_MAP[i][j] = TAKEN;
      BIMODAL2_MAP[i][j] = STRONGLY_TAKEN;
    }
  }
}

void initGshareTable() {
  for (int i = 0; i < NUM_GSHARE_TABLES; ++i) {
    GSHARE_CORRECT_COUNT[i] = 0;
    GSHARE_HISTORY[i] = 0;
    for (int j = 0; j < GSHARE_MAP_SIZE; ++j) {
      GSHARE_MAP[i][j] = STRONGLY_TAKEN;
    }
  }
}

void initTournamentTables() {
  for (int i = 0; i < TOURNAMENT_BIMODAL_MAP_SIZE; ++i) {
    TOURNAMENT_BIMODAL_MAP[i] = STRONGLY_TAKEN; // Strongly Taken
  }
  for (int i = 0; i < TOURNAMENT_GSHARE_MAP_SIZE; ++i) {
    TOURNAMENT_GSHARE_MAP[i] = STRONGLY_TAKEN;
  }
  for (int i = 0; i < TOURNAMENT_PREFERENCE_MAP_SIZE; ++i) {
    TOURNAMENT_PREFERENCE_MAP[i] = PREFER_GSHARE;
  }
}

void incrementNumBranches() {
  ++NUM_BRANCHES;
}

void alwaysTaken(unsigned long long& addr, char behavior_bit) {
  ALWAYS_TAKEN_CORRECT_COUNT += (behavior_bit == 1);
}

void neverTaken(unsigned long long& addr, char behavior_bit) {
  NEVER_TAKEN_CORRECT_COUNT += (behavior_bit == 0);
}

void bimodal1(unsigned long long& addr, char behavior_bit) {
  for (int table_index = 0; table_index < NUM_BIMODAL_TABLES; ++table_index) {
    // fetch info
    int table_size = bimodalTableSizeAtIndex(table_index);
    char behavior_state = BIMODAL1_MAP[table_index][addr % table_size];

    // check for correct prediction
    BIMODAL1_CORRECT_COUNT[table_index] += (behavior_bit == behavior_state);

    // update map
    BIMODAL1_MAP[table_index][addr % table_size] = behavior_bit;
  }
}

void bimodal2(unsigned long long& addr, char behavior_bit) {
  for (int table_index = 0; table_index < NUM_BIMODAL_TABLES; ++table_index) {
    // fetch info
    int table_size = bimodalTableSizeAtIndex(table_index);
    char behavior_state = BIMODAL2_MAP[table_index][addr % table_size];

    // check for correct prediction
    BIMODAL2_CORRECT_COUNT[table_index] += (behavior_bit == 1 && behavior_state >= 2) ||
                                           (behavior_bit == 0 && behavior_state <= 1);

    // update map
    if (behavior_state != STRONGLY_NOT_TAKEN && behavior_bit == 0) --behavior_state;
    if (behavior_state != STRONGLY_TAKEN     && behavior_bit == 1) ++behavior_state;
    BIMODAL2_MAP[table_index][addr % table_size] = behavior_state;
  }
}

void gshare(unsigned long long& addr, char behavior_bit) {
  for (int table_index = 0; table_index < NUM_GSHARE_TABLES; ++table_index) {
    // fetch info
    int table_bits = table_index + 3;
    int map_index = GSHARE_HISTORY[table_index] ^ (addr % GSHARE_MAP_SIZE);
    char behavior_state = GSHARE_MAP[table_index][map_index];

    // check for correct prediction
    GSHARE_CORRECT_COUNT[table_index] += (behavior_bit == 1 && behavior_state >= 2) ||
                                         (behavior_bit == 0 && behavior_state <= 1);

    // update map
    if (behavior_state != STRONGLY_NOT_TAKEN && behavior_bit == 0) --behavior_state;
    if (behavior_state != STRONGLY_TAKEN     && behavior_bit == 1) ++behavior_state;
    GSHARE_MAP[table_index][map_index] = behavior_state;

    // update history
    GSHARE_HISTORY[table_index] = GSHARE_HISTORY[table_index] << 1;
    GSHARE_HISTORY[table_index] = GSHARE_HISTORY[table_index] & ((1 << table_bits) - 1);
    GSHARE_HISTORY[table_index] = GSHARE_HISTORY[table_index] + behavior_bit;
  }
}

void tournament(unsigned long long& addr, char behavior_bit) {
  // fetch bimodal info
  int bimodal_map_index = addr % TOURNAMENT_BIMODAL_MAP_SIZE;
  char bimodal_behavior_state = TOURNAMENT_BIMODAL_MAP[bimodal_map_index];
  bool bimodal_is_correct = (behavior_bit == 1 && bimodal_behavior_state >= 2) ||
                            (behavior_bit == 0 && bimodal_behavior_state <= 1);

  // fetch gshare info
  int gshare_map_index = TOURNAMENT_GSHARE_HISTORY ^ (addr % TOURNAMENT_GSHARE_MAP_SIZE);
  char gshare_behavior_state = TOURNAMENT_GSHARE_MAP[gshare_map_index];
  bool gshare_is_correct = (behavior_bit == 1 && gshare_behavior_state >= 2) ||
                           (behavior_bit == 0 && gshare_behavior_state <= 1);

  // fetch preference info
  int preference_map_index = addr % TOURNAMENT_PREFERENCE_MAP_SIZE;
  char preference = TOURNAMENT_PREFERENCE_MAP[preference_map_index];

  // check for correct prediction
  if (preference <= 1) TOURNAMENT_CORRECT_COUNT += gshare_is_correct;
  else                 TOURNAMENT_CORRECT_COUNT += bimodal_is_correct;

  // update preference map
  if (gshare_is_correct && !bimodal_is_correct && preference != PREFER_GSHARE ) --preference;
  if (!gshare_is_correct && bimodal_is_correct && preference != PREFER_BIMODAL) ++preference;
  TOURNAMENT_PREFERENCE_MAP[preference_map_index] = preference;

  // update bimodal map
  if (bimodal_behavior_state != STRONGLY_NOT_TAKEN && behavior_bit == 0) --bimodal_behavior_state;
  if (bimodal_behavior_state != STRONGLY_TAKEN     && behavior_bit == 1) ++bimodal_behavior_state;
  TOURNAMENT_BIMODAL_MAP[bimodal_map_index] = bimodal_behavior_state;

  // update gshare map
  if (gshare_behavior_state != STRONGLY_NOT_TAKEN && behavior_bit == 0) --gshare_behavior_state;
  if (gshare_behavior_state != STRONGLY_TAKEN     && behavior_bit == 1) ++gshare_behavior_state;
  TOURNAMENT_GSHARE_MAP[gshare_map_index] = gshare_behavior_state;

  // update gshare history
  TOURNAMENT_GSHARE_HISTORY = TOURNAMENT_GSHARE_HISTORY << 1;
  TOURNAMENT_GSHARE_HISTORY = TOURNAMENT_GSHARE_HISTORY & ((1 << TOURNAMENT_GSHARE_TABLE_BITS) - 1);
  TOURNAMENT_GSHARE_HISTORY = TOURNAMENT_GSHARE_HISTORY + behavior_bit;
}

void printCorrectCountsOf(int* count_table, int num_counts, ofstream& outfile) {
  for (int i = 0; i < num_counts; ++i) {
    outfile << count_table[i] << "," << NUM_BRANCHES;
    if (i < num_counts - 1) {
      outfile << "; ";
    }else{
      outfile << ";" << endl;
    }
  }
}

void outputResults(ofstream& outfile) {
  printCorrectCountsOf(&ALWAYS_TAKEN_CORRECT_COUNT, 1, outfile);
  printCorrectCountsOf(&NEVER_TAKEN_CORRECT_COUNT, 1, outfile);
  printCorrectCountsOf(BIMODAL1_CORRECT_COUNT, NUM_BIMODAL_TABLES, outfile);
  printCorrectCountsOf(BIMODAL2_CORRECT_COUNT, NUM_BIMODAL_TABLES, outfile);
  printCorrectCountsOf(GSHARE_CORRECT_COUNT, NUM_GSHARE_TABLES, outfile);
  printCorrectCountsOf(&TOURNAMENT_CORRECT_COUNT, 1, outfile);
}

int main(int argc, char *argv[]) {
  // check for correct number of args
  if (argc != 3) {
    fprintf(stderr, "Incorrect number of arguments.\n");
    exit(1);
  }

  // init tables
  initBimodalTables();
  initGshareTable();
  initTournamentTables();

  // Temporary variables
  unsigned long long addr;
  string behavior;
  char behavior_bit;

  // Open file for reading
  ifstream infile(argv[1]);
  ofstream outfile(argv[2]);

  // The following loop will read a hexadecimal number and
  // a string each time and then output them
  while(infile >> std::hex >> addr >> behavior) {
    behavior_bit = (behavior == "T");
    incrementNumBranches();
    alwaysTaken(addr, behavior_bit);
    neverTaken(addr, behavior_bit);
    bimodal1(addr, behavior_bit);
    bimodal2(addr, behavior_bit);
    gshare(addr, behavior_bit);
    tournament(addr, behavior_bit);
  }

  outputResults(outfile);

  return 0;
}
