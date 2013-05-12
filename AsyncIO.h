#ifndef ASYNCIO_H
#define ASYNCIO_H
/*
 *       File:           AsyncIO.h
 *       Description:    For wrapping MPI communication between adjacent processors
 *       Authors:        Scott Connell && Joel Rausch
 *       Date Created:   May 5, 2013 at 22:02
 *
 *       This file written for Programming Assignment 3 for CprE 426.
 *       Iowa State University
 *
 */

#include <mpi.h>
#include "LifeUtil.h"

// Alias the type used to represent topology.
typedef std::pair<size_t, size_t> Topology_t;

// Wrap async MPI communication between adjacent processors.
class AsyncIO
{
public:
	// Bind to a board for the provided topology.
	AsyncIO(
		LifeBoard &board,
		const Topology_t &topology);

	// Dtor.
	~AsyncIO();

	// Begin async communication.
	void begin();

	// Wait for the async communication to complete.
	void end();

	// Number of processors we depend on.
	size_t links() const;

private:
	MPI_Datatype _columnType;
	MPI_Datatype _rowType;
	MPI_Request _send_requests[8];
	MPI_Request _recv_requests[8];
	MPI_Status _statuses[8]; 
	size_t _links;
};

#endif // ASYNCIO_H
