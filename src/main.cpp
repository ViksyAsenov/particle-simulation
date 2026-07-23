#include <config.h>
#include "application.h"

int main()
{
  Application app;

  if (!app.init())
  {
    std::cerr << "Application failed to initialize." << std::endl;

    return -1;
  }

  app.run();

  return 0;
}