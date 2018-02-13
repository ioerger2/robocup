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

#define CLIENT_HPP

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

    Client( const std::string & server,const int port );
    virtual ~Client();
    void run();
    int open();
    bool bind();
    void close();
    int setCompression( int level );
    void processMsg( const char * msg, const size_t len );
    virtual void parseMsg( const char * msg, const size_t len );
    virtual void player_initialization(); // TRI
    virtual void decide_what_to_do(); // TRI
    void messageLoop();
};
