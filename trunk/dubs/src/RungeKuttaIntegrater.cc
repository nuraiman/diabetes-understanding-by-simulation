#include "DubsConfig.hh"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "ArtificialPancrease.hh"
#include "RungeKuttaIntegrater.hh"

using namespace std;
using namespace boost;
using namespace boost::posix_time;

/******************************************************************************/
/* rungeKutta4()
 * 
 * jjhaag AT dreamincode.net
 * 14 November 2007
 * 
 * 
 * 4-step (fourth order) Runge-Kutta numerical integrator for ordinary differential equations.
 * 
 * A common and powerful numerical integrator.  No adaptive step size; such 
 * functionality can be controlled using an external function.
 * 
 * Arguments:
 * 		x	-	the initial x-value
 * 		yi	-	vector of the initial y-values
 * 		dx	-	the step size for the integration
 * 		derivatives		-	a pointer to a function that returns the derivative 
 * 				of a function at a point (x,y), where y is an STL vector of 
 * 				elements.  Returns a vector of the same size as y.
 * 
 * If you're looking for details on the Runge-Kutta method(s), do a google 
 * search - that would take up pages here, and that's probably a little more 
 * than belongs in the header comments.
 * 
 * Dependencies: requires <vector> (plus whatever is required by the derivatives
 * function and the rest of your program).
 * 
 * 
 * 
 * Example usage - numerical integration dy/dt = cos(x) :
 * #include <vector>
 * #include <cmath>
 * std::vector<double> dydtcos(double x, vector<double> y) {
 *     std::vector<double> dydt(y)
 *     dydt.at(i)=cos(x);
 *     return dydt;
 * }
 * --------------
 * vector<double> yi(1,0.0);
 * double x=0.0, dx=0.10;
 * 
 * while (x<10.0) {
 *     yi=rungeKutta4(yi,x,dx,(*dydtcos));
 *     x+=dx;
 * }
 * 
 */






std::vector<double> rungeKutta4( double xi, std::vector<double> yi,
		                             double dx, RK_DerivFuntion derivatives ) 
{	
	//	total number of elements in the vector
	const size_t n = yi.size();
	
	//	first step
	std::vector<double> k1;
	k1=derivatives(xi, yi);
	for (size_t i=0; i<n; ++i) {
		k1.at(i)*=dx;
	}
	
	//	second step
	std::vector<double> k2(yi);
	for (size_t i=0; i<n; ++i) {
		k2.at(i)+=k1.at(i)/2.0;
	}
	k2=derivatives(xi+dx/2.0,k2);
	for (size_t i=0; i<n; ++i) {
		k2.at(i)*=dx;
	}
	
	//	third step
	std::vector<double> k3(yi);
	for (size_t i=0; i<n; ++i) {
		k3.at(i)+=k2.at(i)/2.0;
	}
	k3=derivatives(xi+dx/2.0,k3);
	for (size_t i=0; i<n; ++i) {
		k3.at(i)*=dx;
	}
	
	
	//	fourth step
	std::vector<double> k4(yi);
	for (size_t i=0; i<n; ++i) {
		k4.at(i)+=k3.at(i);
	}
	k4=derivatives(xi+dx,k4);
	for (size_t i=0; i<n; ++i) {
		k4.at(i)*=dx;
	}
	
	
	//	sum the weighted steps into yf and return the final y values
	std::vector<double> yf(yi);
	for (size_t i=0; i<n; ++i) {
		yf.at(i)+=(k1.at(i)/6.0)+(k2.at(i)/3.0)+(k3.at(i)/3.0)+(k4.at(i)/6.0);
	}
	
	return yf;
}



std::vector<double> rungeKutta4( const boost::posix_time::ptime &time,
                                 const std::vector<double> &yi,
                                 const boost::posix_time::time_duration &dt,
                                 RK_PTimeDFunc derivatives )
{  
	//	total number of elements in the vector
	const size_t n = yi.size();
	const double dx = toNMinutes(dt);
  
	//	first step
	std::vector<double> k1;
	k1=derivatives(time, yi);
	for (size_t i=0; i<n; ++i) {
		k1.at(i)*=dx;
	}
	
	//	second step
	std::vector<double> k2(yi);
	for (size_t i=0; i<n; ++i) {
		k2.at(i)+=k1.at(i)/2.0;
	}
	k2=derivatives(time + (dt/2.0),k2);
	for (size_t i=0; i<n; ++i) {
		k2.at(i)*=dx;
	}
	
	//	third step
	std::vector<double> k3(yi);
	for (size_t i=0; i<n; ++i) {
		k3.at(i)+=k2.at(i)/2.0;
	}
	k3=derivatives(time+(dt/2.0),k3);
	for (size_t i=0; i<n; ++i) {
		k3.at(i)*=dx;
	}
	
	
	//	fourth step
	std::vector<double> k4(yi);
	for (size_t i=0; i<n; ++i) {
		k4.at(i)+=k3.at(i);
	}
	k4=derivatives(time+dt,k4);
	for (size_t i=0; i<n; ++i) {
		k4.at(i)*=dx;
	}
	
	
	//	sum the weighted steps into yf and return the final y values
	std::vector<double> yf(yi);
	for (size_t i=0; i<n; ++i) {
		yf.at(i)+=(k1.at(i)/6.0)+(k2.at(i)/3.0)+(k3.at(i)/3.0)+(k4.at(i)/6.0);
	}
	
	return yf;
}






//To allow integrating single equations ezier, here is a wrapper for the above
double rungeKutta4( double x, double y, double dx, ForcingFunction derivativeFunction )
{
  std::vector<double> y1(1, y);
  RK_DerivFuntion func = boost::bind( SingleVariableWrapperFunction, _1, _2, derivativeFunction );
    
  y1 = rungeKutta4( x, y1, dx, func );
  
  return y1[0];
}//rungeKutta4


double rungeKutta4( const boost::posix_time::ptime &time, 
                    double y, const boost::posix_time::time_duration &dt, 
                    PTimeForcingFunction derivativeFunction )
{
  std::vector<double> y1(1, y);
  RK_PTimeDFunc func = boost::bind( PTimeSingleVariableWrapperFunction, _1, _2, derivativeFunction );
 
  y1 = rungeKutta4( time, y1, dt, func );
  return y1[0];
}//rungeKutta4

//A wrapper used by the above
std::vector<double> SingleVariableWrapperFunction( double x, std::vector<double> y, ForcingFunction func )
{
  y[0] = func(x); //keep from getting compiler warnings
  return y;
}//

std::vector<double> 
PTimeSingleVariableWrapperFunction( const ptime &time, std::vector<double> y, 
                                   PTimeForcingFunction func )
{
  y[0] = func(time); //keep from getting compiler warnings
  return y;
}//


double integrateRungeKutta4( ForcingFunction func, 
                             double t0, double t2, double timeStep )
{
  double f = 0.0;
  
  for( double t = t0; t<=t2; t += timeStep )
    f = rungeKutta4( t, f, timeStep, func );
  
  return f;
}//integrateRungeKutta4



//Integrate in the range t0 to t1 in 
int integrateRungeKutta4( double t0, double t1, double dt, 
                          std::vector<double> y0,  //starting values
                          RK_DerivFuntion derivatives,
                          std::vector<ConsentrationGraph> &answers
                                                     )
{
  assert( y0.size() == answers.size() );
  
  
  for( double t = t0; t<=t1; t += dt )
  {
    y0 = rungeKutta4( t, y0, dt, derivatives );
    
    for( size_t i = 0; i < y0.size(); ++i )
    {
      PosixTime time = answers[i].getT0() + toTimeDuration(t);
      answers[i].insert( time, y0[i] );
    }
  }//for( loop over integration time )
  
  
  return 1;
}//integrateRungeKutta4



