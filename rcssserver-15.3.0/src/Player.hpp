#ifndef CLIENT_HPP
#include "client.hpp"
#endif

#include "Sexpr.hpp"

class Player : public Client {
public:
  float X,Y,Hdg; // Hdg is heading in degrees, 0=to the right
  float ballX,ballY; // set to 999 if can't see
  float ballRelDist,ballRelDir;
  float tX,tY; // target
  float targets[10];
  int ntargets,tid;
  int stage;

  Player( const std::string & server, const int port );
  void parseMsg( const char * msg, const size_t len );
  void update_ball_location(Sexpr* expr);
  int infer_player_loc(Sexpr* expr,float* x,float* y,float* dir);
  void player_initialization();
  void decide_what_to_do();
  void run_trianglar_path();
};

