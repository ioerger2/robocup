#include <vector>

using namespace std;

// S-expressions are nested lists of symbols
// Sexpr class represents a tree structure
// everything is a leaf node (string) or an internal node, which is list of pointers to subtrees
// right now, this code assumes input is synactically legal, and doesn't do any error checking
// to do: one should probably implement destructors to avoid memory leaks...

class Sexpr {
public:
  char* leaf; // set to NULL if not a leaf node
  int start,next; // index in token list
  vector<Sexpr*> children;
  Sexpr(vector<char*>* toks,int i);
  void print_indented(int indent);
  void print_parenthesized();
};

vector<char*>* tokenize(char* s);
