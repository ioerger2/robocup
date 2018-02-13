#include "Player.hpp"
#include <math.h>

#define PI 3.1415927

float abs(float X) { return (X>=0) ? X : -X; }

float dist(float X1,float Y1,float X2,float Y2) { return sqrt(pow(X1-X2,2)+pow(Y1-Y2,2)); }

float angdiff(float A,float B)
{
  float D=A-B;
  if (D>=180) D -= 360;
  if (D<=-180) D += 360;
  return D;
}


//------------------------------------------------------------------------------

Player::Player( const std::string & server, const int port ) : Client(server,port) {}
 
void Player::parseMsg( const char * msg, const size_t len )
{
  Client::parseMsg(msg,len); // let parent class do its work

  Sexpr* expr=new Sexpr(tokenize((char*)msg),0);
  float x,y,dir;
  int stat=infer_player_loc(expr,&x,&y,&dir);

  if (stat==0) 
  {
    printf("inferred player location: (%0.1f,%0.1f), dir=%0.1f\n",x,y,dir);
    X = x; Y = y; Hdg = dir; // update state variables
  }

  update_ball_location(expr);

  //delete expr;
}

void Player::update_ball_location(Sexpr* expr)
{
  if (strcmp(expr->children[0]->leaf,"see")!=0) return;
  ballX = ballY = 999; // unknown, update only if I see it
  // maybe I should try to remember the last place I saw it...

  // loop through objects
  for (int i=2 ; i<expr->children.size() ; i++) 
  {
    Sexpr* obj=expr->children[i]->children[0];
    if (obj->children.size()==1 && obj->children[0]->leaf[0]=='b')
    {
      float dist=atof(expr->children[i]->children[1]->leaf);
      float relang=atof(expr->children[i]->children[2]->leaf);
      float actang=Hdg-relang; // I am not sure this is correct...
      if (actang>=180) actang -= 360;
      if (actang<=-180) actang += 360;
printf("directions: %f %f %f\n",Hdg,relang,actang);
      ballX = X+dist*cos(actang);
      ballY = Y+dist*sin(actang);
      ballRelDist = dist;
      ballRelDir = relang;
printf("updating ball location: (%0.1f,%0.1f), dist=%0.1f, reldir=%0.1f\n",ballX,ballY,ballRelDir,ballRelDir);
    }
  }
}

// fills in vals for x,y,dir; returns -1 if fails for any reason
  
int Player::infer_player_loc(Sexpr* expr,float* x,float* y,float* dir) // add verbose flag?
    {
      if (strcmp(expr->children[0]->leaf,"see")!=0) return -1;
      typedef vector<float> socFlag; // x,y,dist,dir
      vector<socFlag> flags;
    
      // loop through objects and find border flags...
      for (int i=2 ; i<expr->children.size() ; i++) 
      {
        printf("see: time=%s object=",expr->children[1]->leaf);
        expr->children[i]->print_parenthesized();
        Sexpr* obj=expr->children[i]->children[0];
        float dist=atof(expr->children[i]->children[1]->leaf);
        float dir=atof(expr->children[i]->children[2]->leaf);
    
        socFlag flag;
        if (obj->children[0]->leaf[0]=='f')
        {
/* beware, if you want to include the cetner flag, you might have to choose the farther solution      
          if (0 && obj->children.size()==2 && obj->children[1]->leaf[0]=='c')
          {
              printf(" * center flag");
              flag.push_back(0); flag.push_back(0); flag.push_back(dist); flag.push_back(dir);
              flags.push_back(flag);
          }
*/
          if (obj->children.size()==4)
            if (strchr("lrtb",obj->children[1]->leaf[0])!=NULL)
            {
              float p=999,q=999,z=atof((char*)(obj->children[3]->leaf));
    	  if (obj->children[1]->leaf[0]=='l') p = -60;
    	  if (obj->children[1]->leaf[0]=='r') p = 60;
    	  if (obj->children[1]->leaf[0]=='t') q = 40;
    	  if (obj->children[1]->leaf[0]=='b') q = -40;
    	  if (obj->children[2]->leaf[0]=='l') p = -z;
    	  if (obj->children[2]->leaf[0]=='r') p = z;
    	  if (obj->children[2]->leaf[0]=='t') q = z;
    	  if (obj->children[2]->leaf[0]=='b') q = -z;
              printf(" * flag, coords: %0.1f %0.1f",p,q);
              flag.push_back(p); flag.push_back(q); flag.push_back(dist); flag.push_back(dir);
              flags.push_back(flag);
    	}
        }
        printf("\n"); 
      }
    
      if (flags.size()<2) return -1;
      // find two closest flags (because have least error)
      int a=0,b=0; // index of flag with min and max angles
      for (int i=0 ; i<flags.size() ; i++)
        if (flags[i][2]<flags[a][2]) a = i;
      if (a==0) b = 1;
      for (int i=0 ; i<flags.size() ; i++)
        if (i!=a && flags[i][2]<flags[b][2]) b = i;
      if (a==b) { printf("warning: can't triangulate position from one flag\n"); return -1; }
    
      // transform so a is at (0,0) and b is on X-axis
      // '1p' is single-prime, which means translated
      // '2p' is double-prime, which means rotated
      float xa=flags[a][0],ya=flags[a][1],xb=flags[b][0],yb=flags[b][1],ra=flags[a][2],rb=flags[b][2];
      float dab=pow(xa-xb,2)+pow(ya-yb,2); if (dab<0) return -1; dab = sqrt(dab);
      if (ra+rb<dab) { printf("warning: no intersection\n"); return -1; }
      if (ra>dab+rb || rb>dab+ra) { printf("warning: no intersection - one circle contains the other\n"); return -1; }
      float xb1p=xb-xa,yb1p=yb-ya; // one prime
      float theta = atan2(yb1p,xb1p); // ang from a to b; could check for vertical (xb1p=0), but shouldn't occur
      float xb2p=xb1p*cos(-theta)-yb1p*sin(-theta); // b2p should equal (dab,0)
      float yb2p=xb1p*sin(-theta)+yb1p*cos(-theta);
      float x02p=(pow(ra,2)-pow(rb,2)+pow(xb2p,2))/(2.*xb2p);
      float y02p=pow(ra,2)-pow(x02p,2); if (y02p<0) return -1; y02p = sqrt(y02p); // second solution will be (x02p,-y02p)
      float x0s1=xa+x02p*cos(theta)-y02p*sin(theta); // unrotate and untranslate to get first solution
      float y0s1=ya+x02p*sin(theta)+y02p*cos(theta);
      float x0s2=xa+x02p*cos(theta)+y02p*sin(theta); // unrotate and untranslate to get second solution
      float y0s2=ya+x02p*sin(theta)-y02p*cos(theta);
      float d1=(pow(x0s1,2)+pow(y0s1,2)),d2=(pow(x0s2,2)+pow(y0s2,2));
      float x0=x0s1,y0=y0s1;
      if (d2<d1) { x0=x0s2; y0=y0s2; } // choose whichever solution is closest to center of field
      printf("estimate of player location: x=%0.1f y=%0.1f\n",x0,y0);
      printf("based on flags at (%0.1f,%0.1f) and (%0.1f,%0.1f), dists=(%0.1f,%0.1f), dab=%0.1f, theta=%0.1f\n",xa,ya,xb,yb,ra,rb,dab,theta*180./PI);
      *x = x0; *y = y0;
    
      // compute the direction the player is facing, based on relative angle to closest object
      float actang=atan2(ya-y0,xa-x0)*180./PI,relang=flags[a][3];
      *dir = actang+relang; // object to left of center have negative angle
      //printf("heading: actang=%0.1f, relang=%0.1f\n",actang,relang);
      printf("heading: actang=%0.1f, relang=%0.1f\n",actang,relang);
    
      return 0;
    }

void Player::player_initialization()
  {
    X = Y = 999;
    // for run_triangular_path()
    targets[0]=0; targets[1]=20;
    targets[2]=20; targets[3]=-10;
    targets[4]=-20; targets[5]=-10;
    ntargets = 3; tid = 0;
    tX=targets[2*tid]; tY=targets[2*tid+1]; // set initial target

    stage = 1;
  }

//void Player::decide_what_to_do() { run_triangular_path(); }
void Player::decide_what_to_do() { kick_ball_to_goal(); }

// behaviour: run to center, find soccer ball, kick it toward RGoal
// stage 1: if not facing center, turn; else dash; if near center, stage->2
// stage 2: turn around and look for ball, run to it; if near ball, kick toward RGoal; repeat!

void Player::kick_ball_to_goal()
{
  char buf[8192];
  if (stage==1)
  {
    if (dist(X,Y,0,0)<1) { stage = 2; return; } // also, could goto stage 2 early if see ball
    float ang=atan2(0-Y,0-X)*180./PI;
    float delta_ang=angdiff(ang,Hdg);
    if (delta_ang>10) sprintf(buf,"(turn -5)");
    else if (delta_ang<-10) sprintf(buf,"(turn 5)");
    else sprintf(buf,"(dash 50)");
  }  
  else if (stage==2)
  {
    if (ballX==999) sprintf(buf,"(turn -5)");
    else
    {
      float RGoalX=60,RGoalY=0; // maybe it should be (55,0)
      float RGoalDir=atan2(RGoalY-Y,RGoalX-X)*180./PI;  
      float kickDir = angdiff(Hdg,RGoalDir); // relative to direction player is facing
      if (ballRelDist<0.7) sprintf(buf,"(kick 25 %f)",kickDir); 
      else
      {
        if (ballRelDir>10) sprintf(buf,"(turn 5)");
        else if (ballRelDir<-10) sprintf(buf,"(turn -5)");
        else sprintf(buf,"(dash 40)");
      }
    }
  }
  size_t len = std::strlen( buf );
  if (len>0)
  {
    printf("command: %s\n",buf);
    M_transport->write( buf, len + 1 );
    M_transport->flush();
  }
}

void Player::run_trianglar_path()
  {
    char buf[8192];
    buf[0] = 0;
    if (X!=999 && tX!=999)
    {
      if (dist(X,Y,tX,tY)<2.0) // reached target
      {
        tid = (tid+1)%ntargets;
        tX=targets[2*tid]; tY=targets[2*tid+1]; // set next target
      }
      else
      {
        float ang=atan2(tY-Y,tX-X)*180./PI;
        float delta_ang=angdiff(ang,Hdg);
        if (delta_ang>10) sprintf(buf,"(turn -5)");
        else if (delta_ang<-10) sprintf(buf,"(turn 5)");
        else sprintf(buf,"(dash 50)");
printf("target=(%0.1f,%0.1f), objang=%f selfdir=%f, delta=%f\n",tX,tY,ang,Hdg,delta_ang);
printf("command: %s\n",buf);
      }
    }
    size_t len = std::strlen( buf );
    if (len>0)
    {
      M_transport->write( buf, len + 1 );
      M_transport->flush();
    }
  }

