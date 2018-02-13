// -*-c++-*-

/***************************************************************************
                          client.cpp  -  A basic client that connects to
                          the server
                             -------------------
    begin                : 27-DEC-2001
    copyright            : (C) 2001 by The RoboCup Soccer Server
                           Maintenance Group.
    email                : sserver-admin@lists.sourceforge.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU LGPL as published by the Free Software  *
 *   Foundation; either version 3 of the License, or (at your option) any  *
 *   later version.                                                        *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "compress.h"

#include <rcssbase/net/socketstreambuf.hpp>
#include <rcssbase/net/udpsocket.hpp>
#include <rcssbase/gzip/gzstream.hpp>

#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif
#include <iostream>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef __CYGWIN__
// cygwin is not win32
#elif defined(_WIN32) || defined(__WIN32__) || defined (WIN32)
#  define RCSS_WIN
#  include <winsock2.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h> // select()
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h> // select()
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> // select()
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h> // select()
#endif

//------------------------------------------------------------------------------

#include <math.h>
#include "Sexpr.hpp"

#define PI 3.1415927

float abs(float X) { return (X>=0) ? X : -X; }

float dist(float X1,float Y1,float X2,float Y2) { return sqrt(pow(X1-X2,2)+pow(Y1-Y2,2)); }

//------------------------------------------------------------------------------

class Client {
//private:
public:
    rcss::net::Addr M_dest;
    rcss::net::UDPSocket M_socket;
    rcss::net::SocketStreamBuf * M_socket_buf;
    rcss::gz::gzstreambuf * M_gz_buf;
    std::ostream * M_transport;
    int M_comp_level;
    bool M_clean_cycle;

#ifdef HAVE_LIBZ
    Decompressor M_decomp;
#endif

    Client();
    Client( const Client & );
    Client & operator=( const Client & );

    Client( const std::string & server,
            const int port )
        : M_dest( port ),
          M_socket(),
          M_socket_buf( NULL ),
          M_gz_buf( NULL ),
          M_transport( NULL ),
          M_comp_level( -1 ),
          M_clean_cycle( true )
      {
          M_dest.setHost( server );
          open();
          bind();

          M_socket_buf->setEndPoint( M_dest );
      }

    virtual
    ~Client()
      {
          close();
      }

    void run()
      {
          messageLoop();
      }

//private:

    int open()
      {
          if ( M_socket.open() )
          {
              if ( M_socket.setNonBlocking() < 0 )
              {
                  std::cerr << __FILE__ << ": " << __LINE__
                      << ": Error setting socket non-blocking: "
                            << strerror( errno ) << std::endl;
                  M_socket.close();
                  return -1;
              }
          }
          else
          {
              std::cerr << __FILE__ << ": " << __LINE__
                        << ": Error opening socket: "
                        << strerror( errno ) << std::endl;
              M_socket.close();
              return -1;
          }

          M_socket_buf = new rcss::net::SocketStreamBuf( M_socket );
          M_transport = new std::ostream( M_socket_buf );
          return 0;
      }

    bool bind()
      {
          if ( ! M_socket.bind( rcss::net::Addr() ) )
          {
              std::cerr << __FILE__ << ": " << __LINE__
                        << ": Error connecting socket" << std::endl;
              M_socket.close();
              return false;
          }
          return true;
      }

    void close()
      {
          M_socket.close();

          if ( M_transport )
          {
              delete M_transport;
              M_transport = NULL;
          }

          if ( M_gz_buf )
          {
              delete M_gz_buf;
              M_gz_buf = NULL;
          }

          if ( M_socket_buf )
          {
              delete M_socket_buf;
              M_socket_buf = NULL;
          }
      }

    int setCompression( int level )
      {
#ifdef HAVE_LIBZ
          if ( level >= 0 )
          {
              if ( ! M_gz_buf )
              {
                  M_gz_buf = new rcss::gz::gzstreambuf( *M_socket_buf );
              }
              M_gz_buf->setLevel( level );
              M_transport->rdbuf( M_gz_buf );
          }
          else
          {
              M_transport->rdbuf( M_socket_buf );
          }
          return M_comp_level = level;
#endif
          return M_comp_level = -1;
      }

    void processMsg( const char * msg,
                     const size_t len )
      {
#ifdef HAVE_LIBZ
          if ( M_comp_level >= 0 )
          {
              M_decomp.decompress( msg, len, Z_SYNC_FLUSH );
              char * out;
              int size;
              M_decomp.getOutput( out, size );
              if ( size > 0 )
              {
                  parseMsg( out, size );
              }
          }
          else
#endif
          {
              parseMsg( msg, len );
          }
      }

    virtual void parseMsg( const char * msg,
                   const size_t len )
      {
          if ( ! std::strncmp( msg, "(ok compression", 15 ) )
          {
              int level;
              if ( std::sscanf( msg, " ( ok compression %d )", &level ) == 1 )
              {
                  setCompression( level );
              }
          }
          else if ( ! std::strncmp( msg, "(sense_body", 11 )
                    || ! std::strncmp( msg, "(see_global", 11 )
                    || ! std::strncmp( msg, "(init", 5 ) )
          {
              M_clean_cycle = true;
          }

          //std::cout << std::string( msg, len - 1 ) << std::endl;
          std::cout << msg << std::endl;
      }

    int infer_player_loc(Sexpr* expr,float* x,float* y,float* dir) // add verbose flag?
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

    // TRI
    virtual void player_initialization() {}
    virtual void decide_what_to_do() {}

    void messageLoop()
      {
          fd_set read_fds;
          fd_set read_fds_back;
          char buf[8192];
          std::memset( &buf, 0, sizeof( char ) * 8192 );

          int in = fileno( stdin );
          FD_ZERO( &read_fds );
          FD_SET( in, &read_fds );
          FD_SET( M_socket.getFD(), &read_fds );
          read_fds_back = read_fds;

#ifdef RCSS_WIN
          int max_fd = 0;
#else
          int max_fd = M_socket.getFD() + 1;
#endif

          int cnt=0;
          sprintf(buf,"(init Team1 (version 9))");
          M_transport->write( buf, std::strlen( buf ) + 1 );
          M_transport->flush();

          sprintf(buf,"(move 0 0)");
          M_transport->write( buf, std::strlen( buf ) + 1 );
          M_transport->flush();

          player_initialization(); // TRI

          while ( 1 )
          {

              read_fds = read_fds_back;
              int ret = ::select( max_fd, &read_fds, NULL, NULL, NULL );
              if ( ret < 0 )
              {
                  perror( "Error selecting input" );
                  break;
              }
              else if ( ret != 0 )
              {
  		  decide_what_to_do(); // TRI
		  
                  // read from stdin
                  if ( FD_ISSET( in, &read_fds ) )
                  {
                      if ( std::fgets( buf, sizeof( buf ), stdin ) != NULL )
                      {
                          size_t len = std::strlen( buf );
                          if ( buf[len-1] == '\n' )
                          {
                              buf[len-1] = '\0';
                              --len;
                          }

                          M_transport->write( buf, len + 1 );
                          M_transport->flush();
                          if ( ! M_transport->good() )
                          {
                              if ( errno != ECONNREFUSED )
                              {
                                  std::cerr << __FILE__ << ": " << __LINE__
                                            << ": Error sending to socket: "
                                            << strerror( errno ) << std::endl
                                            << "msg = [" << buf << "]\n";
                              }
                              M_socket.close();
                          }
                          std::cout << buf << std::endl;
                      }
                  }

                  // read from socket
                  if ( FD_ISSET( M_socket.getFD(), &read_fds ) )
                  {
                      rcss::net::Addr from;
                      int len = M_socket.recv( buf, sizeof( buf ) - 1, from );
                      if ( len == -1 && errno != EWOULDBLOCK )
                      {
                          if ( errno != ECONNREFUSED )
                          {
                              std::cerr << __FILE__ << ": " << __LINE__
                                        << ": Error receiving from socket: "
                                        << strerror( errno ) << std::endl;
                          }
                          M_socket.close();
                      }
                      else if ( len > 0 )
                      {
                          M_dest.setPort( from.getPort() );
                          M_socket_buf->setEndPoint( M_dest );
                          processMsg( buf, len );
                      }
                  }
              }
          }
      }
};

//----------------------------------------------------------------------------

class PlayerRunsPath : public Client
{
public:
  float X,Y,Dir;
  float tX,tY; // target
  float targets[10];
  int ntargets,tid;

  PlayerRunsPath( const std::string & server, const int port ) : Client(server,port) {}

  
  void parseMsg( const char * msg, const size_t len )
  {
    Client::parseMsg(msg,len); // let parent class do its work

    Sexpr* expr=new Sexpr(tokenize((char*)msg),0);
    float x,y,dir;
    int stat=infer_player_loc(expr,&x,&y,&dir);

    if (stat==0) 
    {
      printf("inferred player location: (%0.1f,%0.1f), dir=%0.1f\n",x,y,dir);
      X = x; Y = y; Dir = dir; // update state variables
    }

   //delete expr;
  }

  void player_initialization()
  {
    X = Y = 999;
    targets[0]=0; targets[1]=20;
    targets[2]=20; targets[3]=-10;
    targets[4]=-20; targets[5]=-10;
    ntargets = 3; tid = 0;
    tX=targets[2*tid]; tY=targets[2*tid+1]; // set initial target
  }

  // fills in vals for x,y,dir; returns -1 if fails for any reason
  
  void decide_what_to_do() // TRI
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
        //if (abs(ang-Dir)>10) sprintf(buf,"(turn %d)",(int)(ang-Dir));
        float delta_ang=ang-Dir;
        if (delta_ang>=180) delta_ang -= 360;
        if (delta_ang<=-180) delta_ang += 360;
        if (delta_ang>10) sprintf(buf,"(turn -5)");
        else if (delta_ang<-10) sprintf(buf,"(turn 5)");
        else sprintf(buf,"(dash 50)");
printf("target=(%0.1f,%0.1f), objang=%f selfdir=%f, delta=%f\n",tX,tY,ang,Dir,delta_ang);
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
};

//----------------------------------------------------------------------------


namespace {
Client * client = static_cast< Client * >( 0 );

void
sig_exit_handle( int )
{
    std::cerr << "\nKilled. Exiting..." << std::endl;
    if ( client )
    {
        delete client;
        client =  static_cast< Client * >( 0 );
    }
    std::exit( EXIT_FAILURE );
}
}


int
main ( int argc, char **argv )
{
    if ( std::signal( SIGINT, &sig_exit_handle) == SIG_ERR
         || std::signal( SIGTERM, &sig_exit_handle ) == SIG_ERR
         || std::signal( SIGHUP, &sig_exit_handle ) == SIG_ERR )
    {
        std::cerr << __FILE__ << ": " << __LINE__
                  << ": could not set signal handler: "
                  << std::strerror( errno ) << std::endl;
        std::exit( EXIT_FAILURE );
    }

    std::cerr << "Hit Ctrl-C to exit." << std::endl;

    std::string server = "localhost";
    int port = 6000;

    for ( int i = 0; i < argc; ++i )
    {
        if ( std::strcmp( argv[ i ], "-server" ) == 0 )
        {
            if ( i + 1 < argc )
            {
                server = argv[ i + 1 ];
                ++i;
            }
        }
        else if ( std::strcmp( argv[ i ], "-port" ) == 0 )
        {
            if ( i + 1 < argc )
            {
                port = std::atoi( argv[ i + 1 ] );
                ++i;
            }
        }
    }

    //client = new Client( server, port );
    client = new PlayerRunsPath( server, port );
    client->run();

    return EXIT_SUCCESS;
}
