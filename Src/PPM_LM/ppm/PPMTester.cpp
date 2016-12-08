
// Simple command line example program that uses Dasher's PPM language model.
//

#include "ProbModelPPM.h"
#include <stdio.h>

int main(int argc, char* argv[])
{
    ProbModelPPM ppm;

    ppm.Init();
    ppm.OutputInfoEveryStep("hello my name is yaming");

    return 0;
}