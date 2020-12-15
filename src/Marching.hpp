#pragma once

#include <bitset>
#include <stack>
#include <algorithm>
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

	float dist(const glm::vec2& p, const glm::vec2& s, const glm::vec2& e) {
		return std::abs((e.x - s.x) * (s.y - p.y) - (s.x - p.x) * (e.y - s.y)) / std::sqrt((e.x - s.x) * (e.x - s.x) + (e.y - s.y) * (e.y - s.y));
	}

	/* Iterative Douglas Peucker to simplify my contour */
	void DouglasPeucker(std::vector<glm::vec2>& simplified, const std::vector<glm::vec2>& contour, float epsilon) {
		// it's fine to use std::vector<bool> because c++ has an
		// space optimization for bool vectors.
		std::vector<bool> keepVertex(contour.size(), true);
		std::stack<std::pair<i64, i64>> indicesStack;
		indicesStack.push({ 0, contour.size() - 1 });

		i64 startIndex, endIndex;
		while (!indicesStack.empty()) {
			std::pair<i64, i64> currentIter = indicesStack.top();
			startIndex = currentIter.first;
			endIndex = currentIter.second;
			indicesStack.pop();

			float dmax = 0;
			int index = startIndex;

			for (i64 i = index + 1; i < endIndex; i++) {
				if (keepVertex[i]) {
					float d = dist(contour[i], contour[startIndex], contour[endIndex]);
					if (d > dmax) {
						index = i;
						dmax = d;
					}
				}
			}

			if (dmax > epsilon) {
				indicesStack.push({ startIndex, index });
				indicesStack.push({ index, endIndex });
			}
			else {
				for (int i = startIndex + 1; i < endIndex; i++) {
					keepVertex[i] = false;
				}
			}
		}

		i64 contourSize = contour.size();
		for (i64 i = 0; i < contourSize; i++) {
			if (keepVertex[i]) {
				simplified.push_back(contour[i]);
			}
		}
	}

	inline ui8 DataAt(const ui8* data, i64 x, i64 y, i64 w, i64 h) {
		if (x < 0 || y < 0 || x >= w || y >= h) {
			return 0;
		}

		return data[x + y * w];
	}

	void MarchingSquares(i64 w, i64 h, ui8* data, std::vector<Contour>& contours) {
		contours.clear();
		// pad zeroes around the states
		i64 nw = w + 1, nh = h + 1;
		State* states = new State[nw * nh];

		// 000000 <Visited from positive?> <Visited from negative?> 
		ui8* visited = new ui8[nw * nh];

		std::fill(states, states + nw * nh, 0);
		std::fill(visited, visited + nw * nh, 0);
		
		// map pixel values to states using LUT
		for (i64 x = 0; x < nw; x++) {
			for (i64 y = 0; y < nh; y++) {
				states[x + y * nw] = (DataAt(data, x - 1, y - 1, w, h)) | (DataAt(data, x, y - 1, w, h) << 1) | (DataAt(data, x, y, w, h) << 2) | (DataAt(data, x - 1, y, w, h) << 3);
				visited[x + y * nw] = false;
			}
		}

		// simplify states to contours
		std::vector<Contour> complexContours;
		for (i64 x = 0; x < nw; x++) {
			for (i64 y = 0; y < nh; y++) {
				// check state
				State& state = states[x + y * nw];
				ui8& isVisited = visited[x + y * nw];

				
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
				bool fromPositive = false;
				while (1) {
					State& cState = states[cx + cy * nw];
					ui8& currentVisited = visited[cx + cy * nw];

					bool visitedPositional = (bool)currentVisited;

					// special handling of states 5 and 10
					// we need to know where we come from 
					// to know where to go
					if (cState == 5) {
						fromPositive = px > cx;
					}
					else if (cState == 10) {
						fromPositive = py > cy;
					}

					if (cState == 5 || cState == 10) {
						visitedPositional = fromPositive ? ((currentVisited >> 1) > 0) : ((currentVisited & 1) > 0);
					}

					// if we have already seen this vertex, we are done!
					if (visitedPositional) break;

					// walk along the contour!

					if (cState == 5 || cState == 10) {
						visited[cx + cy * nw] |= fromPositive ? 2 : 1;
					}
					else {
						// mark current vertex as visited
						visited[cx + cy * nw] = true;
					}

					// find the next segment
					nextSegment(states, cx, cy, nw, fromPositive, nx, ny);

					// push current cell onto the list
					c.vertices.push_back({ cx + (nx - cx) * 0.5, cy + (ny - cy) * 0.1 });

					px = cx;
					py = cy;
					cx = nx;
					cy = ny;
				}

				if (c.vertices.size() > 10) {
					complexContours.push_back(c);
				}
			}
		}

		// douglas peucker them contours!
		for (auto contour : complexContours) {
#ifdef DOUGLAS_PEUCKER
			Contour approximation;
			DouglasPeucker(approximation.vertices, contour.vertices, .5);
			contours.push_back(approximation);
#else
			contours.push_back(contour);
#endif
		}


		delete[] states;
		delete[] visited;
	}
}