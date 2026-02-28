#ifndef ERRORLOGER_HPP
#define ERRORLOGER_HPP

#include <iostream>
#include <string>
#include <SDL3/SDL.h>

void logSDLError(std::ostream& os, const std::string& msg);

#endif