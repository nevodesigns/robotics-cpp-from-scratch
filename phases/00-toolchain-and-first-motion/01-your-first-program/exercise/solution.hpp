// Your first function.
//
// Fill in the body of battery_percent so the tests pass. Everything you need is
// described in docs/en.md, in the section called Build It.
//
// Run:  rcpp verify 00-01

#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// Converts a battery reading in millivolts into a percentage from 0 to 100.
//
//   3000 millivolts or less  ->    0
//   4200 millivolts or more  ->  100
//   3600 millivolts          ->   50
//
// Calculates battery percentage from a millivolt reading.
inline int battery_percent(int millivolts) {
if(millivolts <= 3000){
  return 0;
}else if (millivolts >=4200) {
return 100;
} else{
  return ((millivolts-3000)*100)/1200;
}
}

#endif  // LESSON_SOLUTION_HPP
