https://sites.google.com/site/willjohnsonsstuff/diabetes-understanding-by-simulation/dubs_display.png?width=400px
# overview #
The development and availability of Continuous Glucose Monitoring Systems (CGMS) has substantially improved both the quality of life, and blood-glucose control for many Type 1 Diabetics.
The primary goal of this project is to take this understanding a step further.
By using CGMS data, along with other sources of information, carbohydrates consumed, insulin taken, physical activity, etc.,  my hope is to produces a mathematical model of the individual diabetic.  This model may then be used in conjunction with a data driven algorithm, to predict future blood-glucose concentrations.  In turn this prediction may be used to suggest insulin or carbohydrate dosing, or issue early low blood-sugar warnings.

Some key features I'm working on include:
  * A probabilistic interpretation of model predictions.
    * e.g. for a given confidence level, the model will describe a range of potential outcomes.
  * Be able to pick up on non-trivial trends
    * A mathematical model can't hope to describe such a complex system such as a person with type 1 diabetes.  This project will attempt to account for this through a series of statistical and heuristic methods.
    * Clearly the system won't have access to to all bio-information of the person, therefore the system should be able to take a 'best guess' as the form of driving force behind a given trend or event.
  * "real time" analysis of CGMS data

## Project Status ##
Since this is primarily a personal research project, and in it's infancy, many of the features I am working on have not been hooked up to the GUI interface, however what is available under the gui-driven web interface includes:
  * Read in CGMS/insulin/meal information from Minimed, Dexcom, and Navigator system exports
  * Find parameters for the set of non-linear differential equations currently used
  * Make predictions of blood glucose concentrations up to an hour ahead of present time
  * User control of parameter optimization options
  * Saving programs state to disk

Implemented features not available via gui interface include:
  * Filtering of CGMS data via, Fast Fourier processing, 4th order Butterworth filtering, b-spline smoothing, Savitzy-Golay filtering.
  * A host of features for researching effects, and quick protyping of new ideas

Feature under construction
  * Adding more interfaces to gui
  * Take into account uncertainties of parameters and events
  * Fit for an event that was not recorded (e.g. ate a meal and forgot to take insulin)
  * Recommended correction doses of insulin/carbohydrates


I am hoping to soon have this project in a state presentable to others; meaning able to make predictions for  given confidence levels, suggest corrections, able to simulate hypothetical situations, and more.  In the mean time, you are welcome to contact me if you would like to become involved, or would like future updates, my name is Will Johnson, and my email is banjohik@gmail.com.
I use this project on a continual basis to track my carbohydrates, insulin, exercise, blood-glucose, and various other factors that effect my blood-glucose, via the web-interface from my Android smart phone.  I then use this data to train the model (using a genetic algorithm) to learn my response to the various inputs, so I can then more-improve the model.

You are welcome do download the code, compile and run it either locally on your computer, or through your own webserver; you can also email me and ask for an account on my webserver (with very limited bandwidth).


### A Note about the Code ###
This project is implemented in c++, using using the [Boost](http://www.boost.org/), [ROOT](http://root.cern.ch), [GSL](http://www.gnu.org/software/gsl/) (v 1.12), and [Witty](http://www.webtoolkit.eu/wt) libraries, and is known to compile under **Nix operating systems using either clang or gcc.**


### Various Screen Shots ###
![https://sites.google.com/site/willjohnsonsstuff/diabetes-understanding-by-simulation/dubs_display.png](https://sites.google.com/site/willjohnsonsstuff/diabetes-understanding-by-simulation/dubs_display.png)

![https://sites.google.com/site/willjohnsonsstuff/diabetes-understanding-by-simulation/dubs_error_grid.png](https://sites.google.com/site/willjohnsonsstuff/diabetes-understanding-by-simulation/dubs_error_grid.png)

![https://sites.google.com/site/willjohnsonsstuff/diabetes-understanding-by-simulation/dubs_chi2.png](https://sites.google.com/site/willjohnsonsstuff/diabetes-understanding-by-simulation/dubs_chi2.png)

![https://sites.google.com/site/willjohnsonsstuff/diabetes-understanding-by-simulation/dubs_response_fcn.png](https://sites.google.com/site/willjohnsonsstuff/diabetes-understanding-by-simulation/dubs_response_fcn.png)