#pragma once

#include <glm/glm.hpp>

#include "Types.hpp"

/*
	This header file contains the implementation of the Marching Squares algorithm.
	It also contains the code for performing 
*/

typedef ui8 State;

namespace MarchingSquares {

	/* Gets the next segment of this segment in a counterclockwise manner */
	inline bool nextSegment(State* states, i64 x, i64 y, i64 w, bool fromPositive, i64& nx, i64& ny) {
		switch (states[x + y * w]) {
		// nothing can traverse this state
		case 0: return false;
		case 1: nx = x; ny = y - 1; return true;
		case 2: 
		case 3: nx = x + 1; ny = y; return true;
		case 4: nx = x; ny = y + 1; return true;
		case 5: nx = x; ny = fromPositive ? y + 1 : y - 1; return true;
		case 6: nx = x; ny = y + 1; return true;
		case 7: nx = x; ny = y + 1; return true;
		case 8: nx = x - 1; ny = y; return true;
		case 9: nx = x; ny = y - 1; return true;
		case 10: nx = fromPositive ? x - 1 : x + 1; ny = y; return true;
		case 11: nx = x + 1; ny = y; return true;
		case 12: nx = x - 1; ny = y; return true;
		case 13: nx = x; ny = y - 1; return true;
		case 14: nx = x - 1; ny = y; return true;
		case 15: return false;
		default: return false;
		}
	}

	struct Contour {
		std::vector<glm::vec2> vertices;
	};

	/* Iterative Douglas Peucker to simplify my contour */
	void DouglasPeucker(i64 start, i64 end, std::vector<i64> simplified, const std::vector<glm::vec2>& contour) {
		//if (start)
	}

	void MarchingSquares(i64 w, i64 h, ui8* data, std::vector<Contour>& contours) {
		i64 nw = w - 1, nh = h - 1;
		State* states = new State[nw * nh];
		bool* visited = new bool[nw * nh];
		
		// map pixel values to states using LUT
		for (i64 x = 0; x < nw; x++) {
			for (i64 y = 0; y < nh; y++) {
				states[x + y * nw] = (data[x + y * w]) | (data[x + 1 + y * w] << 1) | (data[x + 1 + (y + 1) * w] << 2) | (data[x + (y + 1) * w] << 3);
				visited[x + y * nw] = false;
			}
		}

		// simplify states to contours
		contours.clear();
		for (i64 x = 0; x < nw; x++) {
			for (i64 y = 0; y < nh; y++) {
				// check state
				State& state = states[x + y * nw];
				bool& isVisited = visited[x + y * nw];

				
				// if this state is 0 or 16, there is no contour segment here
				// if this state is 5 or 10, don't start walking here :)
				// if we already looked at this segment, don't walk here :)
				if (state == 0 || state == 15 || state == 5 || state == 10 || isVisited) continue;

				// we found a valid segment to walk!
				// we always want to walk counter-clockwise using screen space coords.
				// even though the game is inverted screen space coords, it doesn't matter
				// because this algo should be symmetrical
				Contour c;

				i64 px, py;
				i64 cx = x, cy = y;
				i64 nx, ny;
				bool fromPositive;
				while (1) {
					// if we have already seen this vertex, we are done!
					if (visited[cx + cy * nw]) break;

					// push current x + 1 and current y + 1 into the contour
					c.vertices.push_back({ cx + 1, cy + 1 });

					// walk along the contour!
					State& cState = states[cx + cy * nw];

					// special handling of states 5 and 10
					// we need to know where we come from 
					// to know where to go
					if (cState == 5) {
						fromPositive = px > cx;
					}
					else if (cState == 10) {
						fromPositive = py > cy;
					}
					else {
						// mark current vertex as visited
					}

					visited[cx + cy * nw] = true;
					// find the next segment
					nextSegment(states, cx, cy, nw, fromPositive, nx, ny);
					if (nx < 0 || ny < 0 || nx >= nw || ny >= nh) {
						break;
					}

					px = cx;
					py = cy;
					cx = nx;
					cy = ny;
				}
				contours.push_back(c);
			}
		}

		// douglas peucker them contours!
		for (auto contour : contours) {
			Contour approximation;

			// pick endpoints of original contour

		}


		delete[] states;
		delete[] visited;
	}
}