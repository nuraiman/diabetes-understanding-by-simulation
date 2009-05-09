//adapted from http://www.dreamincode.net/code/snippet1441.htm

#include <vector>
#include "boost/function.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#include "ConsentrationGraph.hh"
#include "ArtificialPancrease.hh" //contains useful typedefs and constants


//A note for the future: Try implementing a 'symplectic integrator' to
//  conserve quantities in the long run

/* 4-step (fourth order) Runge-Kutta numerical integrator for ordinary differential equations.
 *
 * Arguments:
 * 		x	-	the initial x-value
 * 		yi	-	vector of the initial y-values
 * 		dx	-	the step size for the integration
 * 		derivatives		-	a pointer to a function that returns the derivative 
 * 				of a function at a point (x,y), where y is an STL vector of 
 * 				elements.  Returns a vector of the same size as y.
*/

std::vector<double> rungeKutta4(
		                             double x,
		                             std::vector<double> yi,
		                             double dx,
                                 RK_DerivFuntion derivatives
                               ) ;


//Integrate in the range t0 to t1 in 
int integrateRungeKutta4( double t0, double t1, double dt, 
                                          std::vector<double> y0,  //starting values
                                          RK_DerivFuntion derivatives,
                                          std::vector<ConsentrationGraph> &answers
                                         );

//To allow integrating single differential equations ezier, here is a wrapper for the above
double rungeKutta4( double x, double y, double dx, ForcingFunction derivativeFunction );

//A wrapper used by the above
std::vector<double> SingleVariableWrapperFunction( double x, std::vector<double> y, ForcingFunction func );

double integrateRungeKutta4( ForcingFunction func, double t0, double t2, double timeStep = 0.01 );
