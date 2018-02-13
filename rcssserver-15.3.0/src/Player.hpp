#ifndef CLIENT_HPP
#include "client.hpp"
#endif

#include "Sexpr.hpp"

class Player : public Client {
public:
  float X,Y,Dir;
  float tX,tY; // target
  float targets[10];
  int ntargets,tid;

  Player( const std::string & server, const int port );
  void parseMsg( const char * msg, const size_t len );
  int infer_player_loc(Sexpr* expr,float* x,float* y,float* dir);
  void player_initialization();
  void decide_what_to_do();
};

