#include "Sexpr.hpp"
#include <vector>
#include <stdio.h>
#include <string.h>

using namespace std;

Sexpr::Sexpr(vector<char*>* toks,int i)
{
  start = i;
  if (toks->at(i)[0]=='(')
  {
    leaf = NULL;
    int j=i+1;
    while (toks->at(j)[0]!=')')
    {
      Sexpr* child=new Sexpr(toks,j);
      children.push_back(child);
      j = child->next;
    }
    next = j+1;
  }
  else
  {
    leaf = toks->at(i);
    next = i+1;
  }
}
  
void Sexpr::print_indented(int indent)
{
  if (leaf!=NULL)
  {
    for (int i=0 ; i<indent ; i++) printf(" ");
    printf("%s\n",leaf);
  }
  else for (int i=0 ; i<children.size() ; i++) children[i]->print_indented(indent+2);
}

void Sexpr::print_parenthesized()
{
  if (leaf!=NULL) printf("%s ",leaf);
  else
  {
    printf("(");
    for (int i=0 ; i<children.size() ; i++) children[i]->print_parenthesized();
    printf(") ");
  }
}

vector<char*>* tokenize(char* s)
{
  int i=0,n=strlen(s);
  vector<char*>* toks = new vector<char*>();
  while (i<n)
  {
    if (s[i]=='(' || s[i]==')') { toks->push_back(strndup(s+i,1)); i++; }
    else if (s[i]==' ') i++;
    else if (s[i]!=' ')
    {
      int j=i;
      while (j<n && s[j]!='(' && s[j]!=')' && s[j]!=' ') j++;
      if (j==n) break;
      toks->push_back(strndup(s+i,j-i));
      i = j;
    }
  }
  return toks;
}

