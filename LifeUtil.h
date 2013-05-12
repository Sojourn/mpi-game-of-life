#ifndef LIFEUTIL_H
#define LIFEUTIL_H
/*
 *       File:           LifeUtil.h
 *       Description:    The header file to describe all of the utility functions
 *       Authors:        Scott Connell && Joel Rausch
 *       Date Created:   April 23, 2013 at 13:12
 *
 *       This file written for Programming Assignment 3 for CprE 426.
 *       Iowa State University
 *
 */
#include <string>
#include <vector>
#include <istream>
#include <ostream>
#include <stdint.h>
#include "Array2D.h"

// Alias a 2-d array of booleans used to represent a game of life board.
typedef Array2D<bool> LifeBoard;

// Header information from a life file.
struct LifeHeader_t
{
	uint32_t width;
	uint32_t height;
	uint32_t generations;
};

// A rectangular region.
struct Region_t
{
	size_t x_start; // Top-left x coordinate
	size_t y_start; // Top-left y coordinate
	size_t width;
	size_t height;
};

// Read a game of life file from an input stream. Returns true on success.
bool readFile(std::istream &in, LifeBoard &board, LifeHeader_t &header);

// Write a game of life file to an output sream. Returns true on success.
bool writeFile(std::ostream &out, const LifeBoard &board, const LifeHeader_t &header);

// Advance a region of the board one generation.
void step_region(
	const Region_t &region,
	const LifeBoard &src_generation,
	LifeBoard &dst_generation);

// Calculate an optimal processor configuration for the game of life simulation.
std::pair<size_t, size_t> calculate_topology(
	int32_t comm_size,
	const std::pair<size_t, size_t>& board_size);

#ifdef DEBUG

// Write the contents of a life board to stdout.
void dumpBoard(const LifeBoard &board);
#endif

#endif // LIFEUTIL_H
