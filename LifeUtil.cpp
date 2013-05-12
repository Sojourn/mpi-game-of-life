/*
 *       File:           LifeUtil.cpp
 *       Description:    The implementation of the utility functions
 *       Authors:        Scott Connell && Joel Rausch
 *       Date Created:   April 23, 2013 at 13:16
 *
 *       This file written for Programming Assignment 3 for CprE 426.
 *       Iowa State University
 *
 */
#include "LifeUtil.h"
#include <iostream>
#include <fstream>
#include <sstream>

bool readFile(std::istream &in, LifeBoard &board, LifeHeader_t &header)
{
	if(!in.good()) return false;
	in >> header.height;
	
	if(!in.good()) return false;
	in >> header.width;
	
	if(!in.good()) return false;
	in >> header.generations;

	board.resize(header.width, header.height);
	for(size_t y = 0; y < board.height(); y++)
	{
		for(size_t x = 0; x < board.width(); x++)
		{
			if(!in.good()) return false;
			in >> board[y][x];
		}
	}

	return true;	
}

bool writeFile(std::ostream &out, const LifeBoard &board, const LifeHeader_t &header)
{
	for(size_t y = 0; y < board.height(); y++)
	{
		for(size_t x = 0; x < board.width(); x++)
		{
			if(!out.good()) return false;
			out << board[y][x];

			if((x + 1) == board.width())
			{
				out << std::endl;
			}
			else
			{
				out << " ";
			}
		}
	}

	return true;
}

#ifdef DEBUG
#include <mpi.h>

void dumpBoard(const LifeBoard &board)
{
	int32_t rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	std::stringstream ss;

	ss << "---------------------------" << std::endl;
	ss << rank << std::endl;

	for(size_t y = 0; y < board.height(); y++)
	{
		for(size_t x = 0; x < board.width(); x++)
		{
			ss << board[y][x];
			if(x + 1 == board.width())
				ss << std::endl;
			else
				ss << " ";		
		}
	}
	ss << "---------------------------" << std::endl;
	std::cout << ss.str().c_str();
}
#endif

inline bool is_alive(uint32_t x, uint32_t y, const LifeBoard &generation)
{
	if(x >= generation.width()) return false;
	if(y >= generation.height()) return false;
	return generation.at(x, y);
}

void step_region(
	const Region_t &region,
	const LifeBoard &src_generation,
	LifeBoard &dst_generation)
{
	for(size_t y = region.y_start; y < (region.y_start + region.height); y++)
	{
		for(size_t x = region.x_start; x < (region.x_start + region.width); x++)
		{
			uint32_t alive_sum = 0;
			alive_sum += is_alive(x    , y - 1, src_generation); // N
			alive_sum += is_alive(x + 1, y - 1, src_generation); // NE		
			alive_sum += is_alive(x + 1, y    , src_generation); // E		
			alive_sum += is_alive(x + 1, y + 1, src_generation); // SE		
			alive_sum += is_alive(x    , y + 1, src_generation); // S		
			alive_sum += is_alive(x - 1, y + 1, src_generation); // SW		
			alive_sum += is_alive(x - 1, y    , src_generation); // W
			alive_sum += is_alive(x - 1, y - 1, src_generation); // NW
			
			if(src_generation[y][x])
			{
				// Overcrowded death
				if(alive_sum >= 4) dst_generation[y][x] = false;
				
				// Loneliness death	
				else if(alive_sum <= 1) dst_generation[y][x] = false;
			
				// Still alive	
				else dst_generation[y][x] = true;
			}
			else
			{
				// Born
				if(alive_sum == 3) dst_generation[y][x] = true;
				
				// Still dead	
				else dst_generation[y][x] = false;
			}
		}
	}

}

std::pair<size_t, size_t> calculate_topology(
	int32_t comm_size,
	const std::pair<size_t, size_t>& board_size)
{
	size_t x;
	size_t y;
	size_t min_dist;

	if(comm_size == 0)
	{
		x = 0;
		y = 0;
	}
	else if(comm_size == 1)
	{
		x = 1;
		y = 1;
	}
	else
	{
		min_dist = comm_size;
		for(size_t i = 1; i < (comm_size / 2) + 1; i++)
		{
			size_t t = comm_size / i;
			if((t * i) != comm_size)
			continue;

			size_t dist = abs(t - i);
			if(dist < min_dist)
			{
				min_dist = dist;
				x = t;
				y = i;
			}
		}
	}

	if(board_size.first >= board_size.second)
	{
		return std::make_pair(std::max(x, y), std::min(x, y));
	}
	else
	{
		return std::make_pair(std::min(x, y), std::max(x, y));
	}
}
