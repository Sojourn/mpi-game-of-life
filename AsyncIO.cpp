/*
 *       File:           AsyncIO.cpp
 *       Description:    Implementation of the AsyncIO class
 *       Authors:        Scott Connell && Joel Rausch
 *       Date Created:   May 5, 2013 at 22:05
 *
 *       This file written for Programming Assignment 3 for CprE 426.
 *       Iowa State University
 *
 */
#include "AsyncIO.h"
#include <sstream>
#include <iostream>

inline bool valid(const std::pair<int32_t, int32_t> &loc, const Topology_t &topology)
{
	return 
		(loc.first >= 0) && (loc.second >= 0) &&
		(topology.first > loc.first) && (topology.second > loc.second);
}

inline std::pair<int32_t, int32_t> map(int32_t rank, const Topology_t &topology)
{
	int32_t x = rank % topology.first;
	int32_t y = rank / topology.first;
	return std::make_pair(x, y);
}

inline int32_t map(const std::pair<int32_t, int32_t> &loc, const Topology_t &topology)
{
	return (loc.second * topology.first) + loc.first;
}

inline void dump_coord(const std::pair<int32_t, int32_t> &coord, int32_t rank)
{
	std::stringstream ss;
	ss << "[" << rank << "]: " << coord.first << ", " << coord.second << std::endl;
	std::cout << ss.str().c_str();
}

AsyncIO::AsyncIO(
	LifeBoard &board,
	const std::pair<size_t, size_t> &topology) :
	_links(0)
{
	int32_t local_rank;
	std::pair<int32_t, int32_t> local_coord;
	std::pair<int32_t, int32_t> coord; 

	MPI_Comm_rank(MPI_COMM_WORLD, &local_rank);
	local_coord = map(local_rank, topology);

	// Construct types for sending rows and columns
	MPI_Type_vector(
		board.height() - 2,
		1,
		board.width(),
		MPI_CHAR,
		&_columnType);
	MPI_Type_vector(
		1,
		board.width() - 2,
		1,
		MPI_CHAR,
		&_rowType);
	MPI_Type_commit(&_columnType);
	MPI_Type_commit(&_rowType);

	// NW
	coord = std::make_pair(local_coord.first - 1, local_coord.second - 1);
	if(valid(coord, topology))
	{
		int32_t rank = map(coord, topology);

		MPI_Send_init(
			&board[1][1],
			1,
			MPI_CHAR,
			rank,
			0,
			MPI_COMM_WORLD,
			&_send_requests[_links]);
		
		MPI_Recv_init(
			&board[0][0],
			1,
			MPI_CHAR,
			rank,
			MPI_ANY_TAG,
			MPI_COMM_WORLD,
			&_recv_requests[_links]);

		_links++;
	}

	// NE
	coord = std::make_pair(local_coord.first + 1, local_coord.second - 1);	
	if(valid(coord, topology))
	{
		int32_t rank = map(coord, topology);

		MPI_Send_init(
			&board[1][board.width() - 2],
			1,
			MPI_CHAR,
			rank,
			0,
			MPI_COMM_WORLD,
			&_send_requests[_links]);
		
		MPI_Recv_init(
			&board[0][board.width() - 1],
			1,
			MPI_CHAR,
			rank,
			MPI_ANY_TAG,
			MPI_COMM_WORLD,
			&_recv_requests[_links]);

		_links++;
	}

	// SE
	coord = std::make_pair(local_coord.first + 1, local_coord.second + 1);	
	if(valid(coord, topology))
	{
		int32_t rank = map(coord, topology);

		MPI_Send_init(
			&board[board.height() - 2][board.width() - 2],
			1,
			MPI_CHAR,
			rank,
			0,
			MPI_COMM_WORLD,
			&_send_requests[_links]);
		
		MPI_Recv_init(
			&board[board.height() - 1][board.width() - 1],
			1,
			MPI_CHAR,
			rank,
			MPI_ANY_TAG,
			MPI_COMM_WORLD,
			&_recv_requests[_links]);

		_links++;
	}

	// SW
	coord = std::make_pair(local_coord.first - 1, local_coord.second + 1);	
	if(valid(coord, topology))
	{
		int32_t rank = map(coord, topology);

		MPI_Send_init(
			&board[board.height() - 2][1],
			1,
			MPI_CHAR,
			rank,
			0,
			MPI_COMM_WORLD,
			&_send_requests[_links]);
		
		MPI_Recv_init(
			&board[board.height() - 1][0],
			1,
			MPI_CHAR,
			rank,
			MPI_ANY_TAG,
			MPI_COMM_WORLD,
			&_recv_requests[_links]);

		_links++;
	}

	// N
	coord = std::make_pair(local_coord.first, local_coord.second - 1);
	if(valid(coord, topology))
	{
		int32_t rank = map(coord, topology);

		MPI_Send_init(
			&board[1][1],
			1,
			_rowType,
			rank,
			0,
			MPI_COMM_WORLD,
			&_send_requests[_links]);

		MPI_Recv_init(
			&board[0][1],
			1,
			_rowType,
			rank,
			MPI_ANY_TAG,
			MPI_COMM_WORLD,
			&_recv_requests[_links]);

		_links++;
	}

	// S
	coord = std::make_pair(local_coord.first, local_coord.second + 1);
	if(valid(coord, topology))
	{
		int32_t rank = map(coord, topology);

		MPI_Send_init(
			&board[board.height() - 2][1],
			1,
			_rowType,
			rank,
			0,
			MPI_COMM_WORLD,
			&_send_requests[_links]);

		MPI_Recv_init(
			&board[board.height() - 1][1],
			1,
			_rowType,
			rank,
			MPI_ANY_TAG,
			MPI_COMM_WORLD,
			&_recv_requests[_links]);

		_links++;
	}	

	// E
	coord = std::make_pair(local_coord.first + 1, local_coord.second);
	if(valid(coord, topology))
	{
		int32_t rank = map(coord, topology);

		MPI_Send_init(
			&board[1][board.width() - 2],
			1,
			_columnType,
			rank,
			0,
			MPI_COMM_WORLD,
			&_send_requests[_links]);

		MPI_Recv_init(
			&board[1][board.width() - 1],
			1,
			_columnType,
			rank,
			MPI_ANY_TAG,
			MPI_COMM_WORLD,
			&_recv_requests[_links]);

		_links++;
	}

	// W
	coord = std::make_pair(local_coord.first - 1, local_coord.second);
	if(valid(coord, topology))
	{
		int32_t rank = map(coord, topology);

		MPI_Send_init(
			&board[1][1],
			1,
			_columnType,
			rank,
			0,
			MPI_COMM_WORLD,
			&_send_requests[_links]);

		MPI_Recv_init(
			&board[1][0],
			1,
			_columnType,
			rank,
			MPI_ANY_TAG,
			MPI_COMM_WORLD,
			&_recv_requests[_links]);

		_links++;
	}
}

AsyncIO::~AsyncIO()
{
	for(size_t i = 0; i < _links; i++)
	{
		MPI_Request_free(&_send_requests[i]);
		MPI_Request_free(&_recv_requests[i]);
	}
	
	MPI_Type_free(&_columnType);
	MPI_Type_free(&_rowType);
}

void AsyncIO::begin()
{
	MPI_Startall(_links, _send_requests);
	MPI_Startall(_links, _recv_requests);
}

void AsyncIO::end()
{
	MPI_Waitall(_links, _recv_requests, _statuses);
	MPI_Waitall(_links, _send_requests, _statuses);
}
	
size_t AsyncIO::links() const
{
	return _links;
}
