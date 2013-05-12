/*
 *       File:           Main.cpp
 *       Description:    The main file to run Conway's Game of Life in parallel.
 *       Authors:        Scott Connell && Joel Rausch
 *       Date Created:   April 23, 2013 at 14:01
 *
 *       This file written for Programming Assignment 3 for CprE 426.
 *       Iowa State University
 *
 */
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <mpi.h>

#include "AsyncIO.h"
#include "Array2D.h"
#include "LifeUtil.h"

// Returns the coordinates of a processor given its index and the grid topology.
std::pair<size_t, size_t> map_processor(size_t index, const std::pair<size_t, size_t> &topology);

// Return the width of a processor's local board.
size_t subgrid_width(size_t index, size_t columns, size_t board_width);

// Return the height of a processor's local board.
size_t subgrid_height(size_t index, size_t rows, size_t columns, size_t board_height);

// Read the board and pass out the local segment. Returns true on success.
bool scatter_board(std::istream &in, LifeBoard &local_board, LifeHeader_t &header);

// Gather each processor's local segment and write it to the output stream. Returns true on success.
bool gather_board(std::ostream &out, const LifeBoard &local_board, const LifeHeader_t &header);

// Move a local board into a send buffer (removing margin and adding padding).
void pad_buffer(const LifeBoard &board, bool *buffer);

// Move the contents of a receive buffer into a board.
void unpad_buffer(LifeBoard &board, const bool *buffer);

// Calculate how the processor's local board maps onto the global board.
std::pair<size_t, size_t> calculate_offsets(
	const std::pair<size_t, size_t> &loc,
	const std::pair<size_t, size_t> &topology,
	const std::pair<size_t, size_t> &board_size);

// Return status enumeration.
enum Status_t
{
	STATUS_SUCCESS     =  0,
	STATUS_BAD_PARAMS  = (1 << 0),
	STATUS_READ_ERROR  = (1 << 1),
	STATUS_WRITE_ERROR = (1 << 2)
};

int main(int argc, char **argv)
{
	LifeBoard board;
	LifeBoard result_board;
	LifeHeader_t header;
	int32_t size;
	int32_t rank;

	MPI_Init(&argc, &argv);
	MPI_Comm_size( MPI_COMM_WORLD, &size );
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );

	if(argc < 3)
		MPI_Abort(MPI_COMM_WORLD, STATUS_BAD_PARAMS);

	// Initialize the local board segment
	std::ifstream in(argv[1]);
	if(scatter_board(in, board, header))
	{
		result_board.resize(board.width(), board.height());
	}
	else
	{
		MPI_Abort(MPI_COMM_WORLD, STATUS_READ_ERROR);
	}
	in.close();

	{
		Region_t center = {2, 2, board.width() - 4, board.height() - 4};
		Region_t north = {1, 1, board.width() - 2, 1};
		Region_t south = {1, board.height() - 2, board.width() - 2, 1};
		Region_t east = {board.width() - 2, 2, 1, board.height() - 4};
		Region_t west = {1, 2, 1, board.height() - 4}; 

		AsyncIO io(board, calculate_topology(size, 
			std::make_pair(header.width, header.height)));
		for(size_t i = 0; i < header.generations; i++)
		{
			io.begin();
			step_region(center, board, result_board);	
			io.end();

			step_region(north, board, result_board);
			step_region(south, board, result_board);
			step_region(west, board, result_board);
			step_region(east, board, result_board);

			for(size_t y = 0; y < board.height(); y++)
			{
				for(size_t x = 0; x < board.width(); x++)
				{
					if(y == 0 || y == board.height() - 1)
						board[y][x] = false;
					else if(x == 0 || x == board.width() - 1)
						board[y][x] = false;
					else board[y][x] = result_board[y][x];
				}
			}
		}
	}

	// Write the output.	
	std::ofstream out(argv[2]);
	if(!gather_board(out, board, header))
	{
		MPI_Abort(MPI_COMM_WORLD, STATUS_WRITE_ERROR);
	}
	out.close();

	MPI_Finalize();
	return STATUS_SUCCESS;
}

bool scatter_board(std::istream &in, LifeBoard &local_board, LifeHeader_t &header)
{
	int32_t rank;
	int32_t size;
	LifeBoard board;
	uint32_t parameters[3];
	std::pair<size_t, size_t> topology;
	std::pair<size_t, size_t> loc;
	std::pair<size_t, size_t> offset;
	std::pair<size_t, size_t> local_size;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	// Root processor reads the file
	if(rank == 0)
	{
		if(readFile(in, board, header))
		{
			parameters[0] = header.width;
			parameters[1] = header.height;
			parameters[2] = header.generations;
		}
		else
		{
			parameters[0] = 0;
			parameters[1] = 0;
			parameters[2] = 0;
		}
	}

	// Send the header to each processor
	MPI_Bcast(parameters, 3, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
	header.width = parameters[0];
	header.height = parameters[1];
	header.generations = parameters[2];
	if(header.width == 0 || header.height == 0)
		return false;
	topology = calculate_topology(size, std::make_pair(header.width, header.height));
	loc = map_processor(rank, topology);

	if(rank != 0)
		board.resize(header.width, header.height);

	// Resize local board with a margin of 1 row/column
	local_size = std::make_pair(
		subgrid_width(rank, topology.first, header.width) + 2,
		subgrid_height(rank, topology.second, topology.first, header.height) + 2);
	local_board.resize(local_size.first, local_size.second);

	// Send the file contents to each processor 
	MPI_Bcast(
		board[0],
		board.width() * board.height(),
		MPI_CHAR,
		0,
		MPI_COMM_WORLD);

	// Find this processors chunk of the board
	offset = calculate_offsets(
		loc,
		topology,
		std::make_pair(board.width(), board.height()));

	// Copy the chunk into the local board with a margin of 1 cell
	for(size_t y = 0; y < local_board.height(); y++)
	{
		for(size_t x = 0; x < local_board.width(); x++)
		{
			if((x == 0) || (y == 0) || (x == local_board.width() - 1) || (y == local_board.height() - 1))
				local_board[y][x] = false;
			else
				local_board[y][x] = board[y + offset.second - 1][x + offset.first - 1];
		}
	}

	return true;
}

void pad_buffer(const LifeBoard &board, bool *buffer)
{	
	size_t buffer_index = 0;
	for(size_t y = 1; y < (board.height()) - 1; y++)
	{
		for(size_t x = 1; x < (board.width()) - 1; x++)
		{
			buffer[buffer_index++] = board[y][x];
		}
	}
}

void unpad_buffer(LifeBoard &board, const bool *buffer)
{
	int32_t rank;
	int32_t size;
	std::pair<size_t, size_t> topology;
	std::pair<size_t, size_t> subgrid_size;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	topology = calculate_topology(size, std::make_pair(board.width(), board.height()));
	subgrid_size = std::make_pair(
		subgrid_width(0, topology.first, board.width()),
		subgrid_height(0, topology.second, topology.first, board.height()));

	size_t by = 0; // board x-index
	size_t bx = 0; // board y-index

	// Loop through processor rows
	for(size_t ty = 0; ty < topology.second; ty++)
	{
		// Number of rows in the current processor row
		size_t height = subgrid_height(
			ty * topology.first,
			topology.second,
			topology.first,
			board.height());

		// Loop through rows of the board in the current processor row
		for(size_t y = 0; y < height; y++, by++)
		{
	
			// Loop through processor columns
			for(size_t tx = 0, bx = 0; tx < topology.first; tx++)
			{
				// Number of columns in the current processor column
				size_t width = subgrid_width(
					(ty * topology.first) + tx,
					topology.first,
					board.width());

				// Loop through columns of the current processor column
				for(size_t x = 0; x < width; x++)
				{

					// Copy the cell into the the board position
					size_t index =
						((ty * topology.first) + tx) * 
						(subgrid_size.first * subgrid_size.second);
					index += (y * width) + x;
					board[by][bx++] = buffer[index];	
				}
			}
		}
	}
}

bool gather_board(std::ostream &out, const LifeBoard &local_board, const LifeHeader_t &header)
{
	int32_t rank;
	int32_t size;
	size_t send_size;
	bool *send_buffer;
	bool *recv_buffer;
	bool result;
	std::pair<size_t, size_t> topology;
	std::pair<size_t, size_t> subgrid_size;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	topology = calculate_topology(size, std::make_pair(header.width, header.height));
	subgrid_size = std::make_pair(
		subgrid_width(0, topology.first, header.width),
		subgrid_height(0, topology.second, topology.first, header.height));

	// The usual &vector[0] trick was causing issues; using raw arrays as a fallback.
	send_size = subgrid_size.first * subgrid_size.second;
	send_buffer = new bool[subgrid_size.first * subgrid_size.second];
	recv_buffer = new bool[(topology.first * topology.second) * (subgrid_size.first * subgrid_size.second)];
	pad_buffer(local_board, send_buffer);

	MPI_Gather(
		send_buffer,
		send_size,
		MPI_CHAR,
		recv_buffer,
		send_size,
		MPI_CHAR,
		0,
		MPI_COMM_WORLD);

	// Write the result to the output stream	
	if(rank == 0)
	{
		LifeBoard board(header.width, header.height);
		unpad_buffer(board, recv_buffer);
		result = writeFile(out, board, header);
	}
	else
	{
		result = true;
	}
	
	delete [] send_buffer;
	delete [] recv_buffer;
	return result;
}

std::pair<size_t, size_t> map_processor(size_t index, const std::pair<size_t, size_t> &topology)
{
	size_t x = index % topology.first;
	size_t y = index / topology.first;
	return std::pair<size_t, size_t>(x, y);
}

size_t subgrid_width(size_t index, size_t columns, size_t board_width)
{
	size_t column = index % columns;
	if((board_width % columns) > column)
		return (board_width / columns) + 1;
	else
		return (board_width / columns);
}

size_t subgrid_height(size_t index, size_t rows, size_t columns, size_t board_height)
{
	size_t row = index / columns;
	if((board_height % rows) > row)
		return (board_height / rows) + 1;
	else
		return (board_height / rows);
}

std::pair<size_t, size_t> calculate_offsets(
	const std::pair<size_t, size_t> &loc,
	const std::pair<size_t, size_t> &topology,
	const std::pair<size_t, size_t> &board_size)
{
	std::pair<size_t, size_t> result = std::make_pair(0, 0);

	// Sum subgrid heights for prior rows
	for(size_t y = 0; y < loc.second; y++)
	{
		result.second += subgrid_height(y * topology.first, topology.second, topology.first, board_size.second);
	}

	// Sum subgrid widths for all prior columns
	for(size_t x = 0; x < loc.first; x++)
	{
		result.first += subgrid_width(x, topology.first, board_size.first);
	}

	return result;
}
