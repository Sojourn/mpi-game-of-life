/*
 *       File:           Serial.cpp
 *       Description:    The main file to run Conway's Game of Life in serial.
 *       Authors:        Scott Connell && Joel Rausch
 *       Date Created:   May 6, 2013 at 01:08
 *
 *       This file written for Programming Assignment 3 for CprE 426.
 *       Iowa State University
 *
 */
#include <fstream>
#include "LifeUtil.h"

int main(int argc, char **argv)
{
	LifeHeader_t header;
	LifeBoard board[2];
	bool index = false;

	// Check arguments
	if(argc < 3)
		return -1;

	// Read input
	std::ifstream in(argv[1]);
	if(!readFile(in, board[index], header))
		return -1;
	in.close();

	// Iterate through the generations
	Region_t region = {0, 0, board[index].width(), board[index].height()};
	board[!index].resize(board[index].width(), board[index].height());
	for(size_t i = 0; i < header.generations; i++)
	{
		step_region(region, board[index], board[!index]);
		index = !index;
	} 

	// Write output
	std::ofstream out(argv[2]);
	if(!writeFile(out, board[index], header))
		return -1;
	out.close();

	return 0;
}
