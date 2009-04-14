#ifndef _SERVER_DEBUG_HPP_
#define _SERVER_DEBUG_HPP_

#ifdef SRV_DEBUG
#define DBG( x ) std::cout << x << std::endl
#else
#define DBG( x )
#endif

#ifdef SRV_TRACE
#define Dout( x, y ) std::cout << x << ": " << y << '(' << __FILE__ << ':' << __LINE__ << ')' << std::endl;
#else
#define Dout( x, y )
#endif

#endif
